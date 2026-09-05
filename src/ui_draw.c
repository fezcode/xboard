#include "ui_draw.h"
#include <math.h>

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* =========================================================================
 * Multi-Theme Preset Definitions
 * ========================================================================= */
const UITheme THEMES[THEME_COUNT] = {
    /* 0: Studio (Modern Luxury Stealth) */
    {
        .id = "obsidian",
        .name = "Studio",
        .bg             = { 15, 17, 23, 255 },     /* Ultra-deep space black (#0d0f14) */
        .surface        = { 25, 28, 37, 255 },     /* Matte titanium plate (#181b24) */
        .surface_high   = { 34, 38, 52, 255 },     /* Elevated surface (#222634) */
        .sunken         = { 8, 9, 13, 255 },       /* Sunken velvet well (#08090d) */
        .shadow_dark    = { 3, 4, 6, 220 },        /* Diffuse drop shadow */
        .shadow_light   = { 65, 78, 105, 120 },    /* Soft pearl highlight */
        .border         = { 38, 44, 60, 255 },     /* Architectural border */
        .accent_blue    = { 157, 180, 255, 255 },   /* Electric Sky Blue (#38bdf8) */
        .accent_mint    = { 52, 211, 153, 255 },   /* Soft Mint Emerald (#34d399) */
        .accent_amber   = { 251, 146, 60, 255 },   /* Warm Sunset Amber (#fb923c) */
        .accent_lavender = { 167, 139, 250, 255 }, /* Royal Lavender (#a78bfa) */
        .accent_rose    = { 244, 63, 94, 255 },    /* Coral Rose (#f43f5e) */
        .text_bright    = { 248, 250, 252, 255 },  /* Pure Snow White (#f8fafc) */
        .text_muted     = { 156, 168, 188, 255 }   /* Cool Slate Gray (#9ca8bc) */
    },
    /* 1: Violet (Neon Synthwave / Cyberpunk) */
    {
        .id = "cyber",
        .name = "Violet",
        .bg             = { 14, 11, 24, 255 },     /* Deep cosmic violet (#0e0b18) */
        .surface        = { 28, 22, 48, 255 },     /* Elevated plum plate (#1c1630) */
        .surface_high   = { 40, 32, 70, 255 },     /* Neon highlight surface (#282046) */
        .sunken         = { 9, 7, 16, 255 },       /* Sunken onyx void (#090710) */
        .shadow_dark    = { 4, 3, 8, 230 },        /* Violet shadow */
        .shadow_light   = { 100, 80, 160, 115 },   /* Magenta specular sheen */
        .border         = { 56, 44, 96, 255 },     /* Neon perimeter (#382c60) */
        .accent_blue    = { 6, 214, 240, 255 },    /* Cyber Cyan (#06d6f0) */
        .accent_mint    = { 52, 211, 153, 255 },   /* Matrix Mint (#34d399) */
        .accent_amber   = { 250, 204, 21, 255 },   /* Solar Cyber Gold (#facc15) */
        .accent_lavender = { 192, 132, 252, 255 }, /* Electric Violet (#c084fc) */
        .accent_rose    = { 244, 63, 140, 255 },   /* Neon Hot Pink (#f43f8c) */
        .text_bright    = { 255, 255, 255, 255 },  /* Hyper White */
        .text_muted     = { 175, 165, 210, 255 }   /* Soft Violet Mist */
    },
    /* 2: Ocean (Deep Polar Navy) */
    {
        .id = "nordic",
        .name = "Ocean",
        .bg             = { 10, 17, 32, 255 },     /* Deepest polar abyss (#0a1120) */
        .surface        = { 19, 31, 56, 255 },     /* Frosted ice plate (#131f38) */
        .surface_high   = { 28, 45, 80, 255 },     /* Glacial surface (#1c2d50) */
        .sunken         = { 7, 11, 22, 255 },      /* Sub-zero well (#070b16) */
        .shadow_dark    = { 2, 5, 12, 225 },       /* Deep water shadow */
        .shadow_light   = { 75, 115, 175, 115 },   /* Specular aurora highlight */
        .border         = { 40, 62, 102, 255 },    /* Crystalline border */
        .accent_blue    = { 56, 189, 248, 255 },   /* Glacier Azure (#38bdf8) */
        .accent_mint    = { 74, 222, 128, 255 },   /* Aurora Green (#4ade80) */
        .accent_amber   = { 251, 146, 60, 255 },   /* Polar Sun Coral (#fb923c) */
        .accent_lavender = { 165, 180, 252, 255 }, /* Arctic Lilac (#a5b4fc) */
        .accent_rose    = { 244, 114, 182, 255 },  /* Rose Quartz */
        .text_bright    = { 248, 250, 255, 255 },  /* Crisp Glacial White */
        .text_muted     = { 152, 172, 204, 255 }   /* Polar Slate */
    },
    /* 3: Titanium Graphite (Warm Precision Craft) */
    {
        .id = "titanium",
        .name = "Graphite",
        .bg             = { 24, 25, 29, 255 },     /* Warm graphite floor (#18191d) */
        .surface        = { 34, 36, 44, 255 },     /* Brushed titanium (#22242c) */
        .surface_high   = { 46, 50, 62, 255 },     /* Elevated plate (#2e323e) */
        .sunken         = { 16, 17, 21, 255 },     /* Sunken charcoal (#101115) */
        .shadow_dark    = { 8, 9, 12, 215 },       /* Ambient shadow */
        .shadow_light   = { 80, 88, 110, 115 },    /* Brushed metal sheen */
        .border         = { 48, 54, 68, 255 },     /* Precision boundary */
        .accent_blue    = { 59, 130, 246, 255 },   /* Precision Blue (#3b82f6) */
        .accent_mint    = { 16, 185, 129, 255 },   /* Sage Green (#10b981) */
        .accent_amber   = { 245, 158, 11, 255 },   /* Amber Gold (#f59e0b) */
        .accent_lavender = { 139, 92, 246, 255 },  /* Lavender (#8b5cf6) */
        .accent_rose    = { 239, 68, 68, 255 },    /* Rose Red (#ef4444) */
        .text_bright    = { 250, 250, 252, 255 },  /* Snow White */
        .text_muted     = { 160, 166, 180, 255 }   /* Slate Silver */
    }
};

