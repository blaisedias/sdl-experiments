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
    int     vol;
    int     peak_hold_vol;
    int     peak_hold_counter;
    int     decay_hold_counter;
    float   decay_vol;
    float   decay_unit;
}runtime_volume_t, *runtime_volume_ptr;

// For now the number of channels is fix at 2
// a future change will remove this hard-coding
#define     NUM_VU_CHANNELS     2
#define     NUM_COMPONENTS      1 + NUM_VU_CHANNELS

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
}vumeter_defn_t;

typedef struct vu_props {
    char*   name;
    int     volume_levels;
    float   rotation;
    void*   handle;
    int     format_version;
    struct {
        int           w;
        int           h;
        // fascia, left, right
        SDL_Rect      viewports[NUM_COMPONENTS];
        channel_arrangement_t  arrangement;
    }layout;
    struct {
        int                 count;
        char**              names;
    }resource_list;
    struct {
        int                 count;
        vu_placement_t*     elements;
    }placement_list;
    struct {
        int                 count;
        vu_composition_t*   compositions;
    }composition_list;
    struct {
        int                 count;
        vumeter_defn_t*     vumeters;
    }vumeter_list;
#ifdef DEBUG_VUMETER_JSON
    const char* kind;
    const char* vutype;
    const char* format;
#endif
}vu_meters_specs_t;

typedef struct { 
    struct {
        int             count;
        texture_id_t*   textures;
    }textures_list;
    struct {
        int     count;
        bool*   elements;
    }vu_meter_disabled;
    bool equal_horizontal_spacing;
}vu_meters_state_t;

typedef struct vu_meters {
    struct vu_meters*           next;
    const char*                 resource_path;
    const vu_meters_specs_t*    spec;
    vu_meters_state_t*          state;
}vu_meters_t;

typedef struct {
    vu_meters_t*    vss;
    const vumeter_defn_t*   defn;
    const bool*             disabled;
    SDL_Point               offset;
    // scaled layout
    SDL_Rect                viewports[NUM_COMPONENTS];
    float                   scale_factor;
    float                   decay_unit;
}vumeter_instance_t;
#endif  // __jl_vumeterdef_h_
