#include <SDL2/SDL.h>
#include "logging.h"

static float screen_orientation=0.0f;
static int screen_width, screen_height;

const char* const flip_strings[3] = {
    "None",
    "Horizontal",
    "Vertical"
};

//void (*translate_xy)(int* x, int* y);
void (*translate_point)(SDL_Point* pt);
void (*translate_image_rect)(SDL_Rect* rect);
void (*translate_draw_rect)(SDL_Rect* rect);
void (*translate_screen_rect)(SDL_Rect* rect);

void copyRect(const SDL_Rect *src, SDL_Rect *dst) {
    dst->x = src->x;
    dst->y = src->y;
    dst->w = src->w;
    dst->h = src->h;
}

void copyPoint(const SDL_Point *src, SDL_Point *dst) {
    dst->x = src->x;
    dst->y = src->y;
}

static const SDL_RendererFlip flip0_180[4] = {
    SDL_FLIP_NONE,
    SDL_FLIP_HORIZONTAL,
    SDL_FLIP_VERTICAL,
    SDL_FLIP_HORIZONTAL|SDL_FLIP_VERTICAL
};

/*
static const SDL_RendererFlip flip90_270[3] = {
    SDL_FLIP_NONE,
    SDL_FLIP_VERTICAL,
    SDL_FLIP_HORIZONTAL
};
*/

const SDL_RendererFlip* oriented_flip = flip0_180;

static void xlate_point_0(SDL_Point* pt) {}

static void xlate_point_180(SDL_Point* pt) {
    pt->x = screen_width - pt->x;
    pt->y = screen_height - pt->y;
}

static void xlate_point_90(SDL_Point* pt) {
    int x =  pt->x;
    int y =  pt->y;
    pt->x = y;
    pt->y = screen_width - x;
}

static void xlate_point_270(SDL_Point* pt) {
    int x = pt->x;
    int y = pt->y;
    pt->x = screen_height - y;
    pt->y = x;
}


static void xlate_centered_dest_rect_0(SDL_Rect* rect) {}

static void xlate_centered_dest_rect_180(SDL_Rect* rect) {
    rect->x = screen_width - rect->x - rect->w;
    rect->y = screen_height - rect->y - rect->h;
}

static void xlate_centered_dest_rect_90(SDL_Rect* rect) {
    int x =  rect->x;
    int y =  rect->y;
    rect->x = screen_width - rect->h - y + ((rect->h - rect->w)/2);
    rect->y = x + ((rect->w - rect->h)/2);
}

static void xlate_centered_dest_rect_270(SDL_Rect* rect) {
    int x =  rect->x;
    int y =  rect->y;
    rect->x = y + ((rect->h - rect->w)/2);
    rect->y = screen_height - rect->w -x + ((rect->w - rect->h)/2);
}

static void xlate_draw_rect_0(SDL_Rect* rect) {}
/*
static void xlate_draw_rect_180(SDL_Rect* rect) {
    rect->x = screen_width - rect->x - rect->w;
    rect->y = screen_height - rect->y - rect->h;
}

static void xlate_draw_rect_90(SDL_Rect* rect) {
    int x =  rect->x;
    int y =  rect->y;
    int w =  rect->w;
    int h =  rect->h;
    rect->x = screen_width - h - y;
    rect->y = x;
    rect->w = h;
    rect->h = w;
}

static void xlate_draw_rect_270(SDL_Rect* rect) {
    int x =  rect->x;
    int y =  rect->y;
    int w =  rect->w;
    int h =  rect->h;
    rect->x = y;
    rect->y = screen_height - x - w;
    rect->w = h;
    rect->h = w;
}
*/

//static void xlate_xy_0(int* px, int *py) {}
//static void xlate_xy_180(int* px, int *py) {
//        *px = screen_width - (*px);
//        *py = screen_height - (*py);
//}
//static void xlate_xy_90(int* px, int *py) {
//        int x =  *px;
//        int y =  *py;
//        *px = y;
//        *py = screen_width - x;
//}
//static void xlate_xy_270(int* px, int *py) {
//        int x = *px;
//        int y =  *py;
//        *px = screen_height - y;
//        *py = x;
//}

