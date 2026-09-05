#ifndef UI_DRAW_H
#define UI_DRAW_H

#include <SDL.h>
#include <stdbool.h>

typedef struct {
    const char* id;
    const char* name;
    SDL_Color bg;
    SDL_Color surface;
    SDL_Color surface_high;
    SDL_Color sunken;
    SDL_Color shadow_dark;
    SDL_Color shadow_light;
    SDL_Color border;
    SDL_Color accent_blue;
    SDL_Color accent_mint;
    SDL_Color accent_amber;
    SDL_Color accent_lavender;
    SDL_Color accent_rose;
    SDL_Color text_bright;
    SDL_Color text_muted;
} UITheme;

#define THEME_COUNT 4
extern const UITheme THEMES[THEME_COUNT];
extern const UITheme* g_theme;

void theme_init(void);
void theme_set(int index);
int theme_get_index(void);
const UITheme* theme_get_current(void);

/* Dynamic Theme Palette Accessors */
#define NEU_BG             (g_theme->bg)
#define NEU_SURFACE        (g_theme->surface)
#define NEU_SURFACE_HIGH   (g_theme->surface_high)
#define NEU_SUNKEN         (g_theme->sunken)
#define NEU_SHADOW_DARK    (g_theme->shadow_dark)
#define NEU_SHADOW_LIGHT   (g_theme->shadow_light)
#define NEU_BORDER         (g_theme->border)
#define NEU_ACCENT_BLUE    (g_theme->accent_blue)
#define NEU_ACCENT_MINT    (g_theme->accent_mint)
#define NEU_ACCENT_AMBER   (g_theme->accent_amber)
#define NEU_ACCENT_LAVENDER (g_theme->accent_lavender)
#define NEU_ACCENT_ROSE    (g_theme->accent_rose)

/* Semantic Aliases */
#define NEU_CYAN           NEU_ACCENT_BLUE
#define NEU_GREEN          NEU_ACCENT_MINT
#define NEU_ORANGE         NEU_ACCENT_AMBER
#define NEU_PURPLE         NEU_ACCENT_LAVENDER
#define NEU_RED            NEU_ACCENT_ROSE

#define COLOR_BG           NEU_BG
#define COLOR_PANEL        NEU_SURFACE
#define COLOR_PANEL_BORDER NEU_BORDER
#define COLOR_ACCENT_CYAN  NEU_ACCENT_BLUE
#define COLOR_ACCENT_GREEN NEU_ACCENT_MINT
#define COLOR_ACCENT_PURPLE NEU_ACCENT_LAVENDER
#define COLOR_ACCENT_ORANGE NEU_ACCENT_AMBER
#define COLOR_TEXT_BRIGHT  (g_theme->text_bright)
#define COLOR_TEXT_MUTED   (g_theme->text_muted)
#define COLOR_HIGHLIGHT    NEU_SURFACE_HIGH
#define COLOR_ACTIVE       NEU_ACCENT_BLUE

/* Smooth Anti-Aliased Shapes */
void ui_draw_filled_circle(SDL_Renderer* r, int cx, int cy, int radius, SDL_Color color);
void ui_draw_circle(SDL_Renderer* r, int cx, int cy, int radius, int thickness, SDL_Color color);
void ui_draw_sector_ring(SDL_Renderer* r, int cx, int cy, int r_in, int r_out,
                         float start_rad, float end_rad, SDL_Color color);
void ui_draw_rounded_rect(SDL_Renderer* r, int x, int y, int w, int h, int radius,
                          SDL_Color color, bool filled);
void ui_draw_thick_line(SDL_Renderer* r, int x1, int y1, int x2, int y2, int thickness, SDL_Color color);

/* Neumorphic Primitives with Soft Diffused Lighting */
void ui_draw_neu_panel(SDL_Renderer* r, int x, int y, int w, int h, int radius,
                       bool inset, bool glow_active, SDL_Color glow_col);
void ui_draw_neu_circle(SDL_Renderer* r, int cx, int cy, int radius,
                        bool inset, bool glow_active, SDL_Color glow_col);
void ui_draw_neu_well_circle(SDL_Renderer* r, int cx, int cy, int radius);
void ui_draw_glowing_ring(SDL_Renderer* r, int cx, int cy, int radius, int thickness, SDL_Color color);
void ui_draw_led_indicator(SDL_Renderer* r, int cx, int cy, int radius, bool on, SDL_Color color);

#endif /* UI_DRAW_H */