static int g_current_theme_idx = 0;
const UITheme* g_theme = &THEMES[0];

void theme_init(void) {
    g_current_theme_idx = 0;
    g_theme = &THEMES[0];
}

void theme_set(int index) {
    if (index < 0 || index >= THEME_COUNT) index = 0;
    g_current_theme_idx = index;
    g_theme = &THEMES[index];
}

int theme_get_index(void) {
    return g_current_theme_idx;
}

const UITheme* theme_get_current(void) {
    return g_theme;
}

/* =========================================================================
 * Anti-Aliased Circle & Ring Primitives
 * ========================================================================= */

void ui_draw_filled_circle(SDL_Renderer* r, int cx, int cy, int radius, SDL_Color color) {
    if (radius <= 0) return;
    if (radius <= 2) {
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        SDL_Rect rect = { cx - radius, cy - radius, radius * 2, radius * 2 };
        SDL_RenderFillRect(r, &rect);
        return;
    }

    int N = 64;
    float fcx = (float)cx;
    float fcy = (float)cy;
    float r_core = (float)radius - 0.75f;
    float r_fringe = (float)radius + 0.75f;
    if (r_core < 0.0f) r_core = 0.0f;

    int num_verts = 1 + 2 * N;
    SDL_Vertex* verts = (SDL_Vertex*)malloc(sizeof(SDL_Vertex) * num_verts);
    if (!verts) return;

    verts[0].position.x = fcx;
    verts[0].position.y = fcy;
    verts[0].color = color;

    for (int i = 0; i < N; ++i) {
        float ang = (float)i * (2.0f * (float)M_PI) / (float)N;
        float c = cosf(ang), s = sinf(ang);

        /* Solid core vertex */
        verts[1 + i].position.x = fcx + r_core * c;
        verts[1 + i].position.y = fcy + r_core * s;
        verts[1 + i].color = color;

        /* Feathered outer fringe (alpha = 0) */
        verts[1 + N + i].position.x = fcx + r_fringe * c;
        verts[1 + N + i].position.y = fcy + r_fringe * s;
        verts[1 + N + i].color = (SDL_Color){ color.r, color.g, color.b, 0 };
    }

    int num_indices = 9 * N;
    int* indices = (int*)malloc(sizeof(int) * num_indices);
    if (!indices) { free(verts); return; }

    int idx = 0;
    for (int i = 0; i < N; ++i) {
        int next = (i + 1) % N;
        /* Center solid fan */
        indices[idx++] = 0;
        indices[idx++] = 1 + i;
        indices[idx++] = 1 + next;

        /* Outer anti-aliasing fringe quad */
        indices[idx++] = 1 + i;
        indices[idx++] = 1 + N + i;
        indices[idx++] = 1 + N + next;

        indices[idx++] = 1 + i;
        indices[idx++] = 1 + N + next;
        indices[idx++] = 1 + next;
    }

    SDL_RenderGeometry(r, NULL, verts, num_verts, indices, num_indices);
    free(indices);
    free(verts);
}

