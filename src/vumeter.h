#ifndef __jl_vumeter_h_
#define __jl_vumeter_h_
#include "logging.h"
#include "vumeterdef.h"
#include "types.h"

void vumeter_setup(vumeter_instance_t* vumeter, SDL_Rect* bounds_in, bool equal_horizontal_spacing);

bool vu_meters_load_media(SDL_Renderer* renderer, vu_meters_t* vu);
void vu_meters_unload_media(vu_meters_t* vu);

void vumeter_render_background(SDL_Renderer* renderer, vumeter_instance_t* vumeter);
void vumeter_render_foreground(SDL_Renderer* renderer, vumeter_instance_t* vumeter, runtime_volume_ptr vol_runtimes);
void vumeter_render_foreground_ms(SDL_Renderer* renderer, vumeter_instance_t* vumeter, runtime_volume_ptr vol_runtimes);

void update_volume_levels(runtime_volume_ptr vol_runtimes, int* vols, float decay_unit);
int vumeter_set_peak_hold(int v);
int vumeter_set_decay_hold(int v);

void vumeter_set_profile_level(int l);

bool vumeter_load_from_json_file(const char* filepath);
int vumeter_populate_instance_array(vumeter_instance_t* array, size_t length);
void vumeter_release_all();

// debug
void vumeter_dump_all_specs();
void vumeter_checked_dump_all_specs();
#endif  // __jl_vumeter_h_