void setup_orientation(float orientation, int w, int h, SDL_Rect *screen) {
    printf("setup_orientation: orientation:%f, w=%d h=%d, screen={x=%d, y=%d, w=%d, h=%d)\n",
            orientation, w, h, screen->x, screen->y, screen->w, screen->h);
    screen_orientation = orientation;
    screen_width = w;
    screen_height = h;
    screen->w = w;
    screen->h = h;

//    translate_xy = xlate_xy_0;
    translate_point = xlate_point_0;
    translate_screen_rect = xlate_centered_dest_rect_0;
    translate_image_rect = xlate_centered_dest_rect_0;
    translate_draw_rect = xlate_draw_rect_0;

    if (orientation == 90) {
//        translate_xy = xlate_xy_90;
        translate_point = xlate_point_90;
        translate_screen_rect = xlate_centered_dest_rect_90;
//        translate_image_rect = xlate_centered_dest_rect_90;
//        translate_draw_rect = xlate_draw_rect_90;
        screen->w = h;
        screen->h = w;
//        oriented_flip = flip90_270;
    }

    if (orientation == 180) {
//        translate_xy = xlate_xy_180;
        translate_point = xlate_point_180;
        translate_screen_rect = xlate_centered_dest_rect_180;
//        translate_image_rect = xlate_centered_dest_rect_180;
//        translate_draw_rect = xlate_draw_rect_180;
    }

    if (orientation == 270) {
//        translate_xy = xlate_xy_270;
        translate_point = xlate_point_270;
        translate_screen_rect = xlate_centered_dest_rect_270;
//        translate_image_rect = xlate_centered_dest_rect_270;
//        translate_draw_rect = xlate_draw_rect_270;
        screen->w = h;
        screen->h = w;
//        oriented_flip = flip90_270;
    }

//    printf("    flip:\n");
//    for(int ix=0; ix < 3; ++ix) {
//        printf("        %d => %s\n", ix, flip_strings[oriented_flip[ix]]);
//    }
}

// src and dst can be the same rect
void rebaseRect(const SDL_Rect* origin, const SDL_Rect* src, SDL_Rect* dst) {
    dst->x = origin->x + src->x;
    dst->y = origin->y + src->y;
    dst->w = src->w;
    dst->h = src->h;
}

void rebasePoint(const SDL_Rect* origin, const SDL_Point* src, SDL_Point* dst) {
    dst->x = origin->x + src->x;
    dst->y = origin->y + src->y;
}

// src and dst can be the same rect
void offset_rect(const SDL_Point* offset, const SDL_Rect* src, SDL_Rect* dst) {
    dst->x = offset->x + src->x;
    dst->y = offset->y + src->y;
    dst->w = src->w;
    dst->h = src->h;
}

void offset_point(const SDL_Rect* offset, const SDL_Point* src, SDL_Point* dst) {
    dst->x = offset->x + src->x;
    dst->y = offset->y + src->y;
}

void translate_axle(const SDL_Rect* enclosure, const SDL_Point* axle, SDL_Rect* rect) {
    SDL_Rect u_enclosure;
    SDL_Point u_axle;
    copyRect(enclosure, &u_enclosure);
    copyPoint(axle, &u_axle);
/*    
    int dx = axle->x - enclosure->x;
    int dy = axle->y - enclosure->y;

    if (screen_orientation == 90) {
        translate_draw_rect(&u_enclosure);
        axle->x = u_enclosure.x + u_enclosure.w - dy;
        axle->y = u_enclosure.y + u_enclosure.h - dx;
    }
    if (screen_orientation == 180) {
        translate_draw_rect(&u_enclosure);
        axle->x = u_enclosure.x + u_enclosure.w - dx;
        axle->y = u_enclosure.y + u_enclosure.h - dy;
    }
    if (screen_orientation == 270) {
        translate_draw_rect(&u_enclosure);
        axle->x = u_enclosure.x + dy;
        axle->y = u_enclosure.y + u_enclosure.h - dx;
    }
*/    
//    debug_printf("u_axle = (%d,%d) axle=(%d,%d)\n", u_axle.x, u_axle.y, axle->x, axle->y);
//    debug_printf(" delta = (%d,%d)\n", axle->x - u_axle.x, axle->y - u_axle.y);
    rect->x += axle->x - u_axle.x;
    rect->y += axle->y - u_axle.y;
//    printf("   new = (%d,%d)\n", rect->x, rect->y);
    debug_printf("translate axle:\n      input:\n\tenclosure=(%d,%d,%d,%d),\n\taxle=(%d,%d)\n", 
            u_enclosure.x, u_enclosure.y, u_enclosure.w, u_enclosure.h,
            u_axle.x, u_axle.y);
    debug_printf("     output:\n\taxle=(%d,%d)\n", 
            axle->x, axle->y);
}

