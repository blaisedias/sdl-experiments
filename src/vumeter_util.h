/*
** Copyright 2025 Blaise Dias. All Rights Reserved.
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/

#ifndef __jl_vumeter_util_h_
#define __jl_vumeter_util_h_
#include "logging.h"
#include "vumeterdef.h"
#include "types.h"

typedef struct {
    float scale_factor;
}vu_channel_params_t, *vu_channel_params_ptr;

char* VUMeter_resource_path(const char *root, vumeter_properties_t* vu);

float VUMeter_scale_factor(vumeter_properties_t* vu, int w, int h);
SDL_bool VUMeter_load_media(SDL_Renderer *renderer, vumeter_properties_t *vu);
void VUMeter_unload_media(vumeter_properties_t *vu);

void VUMeter_draw(SDL_Renderer *renderer, vumeter_properties_t* vu, const vumeter_t* vumeter, int* vols, SDL_Rect* enclosure, vu_channel_params_ptr channel_parms, runtime_volume_ptr vol_runtimes, float decay_unit);

void VUMeter_dump_props(const vumeter_properties_t* vu);

void VUMeter_diag();

void VUMeter_set_perf_level(int);

bool VUMeter_loadlib(const char* path);
const vumeter_properties_t* VUMeter_get_props_list();

void VUMeter_set_peak_hold(int peak_hold);
void VUMeter_set_decay_hold(int decay_hold);

#endif  // __jl_vumeter_util_h_

