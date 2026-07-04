#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include "application.h"
#include "widgets_internal.h"
#include "actions.h"
#include "util.h"
#include "logging.h"
#include "timing.h"

//FIXME: duplicate of var in widgets.c
static SDL_RendererFlip flip = SDL_FLIP_NONE;

static inline void free_ex(void** tgt) {
    if (*tgt) {
        free(*tgt);
    }
    *tgt = NULL;
}

#define FREE(x) free_ex((void **)(&x))


static inline bool _is_slider(widget_t* wdgt) {
    return (wdgt && (wdgt->type == WIDGET_SLIDER || wdgt->type == WIDGET_VSLIDER));
}

static inline bool slider_is_interactive(widget_t* wdgt) {
    return wdgt->sub.slider.defined_interactive && __atomic_load_n(&wdgt->sub.slider.interactive, __ATOMIC_ACQUIRE);
}

static inline bool slider_wk_is_initialised(widget_t* wdgt) {
    return __atomic_load_n(&wdgt->sub.slider.wk.initialised, __ATOMIC_ACQUIRE);
}

static inline void slider_set_wk_initialised(widget_t* wdgt, bool yn) {
    __atomic_store_n(&wdgt->sub.slider.wk.initialised, yn, __ATOMIC_RELEASE);
}

// !!! DO NOT invoke from render thread !!!
static _slider_workspace_t* hslider_widget_configure(widget_t* wdgt) {
    // FIXME assert widget is a slider
    if (!wdgt->configured) {
        return &wdgt->sub.slider.wk;
    }

    if (!slider_wk_is_initialised(wdgt)) {
        _slider_resource_t* pick = wdgt->sub.slider.res+SLIDER_PICK;
        _slider_workspace_t* wk = &wdgt->sub.slider.wk;

        copyRect(&wdgt->rect, &wk->bar_start_rect);
        copyRect(&wdgt->rect, &wk->bar_end_rect);
        copyRect(&wdgt->rect, &wk->bar_rect);
        copyRect(&wdgt->rect, &wk->pick_rect);

         _slider_resource_t* bar_start = wdgt->sub.slider.res+SLIDER_BAR_START;
         _slider_resource_t* bar_end =  wdgt->sub.slider.res+SLIDER_BAR_END;

        wk->value_range_delta = wdgt->sub.slider.range.end - wdgt->sub.slider.range.start;
        // FIXME: pick-w>%2 != 0
        if (slider_is_interactive(wdgt)) {
            wk->pick_rect.w = pick->w;
            wk->half_pick_dim = pick->w/2;
        } else {
            wk->pick_rect.w = 0;
            wk->half_pick_dim = 0;
        }

        wk->half_pick_dim = slider_is_interactive(wdgt) ? pick->w/2 : 0;
        wk->min_pos = wdgt->rect.x + (wk->half_pick_dim > bar_start->w ? wk->half_pick_dim : bar_start->w);
        wk->max_pos = wdgt->rect.x + wdgt->rect.w - (wk->half_pick_dim > bar_end->w ? wk->half_pick_dim : bar_end->w);

        wk->current_pos = wk->min_pos;

        wk->bar_start_rect.w = bar_start->w;
        wk->bar_start_rect.x = wk->min_pos - wk->bar_start_rect.w;
        translate_image_rect(&wk->bar_start_rect);

        wk->bar_end_rect.w = bar_end->w;
        wk->bar_end_rect.x = wk->max_pos;
        translate_image_rect(&wk->bar_end_rect);

        wk->bar_rect.x = wk->min_pos;
        wk->bar_rect.w = wk->max_pos - wk->min_pos;
        translate_image_rect(&wk->bar_end_rect);

        // restore input rectangle y extents.
        wdgt->input_rect.y = wdgt->rect.y;
        wdgt->input_rect.h = wdgt->rect.h;
        // then set input rectangle y extents to match the pick height spec,
        // typically this narrows (vertically) the area of the widget sensitivity
        if (pick->h > 0) {
            wdgt->input_rect.y = wdgt->rect.y + (wdgt->rect.h-pick->h)/2;
            wdgt->input_rect.h = pick->h;
        }
        slider_set_wk_initialised(wdgt, true);
#if 0
log_printf("HSLIDER: %p\n", wdgt);
log_printf("        bar   : %4d,%4d ; %2d,%2d\n",  wk->bar_rect.x,  wk->bar_rect.y ,  wk->bar_rect.w,   wk->bar_rect.h); 
log_printf("        start : %4d,%4d ; %2d,%2d\n",  wk->bar_start_rect.x,  wk->bar_start_rect.y ,  wk->bar_start_rect.w,   wk->bar_start_rect.h); 
log_printf("        end   : %4d,%4d ; %2d,%2d\n",  wk->bar_end_rect.x,  wk->bar_end_rect.y ,  wk->bar_end_rect.w,   wk->bar_end_rect.h); 
log_printf("        pick  : %4d,%4d ; %2d,%2d\n",  wk->pick_rect.x,  wk->pick_rect.y ,  wk->pick_rect.w,   wk->pick_rect.h); 
log_printf("        inpu  : %4d,%4d ; %2d,%2d\n",  wdgt->input_rect.x,  wdgt->input_rect.y ,  wdgt->input_rect.w,   wdgt->input_rect.h); 
#endif
    }

    return &wdgt->sub.slider.wk;
}

