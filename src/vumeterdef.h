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

typedef struct {
    int vol;
    int peak_hold_vol;
    int peak_hold_counter;
    int decay_hold_counter;
    float decay_vol;
}runtime_volume_t, *runtime_volume_ptr;

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
    composition_render_op_t        render_op;
    composition_volume_type_t      volume_type;
    int     placement_count;
    int*    ix_placements;
}vu_composition_t;


typedef struct {
    int     composition_count;
    int*    ix_compositions;
}vu_component_t;

typedef struct {
    char*           name;
    int             component_count;
    // fascia, left, right
    vu_component_t    components[1+NUM_VU_CHANNELS];
}vumeter_t;

typedef struct vu_props {
    struct vu_props*    next;
    char*   resource_path;
    char*   name;
    int     volume_levels;
    float   rotation;
    void*   handle;
    int     format_version;
    struct {
        int           w;
        int           h;
        // fascia, left, right
        SDL_Rect      rects[3];
        channel_arrangement_t  arrangement;
    }layout;
    struct {
        int             count;
        char**          names;
        texture_id_t*   textures;
    }resource_list;
    struct {
        int             count;
        vu_placement_t* elements;
    }placement_list;
    struct {
        int                 count;
        vu_composition_t*   compositions;
    }composition_list;
    struct {
        int         count;
        vumeter_t*  vumeters;
    }vumeter_list;
#ifdef DEBUG_VUMETER_JSON
    const char* kind;
    const char* vutype;
    const char* format;
#endif
}vumeter_properties_t;

#endif  // __jl_vumeterdef_h_
