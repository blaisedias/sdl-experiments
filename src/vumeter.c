#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include "vumeter.h"
#include "vumeter_json.h"
#include "visualizer.h"
#include "util.h"
#include "timing.h"

extern bool debug_rects;

// ==== debug and profiling  {
static int profile_level;
void vumeter_set_profile_level(int l) {
    profile_level = l;
}
// ==== debug and profiling  }

// ==== setup {
void vumeter_setup(vumeter_instance_t* vumeter, SDL_Rect* bounds_in) {
    const vu_meters_specs_t* spec = vumeter->vss->spec;
    // Create a rectangle of same dimensions as the bounds but located at the origin 0,0
    // use this rectangle to scale and position the viewports
    // at render time an offset matching the bounds x and y values is applied
    // this makes it possible to move the vumeter as required simply by
    // adjusting the offset applied.
    SDL_Rect z_bounds = {.x=0, .y=0, .w=bounds_in->w, .h=bounds_in->h};

    // set the offset
    vumeter->offset.x = bounds_in->x;
    vumeter->offset.y = bounds_in->y;

    // calculate the scaling factor to fit the vu meter completely inside the bounds
    vumeter->scale_factor = MIN((float)z_bounds.w/(float)spec->layout.w, (float)z_bounds.h/(float)spec->layout.h);

    // create the scaled rectangle containing the vu meter inside the bounds
    SDL_Rect vu_bounds = {.x=0, .y=0, .w=spec->layout.w, .h=spec->layout.h};
    scale_rect_size(&vu_bounds, &vu_bounds, vumeter->scale_factor);

    // center the scaled rectangle within the bounds
    center_rect(&z_bounds, &vu_bounds, &vu_bounds);
    // scale and position the vu meter viewports
    for(int ixc=0; ixc < vumeter->defn->component_count; ++ixc) {
        scale_rect(spec->layout.viewports+ixc, vumeter->viewports+ixc, vumeter->scale_factor);
        rebaseRect(&vu_bounds, vumeter->viewports+ixc, vumeter->viewports+ixc);
    }

    vumeter->decay_unit = (float)spec->volume_levels/60;

    if (spec->layout.arrangement == HORIZONTAL_ARRANGEMENT && vumeter->vss->state->equal_horizontal_spacing) {
        //FIXME: works with stereo channels
#define FUZZ_DOWN(v) 2*((v)/2)
        SDL_Rect* lrect = vumeter->viewports + 1;
        SDL_Rect* rrect = vumeter->viewports + 2;
        // calculate the distribution of "whitespace" pixels on the horizontal axis
        int lead = FUZZ_DOWN(lrect->x - vu_bounds.x);
        int trail = FUZZ_DOWN((vu_bounds.x + vu_bounds.w) - (rrect->x + rrect->w));
        int middle = FUZZ_DOWN(rrect->x - (lrect->x + lrect->w));
//printf("-- lead=%d, trail=%d, middle=%d\n", lead, trail, middle);
        // the total number of "whitespace" pixels on the horizontal axis
        int avail = (lrect->x - z_bounds.x ) + ((z_bounds.x + z_bounds.w) - (rrect->x + rrect->w)) + middle;
//printf("avail=%d\n", avail);
        float fraction = ((float)avail/(lead+trail+middle));
        lead = lead * fraction;
        trail = trail * fraction;
        middle = avail - lead - trail;
//printf("++ lead=%d, trail=%d, middle=%d\n", lead, trail, middle);
        // reposition the left component
        lrect->x = z_bounds.x + lead;
        // reposition the right component
        rrect->x = lrect->x + lrect->w + middle;
    }
}
// ==== setup }

