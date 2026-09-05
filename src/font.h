#ifndef FONT_H
#define FONT_H

#include <SDL.h>
#include <stdbool.h>

typedef enum {
    FONT_SIZE_SMALL = 0,   /* ~14px */
    FONT_SIZE_MEDIUM,      /* ~20px */
    FONT_SIZE_LARGE,       /* ~32px */
    FONT_SIZE_HUGE,        /* ~52px */
    FONT_SIZE_COUNT
} FontSize;

typedef struct FontManager FontManager;

FontManager* font_init(void);
void font_shutdown(FontManager* fm);

void font_draw_text(FontManager* fm, SDL_Renderer* renderer, const char* text,
                    int x, int y, FontSize size, SDL_Color color, bool center_x, bool center_y);

void font_measure_text(FontManager* fm, const char* text, FontSize size, int* out_w, int* out_h);

#endif /* FONT_H */
