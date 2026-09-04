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
#include "vumeter.h"
#include "application.h"
#include "widgets_internal.h"
#include "visualizer.h"
#include "timing.h"

struct vumeter_widget_s_t {
    vumeter_instance_t meters[100];
    int     num_meters;
    int     atomic_meter_indx;
    bool    locked;
    bool    equal_horizontal_spacing;
    // only used by render code. 
    volatile int render_meter_indx;
};

static inline int vumeter_index(vumeter_widget_t* wdgt) {
    return  __atomic_load_n(&wdgt->atomic_meter_indx, __ATOMIC_ACQUIRE);
}

static inline void vumeter_set_index(vumeter_widget_t* wdgt, int ix) {
     __atomic_store_n(&wdgt->atomic_meter_indx, ix, __ATOMIC_RELEASE);
}


void vumeter_widget_load_media(widget_t *wdgt, const char* resource_path) {
    vumeter_widget_t* vw = wdgt->sub.vu;
    vw->num_meters = vumeter_populate_instance_array(vw->meters, ARRAYLEN(vw->meters));
    debug_printf("VU Meter widget:\n");
    debug_printf("          rect = {%4d,%4d,%4d,%4d}\n", wdgt->rect.x, wdgt->rect.y, wdgt->rect.w, wdgt->rect.h);
    for(int ix = 0; ix < vw->num_meters; ++ix) {
        vumeter_instance_t* vumtr = vw->meters + ix;
        vumeter_setup(vumtr, &wdgt->rect, vw->equal_horizontal_spacing);
    }
    if (vw->num_meters) {
        if (!vu_meters_load_media(wdgt->view->app->renderer, vw->meters[vumeter_index(vw)].vss)) {
            error_printf("failed to load media for %s\n",  vw->meters[vumeter_index(vw)].vss->spec->name);
        }
        wdgt->redraw_required = true;
    } else {
        error_printf("no VU meters loaded\n");
    }
}

void vumeter_widget_unload_media(widget_t *wdgt, const char* resource_path) {
}

static void vumeter_render_bg(widget_t* wdgt) {
    vumeter_widget_t* vw = wdgt->sub.vu;
    if (vw->num_meters) {
        int index = vumeter_index(vw);
        if (index < 0 || index >= vw->num_meters) {
            error_printf("vumeter_render_bg: invalid vu meter index %d, resetting to 0\n", index);
            vumeter_set_index(vw, 0);
            index = 0;
        }
        vw->render_meter_indx = index;
        vumeter_render_background(wdgt->view->app->renderer, vw->meters + index);
    }
    wdgt->redraw_required = false;
}

static void vumeter_render_fg(widget_t* wdgt) {
    vumeter_widget_t* vw = wdgt->sub.vu;
    if (vw->num_meters) {
        vumeter_instance_t* vumtr = vw->meters + vw->render_meter_indx;
        update_volume_levels(vumtr->decay_unit); 
        vumeter_render_foreground(wdgt->view->app->renderer, vumtr, get_runtime_volume());
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
    FREE(wdgt->sub.vu);
    return wdgt;
}

static bool vumeter_select(widget_t *wdgt, int indx) {
    vumeter_widget_t* vw = wdgt->sub.vu;
    if (indx < 0 || indx >= vw->num_meters) {
        error_printf("vumeter_select: invalid index %d\n", indx);
        return false;
    }
    perf_printf("\n");
    if (indx != vumeter_index(vw) && vw->num_meters)
    {
        // let texture cache handle release of textures on demand
        if (!vu_meters_load_media(wdgt->view->app->renderer, vw->meters[indx].vss)) {
            error_printf("failed to load VUMeter media, retrying in 1 second!");
            sleep_milli_seconds(1000);
            if (!vu_meters_load_media(wdgt->view->app->renderer, vw->meters[indx].vss)) {
                unsigned texture_bytes = tcache_get_texture_bytes_count();
                unsigned surface_bytes = tcache_get_surface_bytes_count();
                error_printf("failed to load VUMeter media: texture cache memory: texture:%u %fMiB surface:%u %fMib\n", texture_bytes, (float)texture_bytes/(1024*1024), surface_bytes, (float)surface_bytes/(1024*1024));
            }
//            exit(EXIT_FAILURE);
        }
        wdgt->redraw_required = true;
    }
    vumeter_set_index(vw, indx);
    debug_printf("vumeter: %s\n", vw->meters[vumeter_index(vw)].defn->name);
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
        if (0 == strcmp(name, vw->meters[indx].defn->name)) {
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
