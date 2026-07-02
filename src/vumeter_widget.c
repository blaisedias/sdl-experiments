#include <dlfcn.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
//#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>
#include <time.h>
#include "vumeter_util.h"
#include "application.h"
#include "widgets_internal.h"
#include "visualizer.h"

static vumeter_properties_t* vu_props_list;

//static struct {
//    vumeter_properties_t *props;
//    const vumeter_t* meter;
//} meters[100];
//static int num_meters=0;

typedef struct {
    vumeter_properties_t    *props;
    const                   vumeter_t* meter;
    vu_channel_params_t     channel_parms[NUM_VU_CHANNELS];
    float                   decay_unit;
    // rectangle to scale and centred within the widget rectangle
    SDL_Rect                vu_rect;
}_vw_meter_t;

struct vumeter_widget {
    vumeter_properties_t* props;
    _vw_meter_t meters[100];
    int     num_meters;
    int     atomic_meter_indx;
    bool    locked;
    bool    equal_horizontal_spacing;
    runtime_volume_t vol_runtimes[NUM_VU_CHANNELS];
};

static inline int vumeter_index(vumeter_widget_t* wdgt) {
    return  __atomic_load_n(&wdgt->atomic_meter_indx, __ATOMIC_ACQUIRE);
}

static inline void vumeter_set_index(vumeter_widget_t* wdgt, int ix) {
     __atomic_store_n(&wdgt->atomic_meter_indx, ix, __ATOMIC_RELEASE);
}

const vumeter_properties_t* VUMeter_get_props_list() {
    return vu_props_list;
}

bool VUMeter_loadlib(const char* path) {
    void* handle = dlopen(path, RTLD_NOW);
    dlerror();
    if (handle == NULL) {
        error_printf("failed to load %s\n", path);
//        exit(EXIT_FAILURE);
        return false;
    }
    vumeter_properties_t* vp = dlsym(handle, "VuProperties");
    if (vp == NULL) {
        dlerror();
        dlclose(handle);
        error_printf(" failed to find symbol VuProperties in %s\n", path);
//        exit(EXIT_FAILURE);
        return false;
    }
    vp->handle = handle;
    if (vu_props_list == NULL) {
        vu_props_list = vp;
    } else {
        vp->next = vu_props_list;
        vu_props_list = vp;
    }
    return true;
}

