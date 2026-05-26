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

void rebaseRect(SDL_Rect* origin, SDL_Rect* src, SDL_Rect* dst) {
    dst->x = origin->x + src->x;
    dst->y = origin->y + src->y;
    dst->w = src->w;
    dst->h = src->h;
}

void rebasePoint(SDL_Rect* origin, SDL_Point* src, SDL_Point* dst) {
    dst->x = origin->x + src->x;
    dst->y = origin->y + src->y;
}

void translate_axle(SDL_Rect* enclosure, SDL_Point* axle, SDL_Rect* rect) {
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
