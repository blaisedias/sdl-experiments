/*
** Copyright 2025 Blaise Dias. All Rights Reserved.
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/

#ifndef __jl_vumeterdef_h_
#define __jl_vumeterdef_h_
#include <SDL2/SDL.h>
#include "texture_cache.h"

// For now the number of channels is fix at 2
// a future change will remove this hard-coding
#define     NUM_VU_CHANNELS   2

typedef enum {
    HORIZONTAL_ARRANGEMENT,
    VERTICAL_ARRANGEMENT,
} layout_arrangement;

typedef enum {
    STATIC,
    SINGLE,
    AGGREGATE,
    AGGREGATEOFF,
} component_render_op_t;

typedef enum {
    SAMPLED,
    PEAK_HOLD_AND_SAMPLED,
    DECAY,
    PEAK_HOLD_AND_DECAY,
}component_volume_type_t;

typedef struct {
//    const char* image;
    int                 texture_index;
    SDL_Rect            rect;
    SDL_RendererFlip    flip;
    float               angle;
    SDL_Point           center;
}vu_placement_t;

typedef struct {
    const component_render_op_t     render_op;
    const component_volume_type_t   volume_type;
    const int           placement_count;
    const int           placements[50];
}vu_component_t;

typedef struct {
    const int           placement_count;
    const int           placements[100];
}vu_background_t;

typedef struct {
    int vol;
    int peak_hold_vol;
    int peak_hold_counter;
    int decay_hold_counter;
    float decay_vol;
}runtime_volume_t, *runtime_volume_ptr;

typedef struct {
    const int               component_count;
    const vu_component_t*   components;
}vu_channel_t;

typedef struct {
    const char* name;
    const int              channel_count;
    const vu_background_t* background;
    const vu_background_t* backgrounds[NUM_VU_CHANNELS];
    const vu_channel_t*    channels[NUM_VU_CHANNELS];
}vumeter_t;

typedef struct vu_props {
    struct vu_props*    next;
    const char*         resource_path;
    const char*         name;
    const int           volume_levels;
    struct {
        const int           w;
        const int           h;
        // common + channels, common = 0, left=1, right=2
        const SDL_Rect      rects[3];
        layout_arrangement  arrangement;
    }layout;
    const int           vumeter_count;
    const vumeter_t*    vumeters;
    struct {
        const int       count;
        const char**    names;
        texture_id_t*   textures;
    }resources;
    struct {
        const int       count;
        vu_placement_t* elements;
    }placements;
    float rotation;
    void * handle;
}vumeter_properties_t;

#endif  // __jl_vumeterdef_h_
