/*
** Copyright 2025 Blaise Dias. All Rights Reserved.
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/

#ifndef __jl_vumeterdef_h_
#define __jl_vumeterdef_h_
#include <SDL2/SDL.h>
#include "texture_cache.h"
#include "vumeter_enum.h"

// For now the number of channels is fix at 2
// a future change will remove this hard-coding
#define     NUM_VU_CHANNELS   2

typedef struct {
//    const char* image;
    int                 texture_index;
    SDL_Rect            rect;
    SDL_RendererFlip    flip;
    float               angle;
    SDL_Point           center;
}vu_placement_t;

typedef struct {
    vu_component_render_mode_t    render;
    vu_component_level_t          peak;
    int           placement_count;
    int           placements[50];
}vu_component_t;

typedef struct {
    int           placement_count;
    int           placements[100];
}vu_background_t;

typedef struct {
    int vol;
    int peak_hold_vol;
    int peak_hold_counter;
    int decay_hold_counter;
    float decay_vol;
}runtime_volume_t, *runtime_volume_ptr;

typedef struct {
    int               component_count;
    vu_component_t*   components;
}vu_channel_t;

typedef struct {
    char* name;
    int              channel_count;
    vu_background_t* background;
    vu_background_t* backgrounds[NUM_VU_CHANNELS];
    vu_channel_t*    channels[NUM_VU_CHANNELS];
}vumeter_t;

typedef struct vu_props {
    struct vu_props*    next;
    char*         resource_path;
    char*         name;
    int           volume_levels;
    struct {
        int           w;
        int           h;
        // common + channels, common = 0, left=1, right=2
        SDL_Rect      rects[3];
        layout_arrangement_t  arrangement;
    }layout;
    int           vumeter_count;
    vumeter_t*    vumeters;
    struct {
        int             count;
        char**          names;
        texture_id_t*   textures;
    }resources;
    struct {
        int             count;
        vu_placement_t* elements;
    }placements;
    float rotation;
    void * handle;
    int   format_version;
    struct {
        int                 count;
        vu_component_t*     components;
    }components;
#ifdef DEBUG_VUMETER_JSON
    const char* kind;
    const char* vutype;
    const char* format;
#endif
}vumeter_properties_t;

#endif  // __jl_vumeterdef_h_
