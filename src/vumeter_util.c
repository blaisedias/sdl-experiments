/*
** Copyright 2025 Blaise Dias. All Rights Reserved.
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/

#include <string.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include "vumeter_util.h"
#include "visualizer.h"
#include "util.h"
#include "timing.h"

static const SDL_RendererFlip flipValues[4] = {
    SDL_FLIP_NONE,
    SDL_FLIP_HORIZONTAL,
    SDL_FLIP_VERTICAL,
    SDL_FLIP_HORIZONTAL|SDL_FLIP_VERTICAL
};

// @60 FPS 30 => 1/2 a second
static int peak_hold_counter_init_value = 30;
// fine tune decay behaviour - default is 0 so decay immediately
// @60 FPS 4 appears to be a reasonable value.
static int decay_hold_counter_init_value = 3;

static int profile_level;
void VUMeter_set_profile_level(int l) {
    profile_level = l;
}

void VUMeter_set_peak_hold(int peak_hold) {
    peak_hold_counter_init_value = peak_hold;
}

void VUMeter_set_decay_hold(int decay_hold) {
    decay_hold_counter_init_value = decay_hold;
}

static char load_buffer[4096];
SDL_bool VUMeter_load_media(SDL_Renderer* renderer, vumeter_properties_t* vu) {
    if (!vu) {
        return false;
    }
    int indx;
    int64_t ms = get_milli_seconds();
    SDL_bool ok = SDL_TRUE;
    load_printf("load media: %p\n"
            "resources: count=%d names=%p textures=%p\n",
            vu,
            vu->resources.count,
            vu->resources.names,
            vu->resources.textures
    );
    for(indx = 0; indx < vu->resources.count; ++indx) {
        if (0 == vu->resources.textures[indx]) {
            if ( NULL != vu->resources.names[indx]) {
                int n = snprintf(load_buffer, sizeof(load_buffer), "%s/%s",
                        vu->resource_path, vu->resources.names[indx]);
                if (0 > n || n >= sizeof(load_buffer)) {
                    error_printf("snprintf %ld %s/%s/n",
                            sizeof(load_buffer),
                            vu->resource_path, vu->resources.names[indx]);
                    exit(EXIT_FAILURE);
                }
                bool loaded = false;
                vu->resources.textures[indx] = tcache_load_media(load_buffer, renderer, &loaded, NULL);
                ok = ok && loaded;
            } else {
                // if no texture is associated with a slot point to the empty entry, this 
                //  - prevents error messages associated with retrieving texture for unintialised texture id
                //  - allows use of the texture id slot without additional checks
                vu->resources.textures[indx] = tcache_get_empty_tid();
            }
        } else {
            ok = ok && tcache_load_from_file(vu->resources.textures[indx], renderer);
        }
    }
    ms = get_milli_seconds() - ms;
    perf_printf("load media %s time:%lu milliseconds ok=%s\n",
                vu->name,
                ms,
                ok?"true":"false");
    return ok;
}

void VUMeter_unload_media(vumeter_properties_t* vu) {
    if (vu) {
        load_printf("unload media: %p\n"
                "resources: count=%d names=%p textures=%p\n",
                vu,
                vu->resources.count,
                vu->resources.names,
                vu->resources.textures
        );
        for(int indx = 0; indx < vu->resources.count; ++indx) {
            tcache_quick_delete_texture(vu->resources.textures[indx]);
            vu->resources.textures[indx] = 0;
        }
    }
}

char* VUMeter_resource_path(const char *root, vumeter_properties_t* vu) {
    if (!vu) {
        return strdup(root);
    }
    if (vu->name != NULL) {
        int n = snprintf(load_buffer, sizeof(load_buffer), "%s/%s", root, vu->name);
        if (0 > n || n >= sizeof(load_buffer)) {
            error_printf("snprintf ");
            exit(EXIT_FAILURE);
        }
    } else {
        int n = snprintf(load_buffer, sizeof(load_buffer), "%s", root);
        if (0 > n || n >= sizeof(load_buffer)) {
            error_printf("snprintf ");
            exit(EXIT_FAILURE);
        }
    }
    return strdup(load_buffer);
}

float VUMeter_scale_factor(vumeter_properties_t* vu, int w, int h) {
    if (!vu) {
        return 1;
    }
    return MIN((float)w/(float)vu->layout.w, (float)h/(float)vu->layout.h);
}


static uint64_t frame_count;
static uint32_t sample_frame_count;
static int64_t acc_render_time;
static int64_t max_render_time;
static int64_t ms_1;
static int64_t ms_2;
// to check and reset performance counters when vumeter is changed.
static const vumeter_t* prev_vumeter;

static inline void renderPlacement(vu_placement_t* pve, SDL_Rect* enclosure, vumeter_properties_t* vu, SDL_Renderer* renderer, float scale_factor) {
#define VU_SCALE(val) ((val)*scale_factor + 0.5)
    SDL_Rect    render_rect= {
        .x = VU_SCALE(pve->rect.x),
        .y = VU_SCALE(pve->rect.y),
        .w = VU_SCALE(pve->rect.w),
        .h = VU_SCALE(pve->rect.h),
    };
    SDL_Point   center = {
        .x = VU_SCALE(pve->center.x),
        .y = VU_SCALE(pve->center.y),
    };
    rebaseRect(enclosure, &render_rect, &render_rect);
    SDL_RenderCopyEx(renderer,
            tcache_quick_get_texture(vu->resources.textures[pve->texture_index], renderer),
            NULL,
            &render_rect,
            vu->rotation + pve->angle,
            &center,
            flipValues[pve->flip]);
#undef VU_SCALE
}

void VUMeter_draw(SDL_Renderer* renderer, vumeter_properties_t* vu, const vumeter_t* vumeter, int* vols, SDL_Rect* enclosure, vu_channel_params_ptr channel_parms, runtime_volume_ptr vol_runtimes, float decay_unit) {
    if (!vu) {
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

    if (vumeter->background) {
        const vu_background_t* bg = vumeter->background;
        for(int ix=0; ix < bg->placement_count; ++ix) {
            renderPlacement(vu->placements.elements+bg->placements[ix],
                    enclosure, vu, renderer,
                    //FIXME: should be for the vumeter rectangle 
                    channel_parms[0].scale_factor);
        }
    }

#define _RENDER_VOLUME_LEVEL_(value, chn) \
    renderPlacement(vu->placements.elements+comp->placements[value], enclosure, vu, renderer,\
            channel_parms[chn].scale_factor)

/*    
#define _RENDER_VOLUME_LEVEL_(value, chn) \
    renderPlacement(vu->placements.elements+comp->placements[value], &channel_parms[chn].channel_rect, vu, renderer,\
            channel_parms[chn].scale_factor)
*/

    for(int ix_chan=0; ix_chan < NUM_VU_CHANNELS; ++ix_chan) {
        const vu_background_t* bg = vumeter->backgrounds[ix_chan];
        for(int ix=0; ix < bg->placement_count; ++ix) {
            renderPlacement(vu->placements.elements+bg->placements[ix],
                    enclosure, vu, renderer,
                    channel_parms[ix_chan].scale_factor);
        }
        vol_printf("%2d) ", ix_chan);
        const vu_channel_t* channel = vumeter->channels[ix_chan];
        const vu_component_t* comp = channel->components;
        runtime_volume_ptr runtime = vol_runtimes + ix_chan;
        for(int ic=0; ic < channel->component_count; ++ic, ++comp) {
            switch(comp->render) {
                case SINGLE:
                    {
                        switch(comp->peak) {
                            case PEAK_NONE:
                                vol_printf("SPN:%02d ", runtime->vol);
                                _RENDER_VOLUME_LEVEL_(runtime->vol, ix_chan);
                                break;
                            case DECAY:
                                vol_printf("SD:%02d ", (int)(runtime->decay_vol + 0.5));
                                _RENDER_VOLUME_LEVEL_((int)(runtime->decay_vol + 0.5), ix_chan);
                                break;
                            case HOLD_DECAY:
                                vol_printf("SHD:%02d ", (int)(runtime->decay_vol + 0.5));
                                _RENDER_VOLUME_LEVEL_((int)(runtime->decay_vol + 0.5), ix_chan);
                            case HOLD:
                                vol_printf("SH:%02d ", runtime->peak_hold_vol);
                                _RENDER_VOLUME_LEVEL_(runtime->peak_hold_vol, ix_chan);
                                break;
                        }
                    }break;
                case AGGREGATE:
                    {
                        int vol = 0;
                        int peak_vol = 0;
                        switch(comp->peak) {
                            case PEAK_NONE:
                                vol = runtime->vol;
                                peak_vol = 0;
                                vol_printf("A v:%02d ", vol);
                                break;
                            case HOLD_DECAY:
                                vol = (int)(runtime->decay_vol + 0.5);
                                peak_vol = runtime->peak_hold_vol;
                                vol_printf("A v:%02d p:%02d ", vol, peak_vol);
                                break;
                            case DECAY:
                                vol = (int)(runtime->decay_vol + 0.5);
                                peak_vol = 0;
                                vol_printf("A v:%02d ", vol);
                                break;
                            case HOLD:
                                vol = runtime->vol;
                                peak_vol = runtime->peak_hold_vol;
                                vol_printf("A v:%02d p:%02d ", vol, peak_vol);
                                break;
                        }

                        for(int lvl=0; lvl <= vol; ++lvl) {
                            _RENDER_VOLUME_LEVEL_(lvl, ix_chan);
                        }
                        if (peak_vol > vol) {
                           _RENDER_VOLUME_LEVEL_(peak_vol, ix_chan);
                        }
                    }break;
                case AGGREGATEOFF:
                    {
                        int vol = 0;
                        switch(comp->peak) {
                            case PEAK_NONE:
                            case HOLD:
                                vol = runtime->vol;
                                break;
                            case DECAY:
                            case HOLD_DECAY:
                                vol = (int)(runtime->decay_vol + 0.5);
                                break;
                        }
                        vol_printf("a v:%02d ", vol);
                        for(int lvl=vol+1; lvl< vu->volume_levels; ++lvl) {
                            _RENDER_VOLUME_LEVEL_(lvl, ix_chan);
                        }
                    }break;
            }
        }
    }
    vol_printf("\r");

