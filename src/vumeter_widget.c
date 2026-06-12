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
#include "widgets.h"
#include "visualizer.h"

static vumeter_properties_t* vu_props_list;

//static struct {
//    vumeter_properties_t *props;
//    const vumeter_t* meter;
//} meters[100];
//static int num_meters=0;

struct vumeter_widget {
    vumeter_properties_t* props;
    struct {
        vumeter_properties_t    *props;
        const                   vumeter_t* meter;
        vu_channel_params_t     channel_parms[NUM_VU_CHANNELS];
        float                   decay_unit;
        // rectangle to scale and centred within the widget rectangle
        SDL_Rect                rect;
    } meters[100];
    int     num_meters;
    int     atomic_meter_indx;
    bool    locked;
    runtime_volume_t vol_runtimes[NUM_VU_CHANNELS];
};

static inline int vumeter_index(vumeter_widget* wdgt) {
    return  __atomic_load_n(&wdgt->atomic_meter_indx, __ATOMIC_ACQUIRE);
}

static inline void vumeter_set_index(vumeter_widget* wdgt, int ix) {
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

void vumeter_widget_load_media(widget *wdgt, const char* resource_path) {
    vumeter_properties_t* base_props = vu_props_list;
    char buffer[1024];
    sprintf(buffer, "./images/runtime/%dx%d", wdgt->rect.w, wdgt->rect.h);
    vumeter_widget* vw = wdgt->sub.vu;
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
/*        
        SDL_Rect draw_rect;
        copyRect(&wdgt->rect, &draw_rect);
        translate_draw_rect(&draw_rect);
*/        
#ifdef  VUMETERS_CHECK_ON_INIT
        VUMeter_unload_media(props);
#endif
        const vumeter_t* meter = props->vumeters;
        float decay_unit = (float)props->volume_levels/60;
        for(int ix = 0; ix < props->vumeter_count; ++ix, ++(vw->num_meters), ++meter) {
            vw->meters[vw->num_meters].props = props;
            vw->meters[vw->num_meters].meter = meter;
            vw->meters[vw->num_meters].decay_unit = decay_unit;
            float scalef = VUMeter_scale_factor(base_props, wdgt->rect.w, wdgt->rect.h);
            //set x and y to 0, they will be changed appropriately when  the rectangle is centred
            vw->meters[vw->num_meters].rect.x = vw->meters[vw->num_meters].rect.y = 0;
            vw->meters[vw->num_meters].rect.w = props->w;
            vw->meters[vw->num_meters].rect.h = props->h;
            scale_rect_size(&vw->meters[vw->num_meters].rect, &vw->meters[vw->num_meters].rect, scalef);
            center_rect(&wdgt->rect, &vw->meters[vw->num_meters].rect, &vw->meters[vw->num_meters].rect);
            //TODO: for now scale factor for each channel is identical, and s stuffed inside channel parameter struct, which is visible and defined for vumeter_util.c
            for(int ix_chan=0; ix_chan < NUM_VU_CHANNELS; ++ix_chan) {
                vw->meters[vw->num_meters].channel_parms[ix_chan].scale_factor = scalef;
            }
            debug_printf("    %d) meter:%s decay_unit:%f volume_levels:%d\n", vw->num_meters, vw->meters[vw->num_meters].meter->name, decay_unit, props->volume_levels);
        }
        base_props = base_props->next;
    }
    if (vw->num_meters) {
        if (!VUMeter_load_media(wdgt->view->app->renderer, vw->meters[vumeter_index(vw)].props)) {
            error_printf("failed to load media for %s\n",  vw->meters[vumeter_index(vw)].props->name);
        }
    } else {
            error_printf("no VU meters loaded\n");
    }
}

extern void _debug_draw_rect(widget* wdgt);
extern void _show_draw_rect(widget* wdgt);
extern void _show_input_rect(widget* wdgt);