void ui_draw_circle(SDL_Renderer* r, int cx, int cy, int radius, int thickness, SDL_Color color) {
    if (radius <= 1) return;
    if (thickness < 1) thickness = 1;
    float fthick = (float)thickness;
    float fcx = (float)cx;
    float fcy = (float)cy;
    float frad = (float)radius;

    int N = 64;
    float half_t = fthick * 0.5f;

    float r_out_fringe = frad + half_t + 0.75f;
    float r_out_core   = frad + half_t - 0.75f;
    float r_in_core    = frad - half_t + 0.75f;
    float r_in_fringe  = frad - half_t - 0.75f;
    if (r_in_fringe < 0.0f) r_in_fringe = 0.0f;
    if (r_in_core < 0.0f) r_in_core = 0.0f;

    int num_verts = (N + 1) * 4;
    SDL_Vertex* verts = (SDL_Vertex*)malloc(sizeof(SDL_Vertex) * num_verts);
    if (!verts) return;

    for (int i = 0; i <= N; ++i) {
        float ang = (float)i * (2.0f * (float)M_PI) / (float)N;
        float c = cosf(ang), s = sinf(ang);

        /* 0: Inner fringe (a=0) */
        verts[i * 4 + 0].position.x = fcx + r_in_fringe * c;
        verts[i * 4 + 0].position.y = fcy + r_in_fringe * s;
        verts[i * 4 + 0].color = (SDL_Color){ color.r, color.g, color.b, 0 };

        /* 1: Inner core */
        verts[i * 4 + 1].position.x = fcx + r_in_core * c;
        verts[i * 4 + 1].position.y = fcy + r_in_core * s;
        verts[i * 4 + 1].color = color;

        /* 2: Outer core */
        verts[i * 4 + 2].position.x = fcx + r_out_core * c;
        verts[i * 4 + 2].position.y = fcy + r_out_core * s;
        verts[i * 4 + 2].color = color;

        /* 3: Outer fringe (a=0) */
        verts[i * 4 + 3].position.x = fcx + r_out_fringe * c;
        verts[i * 4 + 3].position.y = fcy + r_out_fringe * s;
        verts[i * 4 + 3].color = (SDL_Color){ color.r, color.g, color.b, 0 };
    }

    int num_indices = 18 * N;
    int* indices = (int*)malloc(sizeof(int) * num_indices);
    if (!indices) { free(verts); return; }

    int idx = 0;
    for (int i = 0; i < N; ++i) {
        int v0 = i * 4;
        int v1 = (i + 1) * 4;

        indices[idx++] = v0 + 0; indices[idx++] = v0 + 1; indices[idx++] = v1 + 1;
        indices[idx++] = v0 + 0; indices[idx++] = v1 + 1; indices[idx++] = v1 + 0;

        indices[idx++] = v0 + 1; indices[idx++] = v0 + 2; indices[idx++] = v1 + 2;
        indices[idx++] = v0 + 1; indices[idx++] = v1 + 2; indices[idx++] = v1 + 1;

        indices[idx++] = v0 + 2; indices[idx++] = v0 + 3; indices[idx++] = v1 + 3;
        indices[idx++] = v0 + 2; indices[idx++] = v1 + 3; indices[idx++] = v1 + 2;
    }

    SDL_RenderGeometry(r, NULL, verts, num_verts, indices, num_indices);
    free(indices);
    free(verts);
}

