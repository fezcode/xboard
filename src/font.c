#include "font.h"
#include <SDL_ttf.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static const char* FONT_REGULAR_CANDIDATES[] = {
    "C:/Windows/Fonts/segoeui.ttf",
    "C:/Windows/Fonts/seguisb.ttf",
    "C:/Windows/Fonts/arial.ttf",
    "C:/Windows/Fonts/consola.ttf",
    NULL
};

static const char* FONT_BOLD_CANDIDATES[] = {
    "C:/Windows/Fonts/seguisb.ttf",
    "C:/Windows/Fonts/segoeuib.ttf",
    "C:/Windows/Fonts/arialbd.ttf",
    "C:/Windows/Fonts/consola.ttf",
    NULL
};

typedef struct { char* text; SDL_Texture* texture; FontSize size; SDL_Color color; int w, h; } TextCache;
#define CACHE_SIZE 256
struct FontManager {
    TextCache cache[CACHE_SIZE];
    unsigned next;
    TTF_Font* fonts[FONT_SIZE_COUNT];
};

static const int SIZES[FONT_SIZE_COUNT] = { 14, 21, 30, 52 };

static const char* find_available_font(const char* const candidates[]) {
    for (int i = 0; candidates[i] != NULL; ++i) {
        FILE* f = fopen(candidates[i], "rb");
        if (f) {
            fclose(f);
            return candidates[i];
        }
    }
    return NULL;
}

FontManager* font_init(void) {
    if (TTF_Init() < 0) {
        SDL_Log("[Font] Failed to init SDL_ttf: %s", TTF_GetError());
        return NULL;
    }

    FontManager* fm = (FontManager*)calloc(1, sizeof(FontManager));
    if (!fm) return NULL;

    const char* reg_path = find_available_font(FONT_REGULAR_CANDIDATES);
    const char* bold_path = find_available_font(FONT_BOLD_CANDIDATES);

    if (!reg_path && !bold_path) {
        SDL_Log("[Font] Could not find any standard Windows font!");
        free(fm);
        return NULL;
    }
    if (!reg_path) reg_path = bold_path;
    if (!bold_path) bold_path = reg_path;

    for (int i = 0; i < FONT_SIZE_COUNT; ++i) {
        /* Use regular font for small/medium UI labels to avoid chunky aliased distortion;
         * Use semibold/bold font for large/huge headers and mode titles. */
        const char* p = (i <= FONT_SIZE_MEDIUM) ? reg_path : bold_path;
        fm->fonts[i] = TTF_OpenFont(p, SIZES[i]);
        if (!fm->fonts[i]) {
            SDL_Log("[Font] Failed to open font size %d: %s", SIZES[i], TTF_GetError());
        } else {
            /* Enable light hinting for crisp anti-aliased glyph outlines without pixel clumping */
            TTF_SetFontHinting(fm->fonts[i], TTF_HINTING_LIGHT);
            TTF_SetFontKerning(fm->fonts[i], 1);
        }
    }

    return fm;
}

void font_shutdown(FontManager* fm) {
    if (!fm) return;
    for (int i = 0; i < FONT_SIZE_COUNT; ++i) {
        if (fm->fonts[i]) {
            TTF_CloseFont(fm->fonts[i]);
        }
    }
    for (int i=0;i<CACHE_SIZE;++i) { SDL_DestroyTexture(fm->cache[i].texture); free(fm->cache[i].text); }
    free(fm);
    TTF_Quit();
}

void font_draw_text(FontManager* fm, SDL_Renderer* renderer, const char* text,
                    int x, int y, FontSize size, SDL_Color color, bool center_x, bool center_y) {
    if (!fm || !text || text[0] == '\0' || !renderer) return;
    if (size < 0 || size >= FONT_SIZE_COUNT || !fm->fonts[size]) return;

    TextCache* entry = NULL;
    for (int i=0;i<CACHE_SIZE;++i) {
        TextCache* c=&fm->cache[i];
        if(c->text && c->size==size && memcmp(&c->color,&color,sizeof(color))==0 && strcmp(c->text,text)==0) { entry=c; break; }
    }
    if (!entry) {
        SDL_Surface* surf=TTF_RenderUTF8_Blended(fm->fonts[size],text,color);
        if(!surf) return;
        SDL_Texture* tex=SDL_CreateTextureFromSurface(renderer,surf);
        char* copy=(char*)malloc(strlen(text)+1);
        if(!tex || !copy) { SDL_DestroyTexture(tex); free(copy); SDL_FreeSurface(surf); return; }
        strcpy(copy,text);
        entry=&fm->cache[fm->next++ % CACHE_SIZE];
        SDL_DestroyTexture(entry->texture); free(entry->text);
        *entry=(TextCache){copy,tex,size,color,surf->w,surf->h};
        SDL_FreeSurface(surf);
    }
    SDL_Rect dst={center_x?x-entry->w/2:x,center_y?y-entry->h/2:y,entry->w,entry->h};
    SDL_RenderCopy(renderer,entry->texture,NULL,&dst);
}

void font_measure_text(FontManager* fm, const char* text, FontSize size, int* out_w, int* out_h) {
    if (out_w) *out_w = 0;
    if (out_h) *out_h = 0;
    if (!fm || !text || text[0] == '\0') return;
    if (size < 0 || size >= FONT_SIZE_COUNT || !fm->fonts[size]) return;

    TTF_SizeUTF8(fm->fonts[size], text, out_w, out_h);
}