static inline _slider_workspace_t*  slider_reconfigure(widget_t* wdgt) {
    slider_set_wk_initialised(wdgt, false);
    return slider_widget_configure(wdgt);
}


static void hslider_widget_render(widget_t* wdgt) {
    wdgt->redraw_required = false;
    if (!slider_wk_is_initialised(wdgt)) { 
        return;
    }
    const _slider_workspace_t* wk = &wdgt->sub.slider.wk;
    if (wk->value_range_delta < 1) {
        // 0 or negative range => nothing to render
        return;
    }

    _slider_resource_t* pick = wdgt->sub.slider.res+SLIDER_PICK;
    SDL_Rect fill_rect;
    SDL_Rect empty_rect;
    SDL_Rect pick_rect;
    copyRect(&wk->bar_rect, &fill_rect);
    copyRect(&wk->bar_rect, &empty_rect);
    copyRect(&wk->pick_rect, &pick_rect);

    SDL_Rect start_rect;
    SDL_Rect end_rect;
    copyRect(&wk->bar_start_rect, &start_rect);
    copyRect(&wk->bar_end_rect, &end_rect);
    int curr_pos = wk->current_pos;
    if (slider_is_interactive(wdgt) && widget_pressed(wdgt)) {
        // FIXME: need offset from down pos on the pick itself?
        curr_pos = wk->drag_pos;
    }

    fill_rect.w = curr_pos - wk->bar_rect.x - wk->half_pick_dim;
    empty_rect.x = curr_pos + wk->half_pick_dim;
    empty_rect.w = wk->max_pos - empty_rect.x;

    pick_rect.x =  curr_pos - wk->half_pick_dim;
    start_rect.w = MIN(pick_rect.x - start_rect.x, start_rect.w);

    {
        int ix_texture = wk->min_pos < curr_pos? 0: 1;
        _slider_resource_t* bar_start = wdgt->sub.slider.res[SLIDER_BAR_START].texture_ids[ix_texture]? wdgt->sub.slider.res+SLIDER_BAR_START:NULL;
        if (bar_start && start_rect.w > 0) {
            SDL_RenderCopyEx(wdgt->view->app->renderer,
                   tcache_quick_get_texture(bar_start->texture_ids[ix_texture], wdgt->view->app->renderer),
                   NULL, &start_rect,
//                   wdgt->view->app->orientation,
                   0.0,
                   NULL, flip);
        }
    }

    {
        int pick_x2 = pick_rect.x + pick_rect.w;
        int srcrect_x = 0;
        if (pick_x2 > end_rect.x) {
            srcrect_x = pick_x2 - end_rect.x;
            end_rect.w -=  srcrect_x;
            end_rect.x = pick_x2;
        }
        // TODO: src_x should feed into srcrect when invoking SDL_RenderCopyEx below
        // however the raw value cannot be used, it has to be scaled to the bar end image
        // size. For now not doing turns out to work good enough.
        int ix_texture = curr_pos < wk->max_pos? 1: 0;
        _slider_resource_t* bar_end = wdgt->sub.slider.res[SLIDER_BAR_END].texture_ids[ix_texture]? wdgt->sub.slider.res+SLIDER_BAR_END:NULL;
        if (bar_end && end_rect.w > 0) {
            SDL_RenderCopyEx(wdgt->view->app->renderer,
                   tcache_quick_get_texture(bar_end->texture_ids[ix_texture], wdgt->view->app->renderer),
                   NULL, &end_rect,
//                   wdgt->view->app->orientation,
                   0.0,
                   NULL, flip);
        }
    }

    _slider_resource_t* bar = wdgt->sub.slider.res[SLIDER_BAR].texture_ids[0]? wdgt->sub.slider.res+SLIDER_BAR:NULL;
    if (bar && fill_rect.w > 0) {
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(bar->texture_ids[0], wdgt->view->app->renderer),
                NULL, &fill_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }

    if (pick_rect.w && pick_rect.h && slider_is_interactive(wdgt)) {
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(pick->texture_ids[0], wdgt->view->app->renderer),
                NULL, &pick_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }

    if (bar && empty_rect.w > 0) {
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(bar->texture_ids[1], wdgt->view->app->renderer),
                NULL, &empty_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }
}