/* =========================================================================
 * Anti-Aliased Sector Ring (Highlight Cone / Petal)
 * ========================================================================= */

void ui_draw_sector_ring(SDL_Renderer* r, int cx, int cy, int r_in, int r_out,
                         float start_rad, float end_rad, SDL_Color color) {
    if (r_out <= r_in) return;
    int N = 48;
    float fcx = (float)cx;
    float fcy = (float)cy;

    float r_in_fringe = (float)r_in - 0.75f;
    if (r_in_fringe < 0.0f) r_in_fringe = 0.0f;
    float r_in_core = (float)r_in + 0.75f;
    float r_out_core = (float)r_out - 0.75f;
    float r_out_fringe = (float)r_out + 0.75f;

    int num_verts = (N + 1) * 4;
    SDL_Vertex* verts = (SDL_Vertex*)malloc(sizeof(SDL_Vertex) * num_verts);
    if (!verts) return;

    for (int i = 0; i <= N; ++i) {
        float t = (float)i / (float)N;
        float ang = start_rad + t * (end_rad - start_rad);
        float c = cosf(ang), s = sinf(ang);

        Uint8 alpha = color.a;
        /* Radial edge anti-aliasing feathering */
        if (i == 0 || i == N) {
            alpha = (Uint8)(color.a * 0.35f);
        }

        /* 0: Inner fringe */
        verts[i * 4 + 0].position.x = fcx + r_in_fringe * c;
        verts[i * 4 + 0].position.y = fcy + r_in_fringe * s;
        verts[i * 4 + 0].color = (SDL_Color){ color.r, color.g, color.b, 0 };

        /* 1: Inner core */
        verts[i * 4 + 1].position.x = fcx + r_in_core * c;
        verts[i * 4 + 1].position.y = fcy + r_in_core * s;
        verts[i * 4 + 1].color = (SDL_Color){ color.r, color.g, color.b, alpha };

        /* 2: Outer core */
        verts[i * 4 + 2].position.x = fcx + r_out_core * c;
        verts[i * 4 + 2].position.y = fcy + r_out_core * s;
        verts[i * 4 + 2].color = (SDL_Color){ color.r, color.g, color.b, alpha };

        /* 3: Outer fringe */
        verts[i * 4 + 3].position.x = fcx + r_out_fringe * c;
        verts[i * 4 + 3].position.y = fcy + r_out_fringe * s;
        verts[i * 4 + 3].color = (SDL_Color){ color.r, color.g, color.b, 0 };
    }

    int num_indices = 18 * N;
    int* indices = (int*)malloc(sizeof(int) * num_indices);
    if (!indices) { free(verts); return; }

    int idx = 0;
    for (int i = 0; i < N; ++i) {
        int v0 = i * 4;
        int v1 = (i + 1) * 4;

        /* Band 1: Inner fringe to core */
        indices[idx++] = v0 + 0; indices[idx++] = v0 + 1; indices[idx++] = v1 + 1;
        indices[idx++] = v0 + 0; indices[idx++] = v1 + 1; indices[idx++] = v1 + 0;

        /* Band 2: Solid core body */
        indices[idx++] = v0 + 1; indices[idx++] = v0 + 2; indices[idx++] = v1 + 2;
        indices[idx++] = v0 + 1; indices[idx++] = v1 + 2; indices[idx++] = v1 + 1;

        /* Band 3: Core to outer fringe */
        indices[idx++] = v0 + 2; indices[idx++] = v0 + 3; indices[idx++] = v1 + 3;
        indices[idx++] = v0 + 2; indices[idx++] = v1 + 3; indices[idx++] = v1 + 2;
    }

    SDL_RenderGeometry(r, NULL, verts, num_verts, indices, num_indices);
    free(indices);
    free(verts);
}