static void vumeter_render(widget* wdgt) {
    if (debug_rects) { _debug_draw_rect(wdgt); }
    if (widget_highlight(wdgt)) {
        if (show_rects) { _show_draw_rect(wdgt); }
        if (show_input_rects) { _show_input_rect(wdgt); }
    }
/*    
    SDL_Rect draw_rect;
    copyRect(&wdgt->rect, &draw_rect);
    translate_draw_rect(&draw_rect);
*/    
    vumeter_widget* vw = wdgt->sub.vu;
    int vols[2];
    visualizer_vumeter(vols);
    if (vw->num_meters) {
        if(vw->meters[vumeter_index(vw)].props->volume_levels != 49) {
            vols[0] = vols[0] * vw->meters[vumeter_index(vw)].props->volume_levels/50;
            vols[1] = vols[1] * vw->meters[vumeter_index(vw)].props->volume_levels/50;
        }
        VUMeter_draw(wdgt->view->app->renderer,
               vw->meters[vumeter_index(vw)].props,
               vw->meters[vumeter_index(vw)].meter, vols,
//               &draw_rect,
               &vw->meters[vumeter_index(vw)].rect,
               vw->meters[vumeter_index(vw)].channel_parms,
               vw->vol_runtimes,
               vw->meters[vumeter_index(vw)].decay_unit);
    }
}

widget *widget_create_vumeter(const view_context* view) {
    widget* wdgt = calloc(sizeof(*wdgt), 1);
    if (wdgt) {
        wdgt->view = view;
        wdgt->action = ACTION_NONE;
        wdgt->render = vumeter_render;
        wdgt->sub.vu = calloc(1, sizeof(vumeter_widget));
        if (wdgt->sub.vu == NULL) {
            widget_destroy(wdgt);
            wdgt = NULL;
        } else {
            *((widget_type*)&wdgt->type) = WIDGET_VUMETER;
            if (view->list) {
                wdgt->next = &view->list->tail;
                wdgt->prev = view->list->tail.prev;
                wdgt->prev->next = wdgt->next->prev = wdgt;
            }
        }
    }
    return wdgt;
}

widget *vumeter_widget_destroy(widget *wdgt) {
    if (wdgt == NULL) {
        return wdgt;
    }
    vumeter_widget* vw = wdgt->sub.vu;
    if (vw) {
        for (int ix=0; ix < vw->num_meters; ++ix) {
            vw->meters[ix].props = NULL;
        }
        free(vw);
        wdgt->sub.vu = (void *)NULL;
    }
    return wdgt;
}

static bool vumeter_select(widget *wdgt, int indx) {
    vumeter_widget* vw = wdgt->sub.vu;
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
    }
    vumeter_set_index(vw, indx);
    debug_printf("vumeter: %s\n", vw->meters[vumeter_index(vw)].meter->name);
    return true;
}

widget *widget_vumeter_select_next(widget *wdgt) {
    if (wdgt == NULL) {
        return wdgt;
    }
    vumeter_widget* vw = wdgt->sub.vu;
    if (vw->locked) {
        return wdgt;
    }
    if (vw->num_meters) { vumeter_select(wdgt, (vumeter_index(vw) + 1) % vw->num_meters); }
    return wdgt;
}

widget *widget_vumeter_select_prev(widget *wdgt) {
    if (wdgt == NULL) {
        return wdgt;
    }
    vumeter_widget* vw = wdgt->sub.vu;
    if (vw->locked) {
        return wdgt;
    }
    if (vw->num_meters) { vumeter_select(wdgt, vumeter_index(vw) == 0 ? vw->num_meters-1 : vumeter_index(vw) - 1); }
    return wdgt;
}

widget *widget_vumeter_select_by_name(widget *wdgt, const char* name) {
    if (wdgt == NULL) {
        return wdgt;
    }
    vumeter_widget* vw = wdgt->sub.vu;
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

widget *widget_vumeter_select_lock(widget *wdgt, bool lock) {
    vumeter_widget* vw = wdgt->sub.vu;
    vw->locked = lock;
    return wdgt;
}