widget_t *widget_create_slider(const view_context_t* view) {
    widget_t* wdgt = widget_create(view);
    if (wdgt) {
        *((widget_type_t*)&wdgt->type) = WIDGET_SLIDER;
        wdgt->action = ACTION_NONE;
        wdgt->render_backdrop = hslider_widget_render;
        __atomic_store_n(&wdgt->sub.slider.interactive, true, __ATOMIC_RELEASE);
        wdgt->sub.slider.defined_interactive = true;
    }
    return wdgt;
}

widget_t *widget_slider_image_paths(widget_t* wdgt, slider_reosurce_ID_t id, const char* path1, const char* path2) {
    if (_is_slider(wdgt)) {
        switch(id) {
            case SLIDER_BAR:
            case SLIDER_PICK:
            case SLIDER_BAR_END:
            case SLIDER_BAR_START:
                for(int ix=0; ix < sizeof(wdgt->sub.slider.res[id].image_paths)/sizeof(wdgt->sub.slider.res[id].image_paths[0]); ++ix) {
                    if (wdgt->sub.slider.res[id].image_paths[ix] != NULL) {
                        FREE(wdgt->sub.slider.res[id].image_paths[ix]);
                    }
                }
                if (path1) {
                    wdgt->sub.slider.res[id].image_paths[0] = strdup(path1);
                }
                if (path2) {
                    wdgt->sub.slider.res[id].image_paths[1] = strdup(path2);
                }
                break;
            case SLIDER_RESOURCE_COUNT:
            default:
                error_printf("widget_slider_image_path: unknown resource id %d\n", id);
                break;
        }
        slider_reconfigure(wdgt);
    }
    return wdgt;
}