/* =========================================================================
 * Anti-Aliased Rounded Rectangle & Lines
 * ========================================================================= */

static void draw_aa_corner_fan(SDL_Renderer* r, float cx, float cy, float radius,
                               float start_ang, float end_ang, SDL_Color color) {
    if (radius <= 0.5f) return;
    int S = 14;
    float r_core = radius - 0.75f;
    float r_fringe = radius + 0.75f;
    if (r_core < 0.0f) r_core = 0.0f;

    int num_verts = 1 + 2 * (S + 1);
    SDL_Vertex* verts = (SDL_Vertex*)malloc(sizeof(SDL_Vertex) * num_verts);
    if (!verts) return;

    verts[0].position.x = cx;
    verts[0].position.y = cy;
    verts[0].color = color;

    for (int i = 0; i <= S; ++i) {
        float t = (float)i / (float)S;
        float ang = start_ang + t * (end_ang - start_ang);
        float c = cosf(ang), s = sinf(ang);

        verts[1 + i].position.x = cx + r_core * c;
        verts[1 + i].position.y = cy + r_core * s;
        verts[1 + i].color = color;

        verts[1 + (S + 1) + i].position.x = cx + r_fringe * c;
        verts[1 + (S + 1) + i].position.y = cy + r_fringe * s;
        verts[1 + (S + 1) + i].color = (SDL_Color){ color.r, color.g, color.b, 0 };
    }

    int num_indices = 9 * S;
    int* indices = (int*)malloc(sizeof(int) * num_indices);
    if (!indices) { free(verts); return; }

    int idx = 0;
    for (int i = 0; i < S; ++i) {
        indices[idx++] = 0;
        indices[idx++] = 1 + i;
        indices[idx++] = 1 + (i + 1);

        indices[idx++] = 1 + i;
        indices[idx++] = 1 + (S + 1) + i;
        indices[idx++] = 1 + (S + 1) + (i + 1);

        indices[idx++] = 1 + i;
        indices[idx++] = 1 + (S + 1) + (i + 1);
        indices[idx++] = 1 + (i + 1);
    }

    SDL_RenderGeometry(r, NULL, verts, num_verts, indices, num_indices);
    free(indices);
    free(verts);
}

void ui_draw_rounded_rect(SDL_Renderer* r, int x, int y, int w, int h, int radius,
                          SDL_Color color, bool filled) {
    if (w <= 0 || h <= 0) return;

    if (radius <= 0) {
        SDL_Rect rect = { x, y, w, h };
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);
        if (filled) SDL_RenderFillRect(r, &rect);
        else SDL_RenderDrawRect(r, &rect);
        return;
    }

    if (radius > w / 2) radius = w / 2;
    if (radius > h / 2) radius = h / 2;

    if (filled) {
        /* Flat central cross */
        SDL_SetRenderDrawColor(r, color.r, color.g, color.b, color.a);

        SDL_Rect mid_v = { x + radius, y, w - 2 * radius, h };
        SDL_RenderFillRect(r, &mid_v);

        SDL_Rect left_v = { x, y + radius, radius, h - 2 * radius };
        SDL_RenderFillRect(r, &left_v);

        SDL_Rect right_v = { x + w - radius, y + radius, radius, h - 2 * radius };
        SDL_RenderFillRect(r, &right_v);

        /* 4 anti-aliased corner quadrant fans */
        float fr = (float)radius;
        draw_aa_corner_fan(r, (float)(x + radius), (float)(y + radius), fr, (float)M_PI, (float)M_PI * 1.5f, color);
        draw_aa_corner_fan(r, (float)(x + w - radius), (float)(y + radius), fr, (float)M_PI * 1.5f, (float)M_PI * 2.0f, color);
        draw_aa_corner_fan(r, (float)(x + w - radius), (float)(y + h - radius), fr, 0.0f, (float)M_PI * 0.5f, color);
        draw_aa_corner_fan(r, (float)(x + radius), (float)(y + h - radius), fr, (float)M_PI * 0.5f, (float)M_PI, color);
    } else {
        /* Smooth anti-aliased border lines */
        ui_draw_thick_line(r, x + radius, y, x + w - radius, y, 1, color);
        ui_draw_thick_line(r, x + radius, y + h - 1, x + w - radius, y + h - 1, 1, color);
        ui_draw_thick_line(r, x, y + radius, x, y + h - radius, 1, color);
        ui_draw_thick_line(r, x + w - 1, y + radius, x + w - 1, y + h - radius, 1, color);

        /* 4 corner arcs */
        ui_draw_sector_ring(r, x + radius, y + radius, radius - 1, radius + 1, (float)M_PI, (float)M_PI * 1.5f, color);
        ui_draw_sector_ring(r, x + w - radius, y + radius, radius - 1, radius + 1, (float)M_PI * 1.5f, (float)M_PI * 2.0f, color);
        ui_draw_sector_ring(r, x + w - radius, y + h - radius, radius - 1, radius + 1, 0.0f, (float)M_PI * 0.5f, color);
        ui_draw_sector_ring(r, x + radius, y + h - radius, radius - 1, radius + 1, (float)M_PI * 0.5f, (float)M_PI, color);
    }
}