// ==== media load and unload {
static char load_buffer[4096];
bool vu_meters_load_media(SDL_Renderer* renderer, vu_meters_t* vu) {
    if (!vu) {
        return false;
    }
    int indx;
    int64_t ms = get_milli_seconds();
    SDL_bool ok = SDL_TRUE;
    load_printf("load media: %p\n"
            "resources: count=%d names=%p textures=%p\n",
            vu,
            vu->spec->resource_list.count,
            vu->spec->resource_list.names,
            vu->state->textures_list.textures
    );
    for(indx = 0; indx < vu->spec->resource_list.count; ++indx) {
        if (0 == vu->state->textures_list.textures[indx]) {
            if ( NULL != vu->spec->resource_list.names[indx]) {
                int n = snprintf(load_buffer, sizeof(load_buffer), "%s/%s",
                        vu->resource_path, vu->spec->resource_list.names[indx]);
                if (0 > n || n >= sizeof(load_buffer)) {
                    error_printf("snprintf %ld %s/%s/n",
                            sizeof(load_buffer),
                            vu->resource_path, vu->spec->resource_list.names[indx]);
                    exit(EXIT_FAILURE);
                }
                bool loaded = false;
                vu->state->textures_list.textures[indx] = tcache_load_media(load_buffer, renderer, &loaded, NULL);
                ok = ok && loaded;
            } else {
                // if no texture is associated with a slot point to the empty entry, this 
                //  - prevents error messages associated with retrieving texture for unintialised texture id
                //  - allows use of the texture id slot without additional checks
                vu->state->textures_list.textures[indx] = tcache_get_empty_tid();
            }
        } else {
            ok = ok && tcache_load_from_file(vu->state->textures_list.textures[indx], renderer);
        }
    }
    ms = get_milli_seconds() - ms;
    perf_printf("load media %s time:%lu milliseconds ok=%s\n",
                vu->spec->name,
                ms,
                ok?"true":"false");
    return ok;
}

void vu_meters_unload_media(vu_meters_t* vu) {
    if (vu) {
        load_printf("unload media: %p\n"
                "resources: count=%d names=%p textures=%p\n",
                vu,
                vu->spec->resource_list.count,
                vu->spec->resource_list.names,
                vu->state->textures_list.textures
        );
        for(int indx = 0; indx < vu->state->textures_list.count; ++indx) {
            tcache_quick_delete_texture(vu->state->textures_list.textures[indx]);
            vu->state->textures_list.textures[indx] = 0;
        }
    }
}

// ==== media load and unload }

// ==== render {
static uint64_t frame_count;
static uint32_t sample_frame_count;
static int64_t acc_render_time;
static int64_t max_render_time;
static int64_t ms_1;
static int64_t ms_2;
// to check and reset performance counters when vumeter is changed.
static const vumeter_instance_t* prev_vumeter;

static const SDL_RendererFlip flipValues[4] = {
    SDL_FLIP_NONE,
    SDL_FLIP_HORIZONTAL,
    SDL_FLIP_VERTICAL,
    SDL_FLIP_HORIZONTAL|SDL_FLIP_VERTICAL
};

static void render_placement(SDL_Renderer* renderer, vumeter_instance_t* vumeter, int placement_index, const SDL_Rect* viewport) {
    if (placement_index < 0 || placement_index >= vumeter->vss->spec->placement_list.count) {
        error_printf("renderPlacement: invalid index %d max=%d\n",
                placement_index, vumeter->vss->spec->placement_list.count);
        exit(EXIT_FAILURE);
    }
    vu_placement_t* pve = vumeter->vss->spec->placement_list.elements + placement_index;
#define _SCALE_PLACEMENT(val) ((val)*vumeter->scale_factor + 0.5)
    SDL_Rect    render_rect= {
        .x = _SCALE_PLACEMENT(pve->rect.x),
        .y = _SCALE_PLACEMENT(pve->rect.y),
        .w = _SCALE_PLACEMENT(pve->rect.w),
        .h = _SCALE_PLACEMENT(pve->rect.h),
    };
    SDL_Point   center = {
        .x = _SCALE_PLACEMENT(pve->center.x),
        .y = _SCALE_PLACEMENT(pve->center.y),
    };
    rebaseRect(viewport, &render_rect, &render_rect);
    SDL_RenderCopyEx(renderer,
            tcache_quick_get_texture(vumeter->vss->state->textures_list.textures[pve->texture_index], renderer),
            NULL,
            &render_rect,
//            vu->rotation + pve->angle,
            pve->angle,
            &center,
            flipValues[pve->flip]);
#undef _SCALE_PLACEMENT
}