static widget_t *hslider_image_width(widget_t* wdgt, slider_reosurce_ID_t id, int width) {
    if (wdgt) {
        bool updated = false;
        switch(id) {
            case SLIDER_PICK:
                updated = wdgt->sub.slider.res[id].w != MAX(width, 2);
                wdgt->sub.slider.res[id].w = MAX(width, 2);
                break;
            case SLIDER_BAR_END:
            case SLIDER_BAR_START:
                updated = wdgt->sub.slider.res[id].w != width;
                wdgt->sub.slider.res[id].w = width;
                break;
            case SLIDER_BAR:
                error_printf("widget_slider_image_width: ignoring width setting for slider bar resource id %d\n", id);
                break;
            case SLIDER_RESOURCE_COUNT:
            default:
                error_printf("widget_slider_image_width: unknown resource id %d\n", id);
                break;
        }
        if (updated) {
            slider_reconfigure(wdgt);
        }
    }
    return wdgt;
}

static widget_t *hslider_image_height(widget_t* wdgt, slider_reosurce_ID_t id, int height) {
    if (wdgt) {
        bool updated = false;
        switch(id) {
            case SLIDER_PICK:
            case SLIDER_BAR_END:
            case SLIDER_BAR_START:
            case SLIDER_BAR:
                updated = wdgt->sub.slider.res[id].h != height;
                wdgt->sub.slider.res[id].h = height;
                break;
            case SLIDER_RESOURCE_COUNT:
            default:
                error_printf("widget_slider_image_height: unknown resource id %d\n", id);
                break;
        }
        if (updated) {
            slider_reconfigure(wdgt);
        }
    }
    return wdgt;
}

widget_t *widget_slider_track(widget_t* wdgt, const SDL_Point *pt) {
    if (_is_slider(wdgt)) {
        if (slider_is_interactive(wdgt)) {
            if (widget_pressed(wdgt) && (wdgt->sub.slider.range.end - wdgt->sub.slider.range.start) > 0) {
                _slider_resource_t* pick = wdgt->sub.slider.res+SLIDER_PICK;
                _slider_workspace_t* wk = &wdgt->sub.slider.wk;
                if (pick) {
                    const int* ppos = wdgt->type == WIDGET_SLIDER ? &pt->x : &pt->y;
                    if (*ppos < wk->min_pos) {
                        wk->drag_pos = wk->min_pos;
                    } else if (*ppos > wk->max_pos) {
                        wk->drag_pos = wk->max_pos;
                    } else {
                        wk->drag_pos = *ppos;
                    }
                }
                wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hotspot;
            }
        }
    }
    return wdgt;
}

widget_t *widget_slider_tracking_commit(widget_t* wdgt, const SDL_Point *pt) {
    if (_is_slider(wdgt)) {
        if (slider_is_interactive(wdgt)) {
            widget_slider_track(wdgt, pt);
            _slider_workspace_t* wk = &wdgt->sub.slider.wk;
            wk->current_pos = wk->drag_pos;
        }
    }
    return wdgt;
}

widget_t *widget_slider_update_value(widget_t* wdgt, int value) {
    if (_is_slider(wdgt)) {
        if (value >= wdgt->sub.slider.range.start
                && value <= wdgt->sub.slider.range.end) {
            if (!slider_wk_is_initialised(wdgt)) {
                 slider_reconfigure(wdgt);
            }
            if (slider_wk_is_initialised(wdgt)) {
                _slider_workspace_t* wk = &wdgt->sub.slider.wk;
                if (wk->value_range_delta) {
                    // range must be non-zero to calculate the position of the pick
                    float offset = ((float)(value - wdgt->sub.slider.range.start)*(wk->max_pos - wk->min_pos))/wk->value_range_delta;
                    bool updated;
                    if (wdgt->type == WIDGET_SLIDER) {
                        // Horizontal sliders "naturally" progress from left (smaller offset values) to right (larger offset values)
                        updated = wk->current_pos != wk->min_pos + (int)offset;
                        wk->current_pos = wk->min_pos + (int)offset;
                    } else {
                        // Vertical sliders "naturally" progress from bottom (larger offset values) to top (smaller offset values)
                        updated = wk->current_pos != wk->max_pos - (int)offset;
                        wk->current_pos = wk->max_pos - (int)offset;
                    }
                    wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hotspot && updated;
                }
            } else {
                error_printf("widget_slider_update_value: workspace is uninitialised\n");
            }
        } else {
            error_printf("widget_slider_update_value: %d not in range %d-%d\n",
                    value,
                    wdgt->sub.slider.range.start,
                    wdgt->sub.slider.range.end);
        }
    }
    return wdgt;
}