#undef _RENDER_VOLUME_LEVEL_
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

void VUMeter_diag() {
    printf("visualizer rate= %u\n", vis_get_rate());
}


void VUMeter_dump_props(const vumeter_properties_t* props) {
    if(!props) {
        return;
    }
    printf("%p\nresource_path=%s name=%s volume_levels=%d w=%d h=%d vumeters=%p\n",
            props,
            props->resource_path,
            props->name,
            props->volume_levels, props->w, props->h,
            props->vumeters
           );
    printf("resources: count=%d names=%p textures=%p\n",
            props->resources.count,
            props->resources.names,
            props->resources.textures
           );
    printf("placements: count=%d elements=%p\n",
            props->placements.count,
            props->placements.elements
           );
    for(int indx=0; indx < props->resources.count; ++indx) {
        printf("%d) texture=%d %s\n",
                indx,
                 props->resources.textures[indx],
                 props->resources.names[indx]
               );
    }
    const vumeter_t* vumeter = props->vumeters;
/*    
    const int *bg = vumeter->background->bg;
    while(bg != NULL && 0 != *bg) {
        vu_placement_t *p = &props->placements.elements[*bg];
        printf("bg, texture_index=%02d texture=%d, rect=(%d, %d, %d, %d) flip=%d %s\n",
                p->texture_index,
                props->resources.textures[p->texture_index],
                p->rect.x, p->rect.y, p->rect.w, p->rect.h,
                p->flip,
                props->resources.names[p->texture_index]
               );
        ++bg;
    }
    */
    for(int ix_chan=0; ix_chan < NUM_VU_CHANNELS; ++ix_chan) {
        const vu_channel_t* channel = vumeter->channels[ix_chan];
        const vu_component_t* comp = channel->components;
        printf("channel %d, components count %d\n", ix_chan, channel->component_count);
        for(int ic=0; ic < channel->component_count; ++ic, ++comp) {
            printf("channel %d), component %d) %d, %d %p\n", ix_chan, ic, comp->render, comp->peak, comp);
            for(int value =0; value < props->volume_levels; ++value) {
                int pi = comp->placements[value];
                int ti = props->placements.elements[pi].texture_index;
                int flip = props->placements.elements[pi].flip;
                SDL_Rect *r = &props->placements.elements[pi].rect;
                printf("\tlevel=%d placement_index=%d texture_index=%d texture=%d rect=(%d,%d,%d,%d) flip=%d %s\n",
                        value,
//                        props->placements.elements[value].texture_index,
//                        props->resources.textures[props->placements.elements[value].texture_index],
                        pi,
                        ti,
                        props->resources.textures[ti],
                        r->x, r->y, r->w, r->h,
                        flip,
                        props->resources.names[ti]
                       );
            }
        }
    }
}