void vumeter_render_background(SDL_Renderer* renderer, vumeter_instance_t* vumeter) {
    if (NULL == vumeter) {
        return;
    }
    const vumeter_defn_t* vudef = vumeter->defn;
    for(int ix_component=0; ix_component < vudef->component_count; ++ix_component) {
        const vu_component_t* component = &vudef->components[ix_component];
        SDL_Rect viewport;
        offset_rect(&vumeter->offset, vumeter->viewports + ix_component, &viewport);
        for(int ix_composition=0; ix_composition < component->composition_count; ++ix_composition) {
            int composition_index = component->ix_compositions[ix_composition];
            if (composition_index < 0 || composition_index >= vumeter->vss->spec->composition_list.count) {
                error_printf("invalid composition index %d max=%d\n",
                    composition_index, vumeter->vss->spec->composition_list.count);
                exit(EXIT_FAILURE);
            }
            vu_composition_t* composition = vumeter->vss->spec->composition_list.compositions + composition_index;
            if (composition->render_op == STATIC) {
                for (int ix_placement = 0; ix_placement < composition->placement_count; ++ix_placement) {
                    int placement_index =  composition->ix_placements[ix_placement];
                    render_placement(renderer, vumeter, placement_index, &viewport);
                }
            }
        }
    }
}

static Uint8 cc[][4] = {
    {255, 255, 0, 128},
    {0, 255, 0, 128},
    {0, 255, 0, 128},
};

void vumeter_render_foreground(SDL_Renderer* renderer, vumeter_instance_t* vumeter, runtime_volume_ptr vol_runtimes) {
#define _RENDER_VOLUME_LEVEL_(_vol_) render_placement(renderer, vumeter, composition->ix_placements[_vol_], &viewport)
    if (NULL == vumeter) {
        return;
    }
    const vumeter_defn_t* vudef = vumeter->defn;
    for(int ix_component=0; ix_component < vudef->component_count; ++ix_component) {
        const vu_component_t* component = &vudef->components[ix_component];
        SDL_Rect viewport;
        offset_rect(&vumeter->offset, vumeter->viewports + ix_component, &viewport);
        for(int ix_composition=0; ix_composition < component->composition_count; ++ix_composition) {
            int composition_index = component->ix_compositions[ix_composition];
            if (composition_index < 0 || composition_index >= vumeter->vss->spec->composition_list.count) {
                error_printf("invalid composition index %d max=%d\n",
                    composition_index, vumeter->vss->spec->composition_list.count);
                exit(EXIT_FAILURE);
            }
            vu_composition_t* composition = vumeter->vss->spec->composition_list.compositions + composition_index;
            runtime_volume_ptr runtime = vol_runtimes + ix_component - 1;
            switch(composition->render_op) {
                case STATIC:
                    // nothing to do
                    break;
                case SINGLE:
                    {
                        switch(composition->volume_type) {
                            case NONE:
                                break;
                            case SAMPLED:
                                vol_printf("SPN:%02d ", runtime->vol);
                                _RENDER_VOLUME_LEVEL_(runtime->vol);
                                break;
                            case DECAY:
                                vol_printf("SD:%02d ", (int)(runtime->decay_vol + 0.5));
                                _RENDER_VOLUME_LEVEL_((int)(runtime->decay_vol + 0.5));
                                break;
                            case PEAK_HOLD_AND_DECAY:
                                vol_printf("SHD:%02d ", (int)(runtime->decay_vol + 0.5));
                                _RENDER_VOLUME_LEVEL_((int)(runtime->decay_vol + 0.5));
                            case PEAK_HOLD_AND_SAMPLED:
                                vol_printf("SH:%02d ", runtime->peak_hold_vol);
                                _RENDER_VOLUME_LEVEL_(runtime->peak_hold_vol);
                                break;
                        }
                    }break;
                case AGGREGATE:
                    {
                        int vol = 0;
                        int peak_vol = 0;
                        switch(composition->volume_type) {
                            case NONE:
                                break;
                            case SAMPLED:
                                vol = runtime->vol;
                                peak_vol = 0;
                                vol_printf("A v:%02d ", vol);
                                break;
                            case PEAK_HOLD_AND_DECAY:
                                vol = (int)(runtime->decay_vol + 0.5);
                                peak_vol = runtime->peak_hold_vol;
                                vol_printf("A v:%02d p:%02d ", vol, peak_vol);
                                break;
                            case DECAY:
                                vol = (int)(runtime->decay_vol + 0.5);
                                peak_vol = 0;
                                vol_printf("A v:%02d ", vol);
                                break;
                            case PEAK_HOLD_AND_SAMPLED:
                                vol = runtime->vol;
                                peak_vol = runtime->peak_hold_vol;
                                vol_printf("A v:%02d p:%02d ", vol, peak_vol);
                                break;
                        }

                        for(int lvl=0; lvl <= vol; ++lvl) {
                            _RENDER_VOLUME_LEVEL_(lvl);
                        }
                        if (peak_vol > vol) {
                           _RENDER_VOLUME_LEVEL_(peak_vol);
                        }
                    }break;
                case AGGREGATEOFF:
                    {
                        int vol = 0;
                        switch(composition->volume_type) {
                            case NONE:
                                break;
                            case SAMPLED:
                            case PEAK_HOLD_AND_SAMPLED:
                                vol = runtime->vol;
                                break;
                            case DECAY:
                            case PEAK_HOLD_AND_DECAY:
                                vol = (int)(runtime->decay_vol + 0.5);
                                break;
                        }
                        vol_printf("a v:%02d ", vol);
                        for(int lvl=vol+1; lvl< composition->placement_count; ++lvl) {
                            _RENDER_VOLUME_LEVEL_(lvl);
                        }
                    }break;
            }
        }
        if (debug_rects) {
            SDL_SetRenderDrawColor(renderer, cc[ix_component][0], cc[ix_component][1], cc[ix_component][2], cc[ix_component][3]);
            SDL_RenderDrawRect(renderer, &viewport);
            SDL_SetRenderDrawColor(renderer, 0, 0, 0, 0);
        }
    }
#undef _RENDER_VOLUME_LEVEL_
}