widget_t *widget_slider_range(widget_t* wdgt, int start, int end) {
    if (_is_slider(wdgt)) {
        bool updated = wdgt->sub.slider.range.start != start || wdgt->sub.slider.range.end != end;
        wdgt->sub.slider.range.start = start;
        wdgt->sub.slider.range.end = end;
        if (updated) {
            slider_reconfigure(wdgt);
            // reset slider position
            wdgt->sub.slider.wk.current_pos = wdgt->sub.slider.wk.min_pos;
        }
        wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hotspot;
    }
    return wdgt;
}

widget_t *widget_slider_set_interactive(widget_t* wdgt, bool yn) {
    if (_is_slider(wdgt)) {
        bool ny = ! yn;
        bool modified = __atomic_compare_exchange_n(&wdgt->sub.slider.interactive, &ny, yn, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
        if (modified) {
            int value;
            widget_slider_get_value(wdgt, &value);
            slider_reconfigure(wdgt);
            widget_slider_update_value(wdgt, value);
            wdgt->redraw_required = true;
        }
    }
    return wdgt;
}

widget_t *widget_slider_define_interactive(widget_t* wdgt, bool yn) {
    if (_is_slider(wdgt)) {
        wdgt->sub.slider.defined_interactive = yn;
    }
    return wdgt;
}


widget_t *widget_slider_get_value(widget_t* wdgt, int* value) {
    if (_is_slider(wdgt)) {
        const _slider_workspace_t* wk = &wdgt->sub.slider.wk;
        // horizontal sliders progress from left (lower position values) to right (higher position values)
        // whereas 
        // vertical sliders progress from bottom (higher position values) to top (lower position values)
        int v = wdgt->type == WIDGET_SLIDER ? (wk->current_pos - wk->min_pos) : (wk->max_pos - wk->current_pos);
        dummy_printf("widget_slider_get_value v=%d %s\n", v, widget_get_type_name(wdgt));
        v *= wk->value_range_delta;
        v /= (wk->max_pos - wk->min_pos);
        *value = wdgt->sub.slider.range.start + v;
    }
    return wdgt;
}

// vertical slider
// !!! DO NOT invoke from render thread !!!
static _slider_workspace_t* vslider_widget_configure(widget_t* wdgt) {
    if (!slider_wk_is_initialised(wdgt)) {
        _slider_resource_t* pick = wdgt->sub.slider.res+SLIDER_PICK;
        _slider_workspace_t* wk = &wdgt->sub.slider.wk;

        copyRect(&wdgt->rect, &wk->bar_start_rect);
        copyRect(&wdgt->rect, &wk->bar_end_rect);
        copyRect(&wdgt->rect, &wk->bar_rect);
        copyRect(&wdgt->rect, &wk->pick_rect);

         _slider_resource_t* bar_start = wdgt->sub.slider.res+SLIDER_BAR_START;
         _slider_resource_t* bar_end =  wdgt->sub.slider.res+SLIDER_BAR_END;

        wk->value_range_delta = wdgt->sub.slider.range.end - wdgt->sub.slider.range.start;
        // FIXME: pick-h>%2 != 0
        if (slider_is_interactive(wdgt)) {
            wk->pick_rect.h = pick->h;
            wk->half_pick_dim = pick->h/2;
        } else {
            wk->pick_rect.h = 0;
            wk->half_pick_dim = 0;
        }

        wk->half_pick_dim = slider_is_interactive(wdgt) ? pick->h/2 : 0;
        wk->min_pos = wdgt->rect.y + (wk->half_pick_dim > bar_start->h ? wk->half_pick_dim : bar_start->h);
        wk->max_pos = wdgt->rect.y + wdgt->rect.h - (wk->half_pick_dim > bar_end->h ? wk->half_pick_dim : bar_end->h);

        wk->current_pos = wk->min_pos;

        wk->bar_start_rect.h = bar_start->h;
        wk->bar_start_rect.y = wk->min_pos - wk->bar_start_rect.h;
        translate_image_rect(&wk->bar_start_rect);

        wk->bar_end_rect.h = bar_end->h;
        wk->bar_end_rect.y = wk->max_pos;
        translate_image_rect(&wk->bar_end_rect);

        wk->bar_rect.y = wk->min_pos;
        wk->bar_rect.h = wk->max_pos - wk->min_pos;
        translate_image_rect(&wk->bar_end_rect);

        // restore input rectangle y extents.
        wdgt->input_rect.x = wdgt->rect.x;
        wdgt->input_rect.w = wdgt->rect.w;
        // then set input rectangle y extents to match the pick height spec,
        // typically this narrows (horizontally) the area of the widget sensitivity
        if (pick->h > 0) {
            wdgt->input_rect.x = wdgt->rect.x + (wdgt->rect.w-pick->w)/2;
            wdgt->input_rect.w = pick->w;
        }
        slider_set_wk_initialised(wdgt, true);
#if 0        
log_printf("VSLIDER: %p\n", wdgt);
log_printf("        bar   : %4d,%4d ; %2d,%2d\n",  wk->bar_rect.x,  wk->bar_rect.y ,  wk->bar_rect.w,   wk->bar_rect.h); 
log_printf("        start : %4d,%4d ; %2d,%2d\n",  wk->bar_start_rect.x,  wk->bar_start_rect.y ,  wk->bar_start_rect.w,   wk->bar_start_rect.h); 
log_printf("        end   : %4d,%4d ; %2d,%2d\n",  wk->bar_end_rect.x,  wk->bar_end_rect.y ,  wk->bar_end_rect.w,   wk->bar_end_rect.h); 
log_printf("        pick  : %4d,%4d ; %2d,%2d\n",  wk->pick_rect.x,  wk->pick_rect.y ,  wk->pick_rect.w,   wk->pick_rect.h); 
log_printf("        inpu  : %4d,%4d ; %2d,%2d\n",  wdgt->input_rect.x,  wdgt->input_rect.y ,  wdgt->input_rect.w,   wdgt->input_rect.h);
#endif
    }

    return &wdgt->sub.slider.wk;
}

static void vslider_widget_render(widget_t* wdgt) {
    wdgt->redraw_required = false;
    if (!slider_wk_is_initialised(wdgt)) { 
        return;
    }
    const _slider_workspace_t* wk = &wdgt->sub.slider.wk;
    if (wk->value_range_delta < 1) {
        // 0 or negative range => nothing to render
        return;
    }

    _slider_resource_t* pick = wdgt->sub.slider.res+SLIDER_PICK;
    SDL_Rect fill_rect;
    SDL_Rect empty_rect;
    SDL_Rect pick_rect;
    copyRect(&wk->bar_rect, &fill_rect);
    copyRect(&wk->bar_rect, &empty_rect);
    copyRect(&wk->pick_rect, &pick_rect);

    SDL_Rect start_rect;
    SDL_Rect end_rect;
    copyRect(&wk->bar_start_rect, &start_rect);
    copyRect(&wk->bar_end_rect, &end_rect);
    int curr_pos = wk->current_pos;
    if (slider_is_interactive(wdgt) && widget_pressed(wdgt)) {
        // FIXME: need offset from down pos on the pick itself?
        curr_pos = wk->drag_pos;
    }

    empty_rect.h = curr_pos - wk->bar_rect.y - wk->half_pick_dim;
    fill_rect.y = curr_pos + wk->half_pick_dim;
    fill_rect.h = wk->max_pos - fill_rect.y;

    pick_rect.y =  curr_pos - wk->half_pick_dim;
    start_rect.h = MIN(pick_rect.y - start_rect.y, start_rect.h);

    {
//        int ix_texture = wk->min_pos < curr_pos? 0: 1;
        int ix_texture = curr_pos < wk->max_pos? 1: 0;
        _slider_resource_t* bar_start = wdgt->sub.slider.res[SLIDER_BAR_START].texture_ids[ix_texture]? wdgt->sub.slider.res+SLIDER_BAR_START:NULL;
        if (bar_start && start_rect.h > 0) {
            SDL_RenderCopyEx(wdgt->view->app->renderer,
                   tcache_quick_get_texture(bar_start->texture_ids[ix_texture], wdgt->view->app->renderer),
                   NULL, &start_rect,
//                   wdgt->view->app->orientation,
                   0.0,
                   NULL, flip);
        }
    }
 
    {
        int pick_y2 = pick_rect.y + pick_rect.h;
        int srcrect_y = 0;
        if (pick_y2 > end_rect.y) {
            srcrect_y = pick_y2 - end_rect.y;
            end_rect.h -=  srcrect_y;
            end_rect.y = pick_y2;
        }
        // TODO: src_y should feed into srcrect when invoking SDL_RenderCopyEx below
        // however the raw value cannot be used, it has to be scaled to the bar end image
        // size. For now not doing turns out to work good enough.
//        int ix_texture = curr_pos < wk->max_pos? 1: 0;
        int ix_texture = wk->min_pos < curr_pos? 0: 1;
        _slider_resource_t* bar_end = wdgt->sub.slider.res[SLIDER_BAR_END].texture_ids[ix_texture]? wdgt->sub.slider.res+SLIDER_BAR_END:NULL;
        if (bar_end && end_rect.h > 0) {
            SDL_RenderCopyEx(wdgt->view->app->renderer,
                   tcache_quick_get_texture(bar_end->texture_ids[ix_texture], wdgt->view->app->renderer),
                   NULL, &end_rect,
//                   wdgt->view->app->orientation,
                   0.0,
                   NULL, flip);
        }
    }

    _slider_resource_t* bar = wdgt->sub.slider.res[SLIDER_BAR].texture_ids[0]? wdgt->sub.slider.res+SLIDER_BAR:NULL;
    if (bar && fill_rect.h > 0) {
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(bar->texture_ids[0], wdgt->view->app->renderer),
                NULL, &fill_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }

    if (pick_rect.w && pick_rect.h && slider_is_interactive(wdgt)) {
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(pick->texture_ids[0], wdgt->view->app->renderer),
                NULL, &pick_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }

    if (bar && empty_rect.h > 0) {
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(bar->texture_ids[1], wdgt->view->app->renderer),
                NULL, &empty_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }
}

widget_t *widget_create_vslider(const view_context_t* view) {
    widget_t* wdgt = widget_create(view);
    if (wdgt) {
        *((widget_type_t*)&wdgt->type) = WIDGET_VSLIDER;
        wdgt->action = ACTION_NONE;
        wdgt->render_backdrop = vslider_widget_render;
        __atomic_store_n(&wdgt->sub.slider.interactive, true, __ATOMIC_RELEASE);
        wdgt->sub.slider.defined_interactive = true;
    }
    return wdgt;
}

static widget_t *vslider_image_height(widget_t* wdgt, slider_reosurce_ID_t id, int height) {
    if (_is_slider(wdgt)) {
        bool updated = false;
        switch(id) {
            case SLIDER_PICK:
                updated = wdgt->sub.slider.res[id].h != MAX(height, 2);
                wdgt->sub.slider.res[id].h = MAX(height, 2);
                break;
            case SLIDER_BAR_END:
            case SLIDER_BAR_START:
                updated = wdgt->sub.slider.res[id].h != height;
                wdgt->sub.slider.res[id].h = height;
                break;
            case SLIDER_BAR:
                error_printf("widget_slider_image_height: ignoring height setting for vslider bar resource id %d\n", id);
                break;
            case SLIDER_RESOURCE_COUNT:
            default:
                error_printf("V widget_slider_image_height: unknown resource id %d\n", id);
                break;
        }
        if (updated) {
            slider_reconfigure(wdgt);
        }
    }
    return wdgt;
}

static widget_t *vslider_image_width(widget_t* wdgt, slider_reosurce_ID_t id, int width) {
    if (_is_slider(wdgt)) {
        bool updated = false;
        switch(id) {
            case SLIDER_PICK:
            case SLIDER_BAR_END:
            case SLIDER_BAR_START:
            case SLIDER_BAR:
                updated = wdgt->sub.slider.res[id].w != width;
                wdgt->sub.slider.res[id].w = width;
                break;
            case SLIDER_RESOURCE_COUNT:
            default:
                error_printf("V widget_slider_image_width: unknown resource id %d\n", id);
                break;
        }
        if (updated) {
            slider_reconfigure(wdgt);
        }
    }
    return wdgt;
}

widget_t *widget_slider_image_height(widget_t* wdgt, slider_reosurce_ID_t id, int height) {
    if (_is_slider(wdgt)) {
        switch(wdgt->type) {
            case WIDGET_SLIDER:
                hslider_image_height(wdgt, id, height);
                break;
            case WIDGET_VSLIDER:
                vslider_image_height(wdgt, id, height);
                break;
            default:
                break;
        }
    }
    return wdgt;
}

widget_t *widget_slider_image_width(widget_t* wdgt, slider_reosurce_ID_t id, int width) {
    if (_is_slider(wdgt)) {
        switch(wdgt->type) {
            case WIDGET_SLIDER:
                hslider_image_width(wdgt, id, width);
                break;
            case WIDGET_VSLIDER:
                vslider_image_width(wdgt, id, width);
                break;
            default:
                break;
        }
    }
    return wdgt;
}


// workspace intialisation spin lock 
static SDL_threadID slider_wk_lock = 0;
// !!! DO NOT invoke from render thread !!!
_slider_workspace_t* slider_widget_configure(widget_t* wdgt) {
    // FIXME assert widget is a slider
    if (!_is_slider(wdgt)) {
        error_printf("slider_widget_configure: widget %p is not a slider\n", wdgt);
        return NULL;  //// FIXME !!!!!!!!!
    }
    if (!wdgt->configured) {
        return &wdgt->sub.slider.wk;
    }

    // thread safety: acquire the spin lock 
    // using a single lock for all sliders id deemed adequate
    // using this instead of mutex to circumvent the requirement
    // for static initialisation.
    // alternative would be to create and use a mutex for each slider.
    SDL_threadID lock_expected = 0;
    SDL_threadID lock_desired = SDL_GetThreadID(NULL);
    while(! __atomic_compare_exchange (&slider_wk_lock, &lock_expected, &lock_desired, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
        sleep_milli_seconds(10);
    }

    switch(wdgt->type) {
        case WIDGET_SLIDER:
            hslider_widget_configure(wdgt);
            break;
        case WIDGET_VSLIDER:
            vslider_widget_configure(wdgt);
            break;
        default:
            break;
    }

    // release the spin lock
    lock_expected = SDL_GetThreadID(NULL);
    lock_desired = 0;
    if (!__atomic_compare_exchange (&slider_wk_lock, &lock_expected, &lock_desired, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED)) {
        error_printf("slider_widget_configure: logic failure on spinlock\n");
        exit(EXIT_FAILURE);
    }

    return &wdgt->sub.slider.wk;
}    

bool widget_is_slider(widget_t* wdgt) {
    return _is_slider(wdgt);
}