/*
void translate_image_rect_in_rect(SDL_Rect* container, SDL_Rect* rect) {
    if (screen_orientation == 0) {
        rect->x += container->x;
        rect->y += container->y;
    }
    if (screen_orientation == 90) {
        int x =  rect->x + container->x;
        int y =  rect->y + container->y;
        rect->x = container->w - rect->h - y + ((rect->h - rect->w)/2);
        rect->y = x + ((rect->w - rect->h)/2);
    }
    if (screen_orientation == 180) {
        rect->x = container->x + container->w - rect->x - rect->w;
        rect->y = container->y + container->h - rect->y - rect->h;
    }
    if (screen_orientation == 270) {
        int x =  rect->x + container->x;
        int y =  rect->y + container->y;
        rect->x = y + ((rect->h - rect->w)/2);
        rect->y = container->h - rect->w -x + ((rect->w - rect->h)/2);
   }
}
*/

void center_rect(const SDL_Rect* outer, const SDL_Rect* inner, SDL_Rect* dst) {
    int dx = (outer->w - inner->w)/2;
    int dy = (outer->h - inner->h)/2;
    dst->x = outer->x + dx;
    dst->y = outer->y + dy;
    dst->w = inner->w;
    dst->h = inner->h;
}

void scale_rect_size(const SDL_Rect* src, SDL_Rect* dst, float scalef) {
    dst->x = src->x;
    dst->y = src->y;
    dst->w = (int)(src->w * scalef + 0.5);
    dst->h = (int)(src->h * scalef + 0.5);
}

void scale_rect(const SDL_Rect* src, SDL_Rect* dst, float scalef) {
    dst->x = (int)(src->x * scalef + 0.5);
    dst->y = (int)(src->y * scalef + 0.5);
    dst->w = (int)(src->w * scalef + 0.5);
    dst->h = (int)(src->h * scalef + 0.5);
}