void vumeter_render_foreground_ms(SDL_Renderer* renderer, vumeter_instance_t* vumeter, runtime_volume_ptr vol_runtimes) {
    if (NULL == vumeter) {
        prev_vumeter = vumeter;
        return;
    }

    if (perf_printf != dummy_printf) {
       if(prev_vumeter != vumeter) {
            max_render_time = 0;
            sample_frame_count = 0;
            frame_count = 0;
            acc_render_time = 0;
            ms_1 = get_micro_seconds();
        }
        prev_vumeter = vumeter;
    }

    int64_t ms0 = get_micro_seconds();

    vumeter_render_foreground(renderer, vumeter, vol_runtimes);
    
    int64_t delta_pf = get_micro_seconds() - ms0;
    acc_render_time += delta_pf;

    if (delta_pf > max_render_time) {
        max_render_time =  delta_pf;
//        printf("%ld\n", max_render_time);
    }
    ++sample_frame_count;
    ++frame_count;
    if (sample_frame_count >= 100) {
        ms_2 = get_micro_seconds();
        float fps = 1000000.0 * sample_frame_count/(ms_2-ms_1);
        switch(profile_level) {
            case 3:
            case 2:
            default:
                perf_printf("\rFPS:%05.2f frame:millis: avg:%5.2f, max:%05.2f sample: millis:%5.2f, frames:%d texture_cache:%ld",
                        fps,
                        ((float)acc_render_time/sample_frame_count)/1000,
                        (float)max_render_time/1000,
                        (float)(ms_2 - ms_1)/1000, sample_frame_count,
                        tcache_get_texture_bytes_count()
                    );
        }
        sample_frame_count = 0;
        acc_render_time = 0;
        ms_1 = ms_2;
//        max_render_time = 0;
    }
}
// ==== render }


// ==== volume levels {
// @60 FPS 30 => 1/2 a second
static int peak_hold_counter_init_value = 30;
// fine tune decay behaviour - default is 0 so decay immediately
// @60 FPS 4 appears to be a reasonable value.
static int decay_hold_counter_init_value = 3;