void vumeter_widget_load_media(widget_t *wdgt, const char* resource_path) {
    vumeter_properties_t* base_props = vu_props_list;
    char buffer[1024];
    sprintf(buffer, "./images/runtime/%dx%d", wdgt->rect.w, wdgt->rect.h);
    vumeter_widget_t* vw = wdgt->sub.vu;
    vw->num_meters = 0;
    debug_printf("VU Meter widget:\n");
    debug_printf("          rect = {%4d,%4d,%4d,%4d}\n", wdgt->rect.x, wdgt->rect.y, wdgt->rect.w, wdgt->rect.h);
    while(NULL != base_props) {
        base_props->resource_path = VUMeter_resource_path(resource_path, base_props);
        vumeter_properties_t* props = base_props;
#ifdef  VUMETERS_CHECK_ON_INIT
        if (!VUMeter_load_media(wdgt->view->app->renderer, props)) {
            error_printf("failed to load media for %s\n", props->name);
            continue;
        }
#endif
#ifdef  VUMETERS_CHECK_ON_INIT
        VUMeter_unload_media(props);
#endif
        const vumeter_t* meter = props->vumeters;
        float decay_unit = (float)props->volume_levels/60;
        for(int ix = 0; ix < props->vumeter_count; ++ix, ++(vw->num_meters), ++meter) {
            _vw_meter_t* vwmeter_ptr = vw->meters + vw->num_meters;
            vwmeter_ptr->props = props;
            vwmeter_ptr->meter = meter;
            vwmeter_ptr->decay_unit = decay_unit;
            float scalef = VUMeter_scale_factor(base_props, wdgt->rect.w, wdgt->rect.h);
            //set x and y to 0, they will be changed appropriately when  the rectangle is centred
            vwmeter_ptr->vu_rect.x = vwmeter_ptr->vu_rect.y = 0;
            vwmeter_ptr->vu_rect.w = vwmeter_ptr->props->layout.w;
            vwmeter_ptr->vu_rect.h = vwmeter_ptr->props->layout.h;
            scale_rect_size(&vwmeter_ptr->vu_rect, &vwmeter_ptr->vu_rect, scalef);
            center_rect(&wdgt->rect, &vwmeter_ptr->vu_rect, &vwmeter_ptr->vu_rect);
            //TODO: for now scale factor for each channel is identical, and is stuffed inside channel parameter struct, which is visible and defined for vumeter_util.c
            for(int ix_chan=0; ix_chan < NUM_VU_CHANNELS; ++ix_chan) {
                vwmeter_ptr->channel_parms[ix_chan].scale_factor = scalef;
                scale_rect(
                        &vwmeter_ptr->props->layout.rects[ix_chan+1],
                        &vwmeter_ptr->channel_parms[ix_chan].channel_rect,
                        scalef);
                rebaseRect(&vwmeter_ptr->vu_rect,
                        &vwmeter_ptr->channel_parms[ix_chan].channel_rect,
                        &vwmeter_ptr->channel_parms[ix_chan].channel_rect
                        );
            }
            // TODO: make equidistant spacing selectable by widget property
            // TODO: support spacing distribution based on sequence of integers
            // TODO: support shared component
            // TODO: vertical spacing
            if (vwmeter_ptr->props->layout.arrangement == HORIZONTAL_ARRANGEMENT && vw->equal_horizontal_spacing)
            {
#define FUZZ_DOWN(v) 2*((v)/2)
                int lead = FUZZ_DOWN(vwmeter_ptr->channel_parms[0].channel_rect.x - vwmeter_ptr->vu_rect.x);
                int trail = FUZZ_DOWN(vwmeter_ptr->vu_rect.x + vwmeter_ptr->vu_rect.w - ( vwmeter_ptr->channel_parms[1].channel_rect.x +  vwmeter_ptr->channel_parms[1].channel_rect.w));
                int middle = FUZZ_DOWN(vwmeter_ptr->channel_parms[1].channel_rect.x - (vwmeter_ptr->channel_parms[0].channel_rect.x +  vwmeter_ptr->channel_parms[0].channel_rect.w));
                float avail = ( vwmeter_ptr->channel_parms[0].channel_rect.x 
                        + wdgt->rect.w - (vwmeter_ptr->channel_parms[1].channel_rect.x + vwmeter_ptr->channel_parms[1].channel_rect.w)
                        + middle);
                avail /= (lead+trail+middle);
                int l = avail*lead;
                int m = avail*middle;
//                int t = avail*trail;
                vwmeter_ptr->channel_parms[0].channel_rect.x = l;
                vwmeter_ptr->channel_parms[1].channel_rect.x = l + vwmeter_ptr->channel_parms[0].channel_rect.w + m;
#undef FUZZ_DOWN
            }
            debug_printf("    %d) meter:%s decay_unit:%f volume_levels:%d\n", vw->num_meters, vw->meters[vw->num_meters].meter->name, decay_unit, props->volume_levels);
        }
        base_props = base_props->next;
    }
    if (vw->num_meters) {
        if (!VUMeter_load_media(wdgt->view->app->renderer, vw->meters[vumeter_index(vw)].props)) {
            error_printf("failed to load media for %s\n",  vw->meters[vumeter_index(vw)].props->name);
        }
        wdgt->redraw_required = true;
    } else {
            error_printf("no VU meters loaded\n");
    }
}

static void vumeter_render_bg(widget_t* wdgt) {
    if (true) {
        vumeter_widget_t* vw = wdgt->sub.vu;
        VUMeter_draw_background(wdgt->view->app->renderer,
            vw->meters[vumeter_index(vw)].props,
            vw->meters[vumeter_index(vw)].meter,
            &vw->meters[vumeter_index(vw)].vu_rect,
            vw->meters[vumeter_index(vw)].channel_parms);
    }
    wdgt->redraw_required = false;
}