void print_sdl_key_scancode(SDL_Scancode scancode) {
    switch(scancode) {
        case SDL_SCANCODE_UNKNOWN:
            printf("SDL_SCANCODE_UNKNOWN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_A:
            printf("SDL_SCANCODE_A %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_B:
            printf("SDL_SCANCODE_B %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_C:
            printf("SDL_SCANCODE_C %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_D:
            printf("SDL_SCANCODE_D %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_E:
            printf("SDL_SCANCODE_E %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F:
            printf("SDL_SCANCODE_F %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_G:
            printf("SDL_SCANCODE_G %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_H:
            printf("SDL_SCANCODE_H %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_I:
            printf("SDL_SCANCODE_I %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_J:
            printf("SDL_SCANCODE_J %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_K:
            printf("SDL_SCANCODE_K %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_L:
            printf("SDL_SCANCODE_L %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_M:
            printf("SDL_SCANCODE_M %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_N:
            printf("SDL_SCANCODE_N %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_O:
            printf("SDL_SCANCODE_O %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_P:
            printf("SDL_SCANCODE_P %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_Q:
            printf("SDL_SCANCODE_Q %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_R:
            printf("SDL_SCANCODE_R %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_S:
            printf("SDL_SCANCODE_S %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_T:
            printf("SDL_SCANCODE_T %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_U:
            printf("SDL_SCANCODE_U %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_V:
            printf("SDL_SCANCODE_V %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_W:
            printf("SDL_SCANCODE_W %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_X:
            printf("SDL_SCANCODE_X %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_Y:
            printf("SDL_SCANCODE_Y %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_Z:
            printf("SDL_SCANCODE_Z %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_1:
            printf("SDL_SCANCODE_1 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_2:
            printf("SDL_SCANCODE_2 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_3:
            printf("SDL_SCANCODE_3 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_4:
            printf("SDL_SCANCODE_4 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_5:
            printf("SDL_SCANCODE_5 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_6:
            printf("SDL_SCANCODE_6 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_7:
            printf("SDL_SCANCODE_7 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_8:
            printf("SDL_SCANCODE_8 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_9:
            printf("SDL_SCANCODE_9 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_0:
            printf("SDL_SCANCODE_0 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_RETURN:
            printf("SDL_SCANCODE_RETURN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_ESCAPE:
            printf("SDL_SCANCODE_ESCAPE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_BACKSPACE:
            printf("SDL_SCANCODE_BACKSPACE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_TAB:
            printf("SDL_SCANCODE_TAB %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SPACE:
            printf("SDL_SCANCODE_SPACE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_MINUS:
            printf("SDL_SCANCODE_MINUS %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_EQUALS:
            printf("SDL_SCANCODE_EQUALS %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LEFTBRACKET:
            printf("SDL_SCANCODE_LEFTBRACKET %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_RIGHTBRACKET:
            printf("SDL_SCANCODE_RIGHTBRACKET %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_BACKSLASH:
            printf("SDL_SCANCODE_BACKSLASH %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_NONUSHASH:
            printf("SDL_SCANCODE_NONUSHASH %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SEMICOLON:
            printf("SDL_SCANCODE_SEMICOLON %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_APOSTROPHE:
            printf("SDL_SCANCODE_APOSTROPHE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_GRAVE:
            printf("SDL_SCANCODE_GRAVE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_COMMA:
            printf("SDL_SCANCODE_COMMA %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_PERIOD:
            printf("SDL_SCANCODE_PERIOD %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SLASH:
            printf("SDL_SCANCODE_SLASH %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CAPSLOCK:
            printf("SDL_SCANCODE_CAPSLOCK %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F1:
            printf("SDL_SCANCODE_F1 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F2:
            printf("SDL_SCANCODE_F2 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F3:
            printf("SDL_SCANCODE_F3 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F4:
            printf("SDL_SCANCODE_F4 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F5:
            printf("SDL_SCANCODE_F5 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F6:
            printf("SDL_SCANCODE_F6 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F7:
            printf("SDL_SCANCODE_F7 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F8:
            printf("SDL_SCANCODE_F8 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F9:
            printf("SDL_SCANCODE_F9 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F10:
            printf("SDL_SCANCODE_F10 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F11:
            printf("SDL_SCANCODE_F11 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F12:
            printf("SDL_SCANCODE_F12 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_PRINTSCREEN:
            printf("SDL_SCANCODE_PRINTSCREEN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SCROLLLOCK:
            printf("SDL_SCANCODE_SCROLLLOCK %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_PAUSE:
            printf("SDL_SCANCODE_PAUSE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INSERT:
            printf("SDL_SCANCODE_INSERT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_HOME:
            printf("SDL_SCANCODE_HOME %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_PAGEUP:
            printf("SDL_SCANCODE_PAGEUP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_DELETE:
            printf("SDL_SCANCODE_DELETE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_END:
            printf("SDL_SCANCODE_END %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_PAGEDOWN:
            printf("SDL_SCANCODE_PAGEDOWN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_RIGHT:
            printf("SDL_SCANCODE_RIGHT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LEFT:
            printf("SDL_SCANCODE_LEFT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_DOWN:
            printf("SDL_SCANCODE_DOWN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_UP:
            printf("SDL_SCANCODE_UP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_NUMLOCKCLEAR:
            printf("SDL_SCANCODE_NUMLOCKCLEAR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_DIVIDE:
            printf("SDL_SCANCODE_KP_DIVIDE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_MULTIPLY:
            printf("SDL_SCANCODE_KP_MULTIPLY %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_MINUS:
            printf("SDL_SCANCODE_KP_MINUS %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_PLUS:
            printf("SDL_SCANCODE_KP_PLUS %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_ENTER:
            printf("SDL_SCANCODE_KP_ENTER %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_1:
            printf("SDL_SCANCODE_KP_1 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_2:
            printf("SDL_SCANCODE_KP_2 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_3:
            printf("SDL_SCANCODE_KP_3 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_4:
            printf("SDL_SCANCODE_KP_4 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_5:
            printf("SDL_SCANCODE_KP_5 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_6:
            printf("SDL_SCANCODE_KP_6 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_7:
            printf("SDL_SCANCODE_KP_7 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_8:
            printf("SDL_SCANCODE_KP_8 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_9:
            printf("SDL_SCANCODE_KP_9 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_0:
            printf("SDL_SCANCODE_KP_0 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_PERIOD:
            printf("SDL_SCANCODE_KP_PERIOD %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_NONUSBACKSLASH:
            printf("SDL_SCANCODE_NONUSBACKSLASH %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_APPLICATION:
            printf("SDL_SCANCODE_APPLICATION %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_POWER:
            printf("SDL_SCANCODE_POWER %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_EQUALS:
            printf("SDL_SCANCODE_KP_EQUALS %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F13:
            printf("SDL_SCANCODE_F13 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F14:
            printf("SDL_SCANCODE_F14 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F15:
            printf("SDL_SCANCODE_F15 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F16:
            printf("SDL_SCANCODE_F16 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F17:
            printf("SDL_SCANCODE_F17 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F18:
            printf("SDL_SCANCODE_F18 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F19:
            printf("SDL_SCANCODE_F19 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F20:
            printf("SDL_SCANCODE_F20 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F21:
            printf("SDL_SCANCODE_F21 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F22:
            printf("SDL_SCANCODE_F22 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F23:
            printf("SDL_SCANCODE_F23 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_F24:
            printf("SDL_SCANCODE_F24 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_EXECUTE:
            printf("SDL_SCANCODE_EXECUTE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_HELP:
            printf("SDL_SCANCODE_HELP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_MENU:
            printf("SDL_SCANCODE_MENU %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SELECT:
            printf("SDL_SCANCODE_SELECT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_STOP:
            printf("SDL_SCANCODE_STOP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AGAIN:
            printf("SDL_SCANCODE_AGAIN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_UNDO:
            printf("SDL_SCANCODE_UNDO %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CUT:
            printf("SDL_SCANCODE_CUT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_COPY:
            printf("SDL_SCANCODE_COPY %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_PASTE:
            printf("SDL_SCANCODE_PASTE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_FIND:
            printf("SDL_SCANCODE_FIND %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_MUTE:
            printf("SDL_SCANCODE_MUTE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_VOLUMEUP:
            printf("SDL_SCANCODE_VOLUMEUP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_VOLUMEDOWN:
            printf("SDL_SCANCODE_VOLUMEDOWN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_COMMA:
            printf("SDL_SCANCODE_KP_COMMA %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_EQUALSAS400:
            printf("SDL_SCANCODE_KP_EQUALSAS400 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INTERNATIONAL1:
            printf("SDL_SCANCODE_INTERNATIONAL1 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INTERNATIONAL2:
            printf("SDL_SCANCODE_INTERNATIONAL2 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INTERNATIONAL3:
            printf("SDL_SCANCODE_INTERNATIONAL3 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INTERNATIONAL4:
            printf("SDL_SCANCODE_INTERNATIONAL4 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INTERNATIONAL5:
            printf("SDL_SCANCODE_INTERNATIONAL5 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INTERNATIONAL6:
            printf("SDL_SCANCODE_INTERNATIONAL6 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INTERNATIONAL7:
            printf("SDL_SCANCODE_INTERNATIONAL7 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INTERNATIONAL8:
            printf("SDL_SCANCODE_INTERNATIONAL8 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_INTERNATIONAL9:
            printf("SDL_SCANCODE_INTERNATIONAL9 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LANG1:
            printf("SDL_SCANCODE_LANG1 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LANG2:
            printf("SDL_SCANCODE_LANG2 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LANG3:
            printf("SDL_SCANCODE_LANG3 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LANG4:
            printf("SDL_SCANCODE_LANG4 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LANG5:
            printf("SDL_SCANCODE_LANG5 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LANG6:
            printf("SDL_SCANCODE_LANG6 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LANG7:
            printf("SDL_SCANCODE_LANG7 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LANG8:
            printf("SDL_SCANCODE_LANG8 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LANG9:
            printf("SDL_SCANCODE_LANG9 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_ALTERASE:
            printf("SDL_SCANCODE_ALTERASE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SYSREQ:
            printf("SDL_SCANCODE_SYSREQ %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CANCEL:
            printf("SDL_SCANCODE_CANCEL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CLEAR:
            printf("SDL_SCANCODE_CLEAR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_PRIOR:
            printf("SDL_SCANCODE_PRIOR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_RETURN2:
            printf("SDL_SCANCODE_RETURN2 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SEPARATOR:
            printf("SDL_SCANCODE_SEPARATOR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_OUT:
            printf("SDL_SCANCODE_OUT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_OPER:
            printf("SDL_SCANCODE_OPER %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CLEARAGAIN:
            printf("SDL_SCANCODE_CLEARAGAIN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CRSEL:
            printf("SDL_SCANCODE_CRSEL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_EXSEL:
            printf("SDL_SCANCODE_EXSEL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_00:
            printf("SDL_SCANCODE_KP_00 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_000:
            printf("SDL_SCANCODE_KP_000 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_THOUSANDSSEPARATOR:
            printf("SDL_SCANCODE_THOUSANDSSEPARATOR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_DECIMALSEPARATOR:
            printf("SDL_SCANCODE_DECIMALSEPARATOR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CURRENCYUNIT:
            printf("SDL_SCANCODE_CURRENCYUNIT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CURRENCYSUBUNIT:
            printf("SDL_SCANCODE_CURRENCYSUBUNIT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_LEFTPAREN:
            printf("SDL_SCANCODE_KP_LEFTPAREN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_RIGHTPAREN:
            printf("SDL_SCANCODE_KP_RIGHTPAREN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_LEFTBRACE:
            printf("SDL_SCANCODE_KP_LEFTBRACE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_RIGHTBRACE:
            printf("SDL_SCANCODE_KP_RIGHTBRACE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_TAB:
            printf("SDL_SCANCODE_KP_TAB %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_BACKSPACE:
            printf("SDL_SCANCODE_KP_BACKSPACE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_A:
            printf("SDL_SCANCODE_KP_A %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_B:
            printf("SDL_SCANCODE_KP_B %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_C:
            printf("SDL_SCANCODE_KP_C %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_D:
            printf("SDL_SCANCODE_KP_D %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_E:
            printf("SDL_SCANCODE_KP_E %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_F:
            printf("SDL_SCANCODE_KP_F %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_XOR:
            printf("SDL_SCANCODE_KP_XOR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_POWER:
            printf("SDL_SCANCODE_KP_POWER %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_PERCENT:
            printf("SDL_SCANCODE_KP_PERCENT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_LESS:
            printf("SDL_SCANCODE_KP_LESS %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_GREATER:
            printf("SDL_SCANCODE_KP_GREATER %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_AMPERSAND:
            printf("SDL_SCANCODE_KP_AMPERSAND %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_DBLAMPERSAND:
            printf("SDL_SCANCODE_KP_DBLAMPERSAND %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_VERTICALBAR:
            printf("SDL_SCANCODE_KP_VERTICALBAR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_DBLVERTICALBAR:
            printf("SDL_SCANCODE_KP_DBLVERTICALBAR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_COLON:
            printf("SDL_SCANCODE_KP_COLON %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_HASH:
            printf("SDL_SCANCODE_KP_HASH %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_SPACE:
            printf("SDL_SCANCODE_KP_SPACE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_AT:
            printf("SDL_SCANCODE_KP_AT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_EXCLAM:
            printf("SDL_SCANCODE_KP_EXCLAM %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_MEMSTORE:
            printf("SDL_SCANCODE_KP_MEMSTORE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_MEMRECALL:
            printf("SDL_SCANCODE_KP_MEMRECALL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_MEMCLEAR:
            printf("SDL_SCANCODE_KP_MEMCLEAR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_MEMADD:
            printf("SDL_SCANCODE_KP_MEMADD %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_MEMSUBTRACT:
            printf("SDL_SCANCODE_KP_MEMSUBTRACT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_MEMMULTIPLY:
            printf("SDL_SCANCODE_KP_MEMMULTIPLY %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_MEMDIVIDE:
            printf("SDL_SCANCODE_KP_MEMDIVIDE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_PLUSMINUS:
            printf("SDL_SCANCODE_KP_PLUSMINUS %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_CLEAR:
            printf("SDL_SCANCODE_KP_CLEAR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_CLEARENTRY:
            printf("SDL_SCANCODE_KP_CLEARENTRY %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_BINARY:
            printf("SDL_SCANCODE_KP_BINARY %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_OCTAL:
            printf("SDL_SCANCODE_KP_OCTAL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_DECIMAL:
            printf("SDL_SCANCODE_KP_DECIMAL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KP_HEXADECIMAL:
            printf("SDL_SCANCODE_KP_HEXADECIMAL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LCTRL:
            printf("SDL_SCANCODE_LCTRL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LSHIFT:
            printf("SDL_SCANCODE_LSHIFT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LALT:
            printf("SDL_SCANCODE_LALT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_LGUI:
            printf("SDL_SCANCODE_LGUI %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_RCTRL:
            printf("SDL_SCANCODE_RCTRL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_RSHIFT:
            printf("SDL_SCANCODE_RSHIFT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_RALT:
            printf("SDL_SCANCODE_RALT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_RGUI:
            printf("SDL_SCANCODE_RGUI %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_MODE:
            printf("SDL_SCANCODE_MODE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AUDIONEXT:
            printf("SDL_SCANCODE_AUDIONEXT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AUDIOPREV:
            printf("SDL_SCANCODE_AUDIOPREV %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AUDIOSTOP:
            printf("SDL_SCANCODE_AUDIOSTOP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AUDIOPLAY:
            printf("SDL_SCANCODE_AUDIOPLAY %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AUDIOMUTE:
            printf("SDL_SCANCODE_AUDIOMUTE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_MEDIASELECT:
            printf("SDL_SCANCODE_MEDIASELECT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_WWW:
            printf("SDL_SCANCODE_WWW %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_MAIL:
            printf("SDL_SCANCODE_MAIL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CALCULATOR:
            printf("SDL_SCANCODE_CALCULATOR %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_COMPUTER:
            printf("SDL_SCANCODE_COMPUTER %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AC_SEARCH:
            printf("SDL_SCANCODE_AC_SEARCH %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AC_HOME:
            printf("SDL_SCANCODE_AC_HOME %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AC_BACK:
            printf("SDL_SCANCODE_AC_BACK %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AC_FORWARD:
            printf("SDL_SCANCODE_AC_FORWARD %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AC_STOP:
            printf("SDL_SCANCODE_AC_STOP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AC_REFRESH:
            printf("SDL_SCANCODE_AC_REFRESH %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AC_BOOKMARKS:
            printf("SDL_SCANCODE_AC_BOOKMARKS %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_BRIGHTNESSDOWN:
            printf("SDL_SCANCODE_BRIGHTNESSDOWN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_BRIGHTNESSUP:
            printf("SDL_SCANCODE_BRIGHTNESSUP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_DISPLAYSWITCH:
            printf("SDL_SCANCODE_DISPLAYSWITCH %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KBDILLUMTOGGLE:
            printf("SDL_SCANCODE_KBDILLUMTOGGLE %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KBDILLUMDOWN:
            printf("SDL_SCANCODE_KBDILLUMDOWN %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_KBDILLUMUP:
            printf("SDL_SCANCODE_KBDILLUMUP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_EJECT:
            printf("SDL_SCANCODE_EJECT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SLEEP:
            printf("SDL_SCANCODE_SLEEP %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_APP1:
            printf("SDL_SCANCODE_APP1 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_APP2:
            printf("SDL_SCANCODE_APP2 %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AUDIOREWIND:
            printf("SDL_SCANCODE_AUDIOREWIND %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_AUDIOFASTFORWARD:
            printf("SDL_SCANCODE_AUDIOFASTFORWARD %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SOFTLEFT:
            printf("SDL_SCANCODE_SOFTLEFT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_SOFTRIGHT:
            printf("SDL_SCANCODE_SOFTRIGHT %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_CALL:
            printf("SDL_SCANCODE_CALL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_SCANCODE_ENDCALL:
            printf("SDL_SCANCODE_ENDCALL %d 0x%x\n", scancode, scancode);
            break;
        case SDL_NUM_SCANCODES:
            printf("SDL_NUM_SCANCODES %d 0x%x\n", scancode, scancode);
            break;
        default:
            printf("????????????????? %d 0x%x\n", scancode, scancode);
            break;
    }
    fflush(stdout);
}

int strcmp_ex(const char* const x, const char* const y) {
    if (x == y) return 0;
    if (x == NULL) return -1;
    if (y == NULL) return 1;
    return strcmp(x, y);
}

void free_ex(void** tgt) {
    if (tgt && *tgt) {
        free(*tgt);
    }
    *tgt = NULL;
}

void* calloc_ex(void** tgt, int nmemb, size_t memb_size) {
    if (tgt) {
        *tgt  = calloc(nmemb, memb_size);
        if (*tgt == NULL) {
            error_printf("OOM: %d * %d = %ld\n", nmemb, (int)memb_size, (long)(nmemb*memb_size));
        }
#if 0
        {
            printf("calloc: %d * %d = %ld\n", nmemb, (int)memb_size, (long)(nmemb*memb_size));
        }
#endif
        return *tgt;
    }
    return NULL;
}

