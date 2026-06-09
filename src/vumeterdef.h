/*
** Copyright 2025 Blaise Dias. All Rights Reserved.
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/

#ifndef __jl_vumeterdef_h_
#define __jl_vumeterdef_h_
#include <SDL2/SDL.h>
#include "texture_cache.h"

#define     NUM_VU_CHANNELS   2

typedef struct {
//    const char* image;
    int         texture_index;
    SDL_Rect    rect;
    SDL_RendererFlip flip;
    float       angle;
    SDL_Point   center;
}vu_placement_t;

typedef enum {
    SINGLE,
    AGGREGATE,
    AGGREGATEOFF,
} render_typ;

typedef enum {
    PEAK_NONE,
    HOLD,
    DECAY,
    HOLD_DECAY,
}peak_typ;

typedef struct {
    const int* bg;
}background;

typedef struct {
    const render_typ  render;
    const peak_typ    peak;
    const int placements[50];
}vu_component_t;

typedef struct {
    int vol;
    int peak_hold_vol;
    int peak_hold_counter;
    int decay_hold_counter;
    float decay_vol;
}runtime_volume_t, *runtime_volume_ptr;

typedef struct {
    const int component_count;
    const vu_component_t* components;
}vu_channel_t;

typedef struct {
    const char* name;
    const background*   background;
    vu_channel_t*      channels[2];
}vumeter_t;

typedef struct vu_props {
    struct vu_props* next;
    const char* resource_path;
    const char* name;
    const int volume_levels;
    const int w;
    const int h;
    const int vumeter_count;
    const vumeter_t* vumeters;
    struct {
        const int count;
        const char** names;
        texture_id_t* textures;
    }resources;
    struct {
        const int count;
        vu_placement_t* elements;
    }placements;
    float rotation;
    void * handle;
}vumeter_properties_t;

#endif  // __jl_vumeterdef_h_