static void vumeter_render_fg(widget_t* wdgt) {
/*    
    SDL_Rect draw_rect;
    copyRect(&wdgt->rect, &draw_rect);
    translate_draw_rect(&draw_rect);
*/    
    vumeter_widget_t* vw = wdgt->sub.vu;
    int vols[2];
    visualizer_vumeter(vols);
    if (vw->num_meters) {
        if(vw->meters[vumeter_index(vw)].props->volume_levels != 49) {
            vols[0] = vols[0] * vw->meters[vumeter_index(vw)].props->volume_levels/50;
            vols[1] = vols[1] * vw->meters[vumeter_index(vw)].props->volume_levels/50;
        }
        VUMeter_draw_foreground(wdgt->view->app->renderer,
               vw->meters[vumeter_index(vw)].props,
               vw->meters[vumeter_index(vw)].meter, vols,
//               &draw_rect,
               &vw->meters[vumeter_index(vw)].vu_rect,
               vw->meters[vumeter_index(vw)].channel_parms,
               vw->vol_runtimes,
               vw->meters[vumeter_index(vw)].decay_unit);
    }
}

widget_t *widget_create_vumeter(const view_context_t* view) {
    widget_t* wdgt = widget_create(view);
    if (wdgt) {
        *((widget_type_t*)&wdgt->type) = WIDGET_VUMETER;
        wdgt->render_backdrop = vumeter_render_bg;
        wdgt->render_foreground = vumeter_render_fg;
        wdgt->sub.vu = calloc(1, sizeof(vumeter_widget_t));
        if (wdgt->sub.vu == NULL) {
            widget_destroy(wdgt);
            wdgt = NULL;
        }
    }
    return wdgt;
}

widget_t *vumeter_widget_destroy(widget_t *wdgt) {
    if (wdgt == NULL) {
        return wdgt;
    }
    vumeter_widget_t* vw = wdgt->sub.vu;
    if (vw) {
        for (int ix=0; ix < vw->num_meters; ++ix) {
            vw->meters[ix].props = NULL;
        }
        free(vw);
        wdgt->sub.vu = (void *)NULL;
    }
    return wdgt;
}

static bool vumeter_select(widget_t *wdgt, int indx) {
    vumeter_widget_t* vw = wdgt->sub.vu;
    if (indx < 0 || indx >= vw->num_meters) {
        return false;
    }
    perf_printf("\n");
    if (indx != vumeter_index(vw) && vw->num_meters)
    {
        // let texture cache handle release of textures on demand
        //VUMeter_unload_media(vw->meters[vumeter_index(vw)].props);
        vumeter_properties_t* props = vw->meters[indx].props;
        if (!VUMeter_load_media(wdgt->view->app->renderer, props)) {
            exit(EXIT_FAILURE);
        }
        wdgt->redraw_required = true;
    }
    vumeter_set_index(vw, indx);
    debug_printf("vumeter: %s\n", vw->meters[vumeter_index(vw)].meter->name);
    return true;
}

widget_t *widget_vumeter_select_next(widget_t *wdgt) {
    if (wdgt == NULL) {
        return wdgt;
    }
    vumeter_widget_t* vw = wdgt->sub.vu;
    if (vw->locked) {
        return wdgt;
    }
    if (vw->num_meters) { vumeter_select(wdgt, (vumeter_index(vw) + 1) % vw->num_meters); }
    return wdgt;
}

widget_t *widget_vumeter_select_prev(widget_t *wdgt) {
    if (wdgt == NULL) {
        return wdgt;
    }
    vumeter_widget_t* vw = wdgt->sub.vu;
    if (vw->locked) {
        return wdgt;
    }
    if (vw->num_meters) { vumeter_select(wdgt, vumeter_index(vw) == 0 ? vw->num_meters-1 : vumeter_index(vw) - 1); }
    return wdgt;
}

widget_t *widget_vumeter_select_by_name(widget_t *wdgt, const char* name) {
    if (wdgt == NULL) {
        return wdgt;
    }
    vumeter_widget_t* vw = wdgt->sub.vu;
    if (vw->locked || name == NULL) {
        return wdgt;
    }
    for (int indx=0; indx < vw->num_meters; ++indx) {
        if (0 == strcmp(name, vw->meters[indx].meter->name)) {
            vumeter_select(wdgt, indx);
        }
    }
    return wdgt;
}

widget_t *widget_vumeter_select_lock(widget_t *wdgt, bool lock) {
    vumeter_widget_t* vw = wdgt->sub.vu;
    vw->locked = lock;
    return wdgt;
}

widget_t *widget_vumeter_equal_horizontal_spacing(widget_t *wdgt, bool val) {
    vumeter_widget_t* vw = wdgt->sub.vu;
    vw->equal_horizontal_spacing = val;
    return wdgt;
}