void ui_draw_thick_line(SDL_Renderer* r, int x1, int y1, int x2, int y2, int thickness, SDL_Color color) {
    if (thickness < 1) thickness = 1;
    float dx = (float)(x2 - x1);
    float dy = (float)(y2 - y1);
    float len = sqrtf(dx * dx + dy * dy);
    if (len < 0.001f) return;

    float nx = -dy / len;
    float ny =  dx / len;

    float w_core = (float)thickness * 0.5f - 0.75f;
    if (w_core < 0.0f) w_core = 0.0f;
    float w_fringe = (float)thickness * 0.5f + 0.75f;

    /* 8 vertices: 2 core top, 2 core bottom, 2 outer fringe top/bottom */
    SDL_Vertex verts[8];
    float fx1 = (float)x1, fy1 = (float)y1;
    float fx2 = (float)x2, fy2 = (float)y2;

    /* Negative fringe */
    verts[0].position.x = fx1 - nx * w_fringe; verts[0].position.y = fy1 - ny * w_fringe;
    verts[0].color = (SDL_Color){ color.r, color.g, color.b, 0 };
    verts[1].position.x = fx2 - nx * w_fringe; verts[1].position.y = fy2 - ny * w_fringe;
    verts[1].color = (SDL_Color){ color.r, color.g, color.b, 0 };

    /* Negative core */
    verts[2].position.x = fx1 - nx * w_core; verts[2].position.y = fy1 - ny * w_core;
    verts[2].color = color;
    verts[3].position.x = fx2 - nx * w_core; verts[3].position.y = fy2 - ny * w_core;
    verts[3].color = color;

    /* Positive core */
    verts[4].position.x = fx1 + nx * w_core; verts[4].position.y = fy1 + ny * w_core;
    verts[4].color = color;
    verts[5].position.x = fx2 + nx * w_core; verts[5].position.y = fy2 + ny * w_core;
    verts[5].color = color;

    /* Positive fringe */
    verts[6].position.x = fx1 + nx * w_fringe; verts[6].position.y = fy1 + ny * w_fringe;
    verts[6].color = (SDL_Color){ color.r, color.g, color.b, 0 };
    verts[7].position.x = fx2 + nx * w_fringe; verts[7].position.y = fy2 + ny * w_fringe;
    verts[7].color = (SDL_Color){ color.r, color.g, color.b, 0 };

    int indices[18] = {
        0, 1, 3, 0, 3, 2,  /* Left fringe */
        2, 3, 5, 2, 5, 4,  /* Solid core */
        4, 5, 7, 4, 7, 6   /* Right fringe */
    };

    SDL_RenderGeometry(r, NULL, verts, 8, indices, 18);
}