void update_volume_levels(runtime_volume_ptr vol_runtimes, int* vols, float decay_unit) {
    vol_runtimes[0].vol = vols[0];
    vol_runtimes[1].vol = vols[1];

    for (int ix_chan=0; ix_chan < NUM_VU_CHANNELS; ++ix_chan) {
        vol_runtimes[ix_chan].vol = vols[ix_chan];
        if (vol_runtimes[ix_chan].vol >= vol_runtimes[ix_chan].peak_hold_vol) {
//            vol_runtimes[ix_chan].eak_hold_counter = peak_hold_counter_start;
            vol_runtimes[ix_chan].peak_hold_counter = peak_hold_counter_init_value;
            vol_runtimes[ix_chan].peak_hold_vol = vol_runtimes[ix_chan].vol;
        }
        if (--vol_runtimes[ix_chan].peak_hold_counter < 0) {
            vol_runtimes[ix_chan].peak_hold_vol = 0;
            vol_runtimes[ix_chan].peak_hold_counter = 0;
        }
        if (vol_runtimes[ix_chan].vol >= vol_runtimes[ix_chan].decay_vol) {
            vol_runtimes[ix_chan].decay_vol = vol_runtimes[ix_chan].vol;
            vol_runtimes[ix_chan].decay_hold_counter = decay_hold_counter_init_value;
        } else {
            if (--vol_runtimes[ix_chan].decay_hold_counter < 0) {
                vol_runtimes[ix_chan].decay_vol -= decay_unit;
                vol_runtimes[ix_chan].decay_hold_counter = 0;
            }
        }
    }
}

int vumeter_set_peak_hold(int v) {
    int retv = peak_hold_counter_init_value;
    peak_hold_counter_init_value = v;
    return retv;
}

int vumeter_set_decay_hold(int v) {
    int retv = decay_hold_counter_init_value;
    decay_hold_counter_init_value = v;
    return retv;
}

// ==== volume levels }

// ==== json file {
static vu_meters_t* vu_meters_list = NULL;

bool vumeter_load_from_json_file(const char* filepath) {
    vu_meters_t* vu = deserialise_vumeters_json_file(filepath);
    if (NULL == vu) {
        return false;
    }
    vu_meters_t** pvumeters = &vu_meters_list;
    while(NULL != *pvumeters) {
        pvumeters = &(*pvumeters)->next;
    }
    *pvumeters = vu;
    return true;
}

int vumeter_populate_instance_array(vumeter_instance_t* array, size_t length) {
    int insert = 0;
    for(vu_meters_t* vu = vu_meters_list; vu; vu = vu->next) {
        for(int ix=0; ix < vu->spec->vumeter_list.count; ++ix, ++insert) {
            if (insert < length) {
                vumeter_instance_t* vumtr = &array[insert];
                vumtr->vss = vu;
                vumtr->defn = &vu->spec->vumeter_list.vumeters[ix];
                vumtr->disabled = &vu->state->vu_meter_disabled.elements[ix];
                for(int ixc= 0; ixc < NUM_COMPONENTS; ++ixc) {
                    copyRect(vu->spec->layout.viewports+ixc, vumtr->viewports+ixc);
                }
            } else {
                error_printf("available vu meters exceed %d\n", length);
            }
        }
    }
    return insert;
}

static void _vumeters_release(vu_meters_t** pvu) {
    if (*pvu) {
        _vumeters_release(&((*pvu)->next));
        release_deserialised_vumeters(*pvu);
        *pvu = NULL;
    }
}

void vumeter_release_all() {
    _vumeters_release(&vu_meters_list);
}

// ==== json file }

// ==== DEBUG {
void vumeter_dump_all_specs() {
    for(vu_meters_t* vu = vu_meters_list; vu ; vu =vu->next) {
        dump_vumeter_specs(vu->spec);
    }
}

void vumeter_checked_dump_all_specs() {
    for(vu_meters_t* vu = vu_meters_list; vu ; vu =vu->next) {
        checked_dump_vumeter_specs(vu->spec);
    }
}
// ==== DEBUG }