/* =========================================================================
 * Neumorphic Soft UI Primitives
 * ========================================================================= */

void ui_draw_neu_panel(SDL_Renderer* r, int x, int y, int w, int h, int radius,
                       bool inset, bool glow_active, SDL_Color glow_col) {
    SDL_Color fill = inset ? NEU_SUNKEN : NEU_SURFACE;
    if (glow_active) fill = NEU_SURFACE_HIGH;
    ui_draw_rounded_rect(r, x, y, w, h, radius, glow_active ? glow_col : NEU_BORDER, true);
    ui_draw_rounded_rect(r, x+1, y+1, w-2, h-2, radius > 0 ? radius-1 : 0, fill, true);
}
void ui_draw_neu_circle(SDL_Renderer* r, int cx, int cy, int radius,
                        bool inset, bool glow_active, SDL_Color glow_col) {
    ui_draw_filled_circle(r, cx, cy, radius, inset ? NEU_SUNKEN : NEU_SURFACE);
    ui_draw_circle(r, cx, cy, radius, glow_active ? 2 : 1, glow_active ? glow_col : NEU_BORDER);
}

void ui_draw_neu_well_circle(SDL_Renderer* r, int cx, int cy, int radius) {
    ui_draw_neu_circle(r, cx, cy, radius, true, false, (SDL_Color){0,0,0,0});
    /* Smooth Concentric Grooves for concave depth */
    if (radius > 16) {
        ui_draw_circle(r, cx, cy, (radius * 75) / 100, 1, (SDL_Color){ 20, 22, 28, 180 });
        ui_draw_circle(r, cx, cy, (radius * 50) / 100, 1, (SDL_Color){ 18, 20, 25, 180 });
    }
}

void ui_draw_glowing_ring(SDL_Renderer* r, int cx, int cy, int radius, int thickness, SDL_Color color) {
    ui_draw_circle(r, cx, cy, radius + thickness + 1, 1, (SDL_Color){ color.r, color.g, color.b, 50 });
    ui_draw_circle(r, cx, cy, radius + thickness, 1, (SDL_Color){ color.r, color.g, color.b, 130 });
    ui_draw_circle(r, cx, cy, radius, thickness, color);
    if (radius > thickness + 1) {
        ui_draw_circle(r, cx, cy, radius - thickness, 1, (SDL_Color){ color.r, color.g, color.b, 80 });
    }
}

void ui_draw_led_indicator(SDL_Renderer* r, int cx, int cy, int radius, bool on, SDL_Color color) {
    /* Recessed Socket */
    ui_draw_filled_circle(r, cx, cy, radius + 3, NEU_SUNKEN);
    ui_draw_circle(r, cx, cy, radius + 3, 1, NEU_SHADOW_DARK);

    if (on) {
        /* Multi-pass smooth radial glow */
        ui_draw_filled_circle(r, cx, cy, radius + 4, (SDL_Color){ color.r, color.g, color.b, 45 });
        ui_draw_filled_circle(r, cx, cy, radius + 2, (SDL_Color){ color.r, color.g, color.b, 120 });
        ui_draw_filled_circle(r, cx, cy, radius, color);

        /* Specular pinpoint reflection */
        int pip_r = radius / 3;
        if (pip_r < 1) pip_r = 1;
        ui_draw_filled_circle(r, cx - radius / 3, cy - radius / 3, pip_r, (SDL_Color){ 255, 255, 255, 230 });
    } else {
        /* Dim unlit gemstone */
        ui_draw_filled_circle(r, cx, cy, radius, (SDL_Color){ (Uint8)(color.r / 6), (Uint8)(color.g / 6), (Uint8)(color.b / 6), 255 });
        ui_draw_circle(r, cx, cy, radius, 1, (SDL_Color){ 10, 11, 15, 200 });
        int pip_r = radius / 3;
        if (pip_r < 1) pip_r = 1;
        ui_draw_filled_circle(r, cx - radius / 3, cy - radius / 3, pip_r, (SDL_Color){ 80, 90, 110, 90 });
    }
}
