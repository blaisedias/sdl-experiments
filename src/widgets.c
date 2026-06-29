#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include "application.h"
#include "widgets.h"
#include "actions.h"
#include "util.h"
#include "logging.h"
#include "timing.h"

extern widget *vumeter_widget_destroy(widget *wdgt);
extern void vumeter_widget_load_media(widget *wdgt, const char* resource_path);

bool debug_rects = false;
bool show_rects = false;
bool show_input_rects = false;

static SDL_RendererFlip flip = SDL_FLIP_NONE;

static char* widget_type_strings[] = {
    "None",
    "Image",
    "Button",
    "MultistateButton",
    "VUMeter",
    "Slider",
    "none"
};

static unsigned text_widget_id = 1;
static void text_render_surface(widget* wdgt);

static inline void free_ex(void** tgt) {
    if (*tgt) {
        free(*tgt);
    }
    *tgt = NULL;
}

#define FREE(x) free_ex((void **)(&x))

bool widget_highlighted(widget* wdgt) {
    return  __atomic_load_n(&wdgt->atomic_highlight, __ATOMIC_ACQUIRE);
}

void widget_set_highlight(widget* wdgt, bool onoff) {
     __atomic_store_n(&wdgt->atomic_highlight, onoff, __ATOMIC_RELEASE);
}

bool widget_pressed(widget* wdgt) {
    return  __atomic_load_n(&wdgt->atomic_pressed, __ATOMIC_ACQUIRE);
}

void widget_set_pressed(widget* wdgt, bool onoff) {
     __atomic_store_n(&wdgt->atomic_pressed, onoff, __ATOMIC_RELEASE);
     if (onoff) {
         wdgt->pressed_millis_start = get_milli_seconds();
     } else {
         wdgt->pressed_millis_end = get_milli_seconds();
     }
     wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hidden;
}

int widget_get_pressed_millis(widget* wdgt) {
    return (int)(wdgt->pressed_millis_end - wdgt->pressed_millis_start);
}

const char* widget_type_name(widget_type typ) {
    if (typ >= WIDGET_NONE && typ <= WIDGET_END) {
        return widget_type_strings[typ];
    }
    return "";
}

void render_none(widget* btn) {
}

static void _debug_draw_rect(widget* wdgt) {
    if (wdgt) {
        SDL_Rect draw_rect;
        copyRect(&wdgt->rect, &draw_rect);
        translate_draw_rect(&draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 255, 0, 0, 128);
        SDL_RenderDrawRect(wdgt->view->app->renderer, &draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 0, 0, 0, 0);
    }
}

static void _show_draw_rect(widget* wdgt) {
    if (wdgt) {
        SDL_Rect draw_rect;
        copyRect(&wdgt->rect, &draw_rect);
        translate_draw_rect(&draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 128, 128, 64, 128);
        SDL_RenderDrawRect(wdgt->view->app->renderer, &draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 0, 0, 0, 0);
    }
}

static void _show_input_rect(widget* wdgt) {
    if (wdgt) {
        SDL_Rect input_rect;
        copyRect(&wdgt->input_rect, &input_rect);
        translate_draw_rect(&input_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 128, 128, 0, 128);
        SDL_RenderDrawRect(wdgt->view->app->renderer, &input_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 0, 0, 0, 0);
    }
}

void widget_render_foreground_default(widget* wdgt) {
    if (debug_rects) {
       _debug_draw_rect(wdgt);
    }
    if (widget_highlighted(wdgt) && wdgt->hotspot == false && show_rects) {
        _show_draw_rect(wdgt);
    }
    if (widget_highlighted(wdgt) && show_input_rects) {
        _show_input_rect(wdgt);
    }
}


static void button_widget_render(widget* wdgt) {
    wdgt->redraw_required = false;
    if (widget_pressed(wdgt) && !wdgt->hotspot) {
        SDL_Rect draw_rect;
        copyRect(&wdgt->rect, &draw_rect);
        translate_draw_rect(&draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 128, 128, 128, 128);
        SDL_RenderFillRect(wdgt->view->app->renderer, &draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 0, 0, 0, 0);
    }
    if (wdgt->hotspot == false || widget_highlighted(wdgt))  {
        SDL_Rect image_rect;
        copyRect(&wdgt->rect, &image_rect);
        translate_image_rect(&image_rect);
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(wdgt->sub.button.texture_id, wdgt->view->app->renderer),
                NULL, &image_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }
}

static void setup_image_fit_src_rect(widget *wdgt) {
    if (wdgt->type == WIDGET_IMAGE) {
        switch(wdgt->sub.image.scale_op)
        {
            case IMAGE_STRETCH_FILL:
                break;
            case IMAGE_CENTRED_FILL: {
                float scale_f = MAX((float)(wdgt->rect.w)/wdgt->sub.image.w, (float)(wdgt->rect.h)/wdgt->sub.image.h);
                wdgt->sub.image.src_rect.w = wdgt->rect.w/scale_f;
                wdgt->sub.image.src_rect.h = wdgt->rect.h/scale_f;
                wdgt->sub.image.src_rect.x = (wdgt->sub.image.w -  wdgt->sub.image.src_rect.w)/2;
                wdgt->sub.image.src_rect.y = (wdgt->sub.image.h -  wdgt->sub.image.src_rect.h)/2;
                debug_printf("image widget: centered_fill src: {w=%d, h=%d} %f, scalef=%f {%d,%d,%d,%d}\n", 
                        wdgt->sub.image.w, wdgt->sub.image.h, 
                        (float)wdgt->sub.image.w/wdgt->sub.image.h,
                        scale_f,
                        wdgt->sub.image.src_rect.x, wdgt->sub.image.src_rect.y, wdgt->sub.image.src_rect.w, wdgt->sub.image.src_rect.h
                        );
                }break;
            case IMAGE_FIT: {
                float scale_f = MIN((float)(wdgt->rect.w)/wdgt->sub.image.w, (float)(wdgt->rect.h)/wdgt->sub.image.h);

                wdgt->sub.image.dst_rect.w = wdgt->sub.image.w*scale_f;
                wdgt->sub.image.dst_rect.h = wdgt->sub.image.h*scale_f;
                wdgt->sub.image.dst_rect.x = (wdgt->rect.w -  wdgt->sub.image.dst_rect.w)/2;
                wdgt->sub.image.dst_rect.y = (wdgt->rect.h -  wdgt->sub.image.dst_rect.h)/2;
                debug_printf("image widget: dst: fit: {w=%d, h=%d} %f, scalef=%f {%d,%d,%d,%d}\n", 
                        wdgt->sub.image.w, wdgt->sub.image.h, 
                        (float)wdgt->sub.image.w/wdgt->sub.image.h,
                        scale_f,
                        wdgt->sub.image.dst_rect.x, wdgt->sub.image.dst_rect.y, wdgt->sub.image.dst_rect.w, wdgt->sub.image.dst_rect.h
                        );
//                if (wdgt->view->app->orientation == 90.0 || wdgt->view->app->orientation == 270.0) {
//                    translate_image_rect(&wdgt->sub.image.dst_rect);
//                }
              }break;
        }
    }
}

widget* widget_load_media(widget* wdgt, const char* resource_path) {
    if (wdgt && wdgt->view->app->renderer) {
        switch(wdgt->type) {
            case WIDGET_NONE:
            case WIDGET_END:
                break;
            case WIDGET_VUMETER:
                vumeter_widget_load_media(wdgt, resource_path);
                break;
            case WIDGET_IMAGE:
                {
                    bool loaded = false;
                    wdgt->sub.image.texture_id = tcache_load_media(wdgt->image_path, wdgt->view->app->renderer, &loaded, NULL);
                    if (loaded) {
                        tcache_lock_texture(wdgt->sub.image.texture_id);
                        if (tcache_quick_get_texture_dimensions(wdgt->sub.image.texture_id, &wdgt->sub.image.w, &wdgt->sub.image.h)) {
                            setup_image_fit_src_rect(wdgt);
                        }
                        wdgt->redraw_required = true;
                    } else {
                        error_printf("widget_load_media: image failed to load %s\n", wdgt->image_path);
                    }
                }break;
            case WIDGET_BUTTON:
                {
                    bool loaded = false;
                    wdgt->sub.button.texture_id = tcache_load_media(wdgt->image_path, wdgt->view->app->renderer, &loaded, NULL);
                    if (loaded) {
                        tcache_lock_texture(wdgt->sub.button.texture_id);
                        wdgt->redraw_required = true;
                    } else {
                        error_printf("widget_load_media: button failed to load %s\n", wdgt->image_path);
                    }
                }
                break;
            case WIDGET_MULTISTATE_BUTTON:
                for(int ims=0; ims < wdgt->sub.multistate_button.state_count; ++ims) {
                    bool loaded = false;
                    _btn_resource* res = wdgt->sub.multistate_button.res + ims;
                    res->texture_id = tcache_load_media(res->resource_path, wdgt->view->app->renderer, &loaded, NULL);
                    if (loaded) {
                        tcache_lock_texture(res->texture_id);
                    } else {
                        error_printf("widget_load_media: multistate button failed to load %s\n", res->resource_path);
                    }
                    wdgt->redraw_required = true;
                }
                break;
            case WIDGET_SLIDER:
                for(int ix=0; ix<SLIDER_RESOURCE_COUNT; ++ix) {
                    for(int ix_img=0; ix_img < sizeof(wdgt->sub.slider.res[ix].image_paths)/sizeof(wdgt->sub.slider.res[ix].image_paths[0]); ++ix_img) {
                        if (wdgt->sub.slider.res[ix].image_paths[ix_img]) {
                            bool loaded = false;
                            wdgt->sub.slider.res[ix].texture_ids[ix_img] = tcache_load_media(
                                    wdgt->sub.slider.res[ix].image_paths[ix_img],
                                    wdgt->view->app->renderer,
                                    &loaded, NULL);
                            if (loaded) {
                                tcache_lock_texture(wdgt->sub.slider.res[ix].texture_ids[ix_img]);
                            } else {
                                 error_printf("widget_load_media: slider failed to load %d %s\n", ix, wdgt->sub.slider.res[ix].image_paths[ix_img]);
                            }
                        }
                    }
                }
                wdgt->redraw_required = true;
                break;
            case WIDGET_TEXT:
                {
                    _text_data_ptr txt_w = &wdgt->sub.text;
                    txt_w->texture_id = tcache_create_entry(txt_w->name);
                    if (txt_w->texture_id) {
                        tcache_lock_texture(txt_w->texture_id);
                        text_render_surface(wdgt);
                    } else {
                        error_printf("widget_load_media: text failed to create texture_id %s\n", txt_w->name);
                    }
                    wdgt->redraw_required = true;
                }
                break;
        }
    }
    return wdgt;
}

widget* widget_rect(widget *wdgt, const SDL_Rect *rect) {
    if (wdgt) {
        copyRect(rect, &wdgt->rect);
        copyRect(rect, &wdgt->input_rect);
/*        
        if (wdgt->type == WIDGET_SLIDER) {
            wdgt->input_rect.y = wdgt->rect.y + wdgt->rect.h/3;
            wdgt->input_rect.h = wdgt->rect.h/3;
        }
*/
//        copyRect(&wdgt->rect, &wdgt->image_rect);
//        translate_image_rect(&wdgt->image_rect);

//        copyRect(&wdgt->rect, &wdgt->draw_rect);
//        translate_draw_rect(&wdgt->draw_rect);
    }
    return wdgt;
}

widget* widget_bounds(widget *wdgt, int x, int y, int w, int h) {
    SDL_Rect rect = {.x=x, .y=y, .w=w, .h=h};
    return widget_rect(wdgt, &rect);
}

/*
widget* widget_next(widget *wdgt, widget* next) {
    if (wdgt) {
        wdgt->next = next;
    }
    return wdgt;
}

widget* widget_prev(widget *wdgt, widget* prev) {
    if (wdgt) {
        if (prev) {
            wdgt->next = prev->next;
            prev->next = wdgt;
        }
    }
    return wdgt;
}
*/

widget* widget_set_player_value_key(widget* wdgt, const char* key) {
    if (wdgt) {
        if (wdgt->player_value_key != NULL) {
            FREE(wdgt->player_value_key);
        }
        if (key) {
            wdgt->player_value_key = strdup(key);
        }
    }
    return wdgt;
}

widget* widget_set_player_range_value_key(widget* wdgt, const char* key) {
    if (wdgt) {
        if (wdgt->player_range_value_key != NULL) {
            FREE(wdgt->player_range_value_key);
        }
        if (key) {
            wdgt->player_range_value_key = strdup(key);
        }
    }
    return wdgt;
}

widget* widget_set_runtime_value_key(widget* wdgt, const char* key) {
    if (wdgt) {
        if (wdgt->runtime_value_key != NULL) {
            FREE(wdgt->runtime_value_key);
        }
        if (key) {
            wdgt->runtime_value_key = strdup(key);
        }
    }
    return wdgt;
}


widget* widget_action(widget* wdgt, action_t action) {
    if (wdgt) {
        if (wdgt->type != WIDGET_MULTISTATE_BUTTON) {
            wdgt->action = action;
        } else {
            if (action != ACTION_NONE) {
                error_printf("widget_action: ignoring set action for multistate button %p %d\n", wdgt, action);
            }
        }
    }
    return wdgt;
}

bool widget_has_action(widget* wdgt, action_t action) {
    if (wdgt) {
        if (wdgt->type != WIDGET_MULTISTATE_BUTTON) {
            return wdgt->action == action;
        } else {
            for(int ix_state=0; ix_state<wdgt->sub.multistate_button.state_count; ++ix_state) {
                if (action == wdgt->sub.multistate_button.res[ix_state].dispatch_action) {
                    return true;
                }
            }
        }
    }
    return false;
}


widget* widget_hide(widget* wdgt, bool hide) {
    if (wdgt) {
        wdgt->hidden = hide;
    }
    return wdgt;
}

widget* widget_image_path(widget* wdgt, const char* path) {
    if (wdgt) {
        if (wdgt->image_path != NULL) {
            FREE(wdgt->image_path);
        }
        if (path) {
            wdgt->image_path = strdup(path);
        }
    }
    return wdgt;
}

widget* widget_focus_enable(widget* wdgt, bool f) {
    *((bool *)(&wdgt->focus_disabled)) = !f;
    return wdgt;
}

static widget* widget_create(const view_context *view) {
    widget* wdgt = calloc(sizeof(*wdgt), 1);
    if (wdgt) {
        wdgt->view = view;
        wdgt->action = ACTION_NONE;
        wdgt->render_backdrop = render_none;
        wdgt->render_foreground = render_none;
        if (view->list) {
            wdgt->next = &view->list->tail;
            wdgt->prev = view->list->tail.prev;
            wdgt->prev->next = wdgt->next->prev = wdgt;
        }
        wdgt->redraw_required = true;
    }
    return wdgt;
}

widget* widget_destroy(widget* wdgt) {
    if (wdgt) {
        switch(wdgt->type) {
            case WIDGET_NONE:
            case WIDGET_END:
                break;
            case WIDGET_VUMETER:
                vumeter_widget_destroy(wdgt);
                break;
            case WIDGET_IMAGE:
                tcache_unlock_texture(wdgt->sub.image.texture_id);
                break;
            case WIDGET_BUTTON:
                tcache_unlock_texture(wdgt->sub.button.texture_id);
                break;
            case WIDGET_MULTISTATE_BUTTON:
                {
                    _btn_resource* res =  wdgt->sub.multistate_button.res;
                    for(int ims=0; ims < wdgt->sub.multistate_button.state_count; ++ims) {
                        tcache_unlock_texture(res[ims].texture_id);
                        FREE(res[ims].resource_path);
                    }
                    FREE(res);
                }break;
            case WIDGET_SLIDER:
                for(int ix=0; ix<SLIDER_RESOURCE_COUNT; ++ix) {
                    for(int ix_txtr=0; ix_txtr < sizeof(wdgt->sub.slider.res[ix].image_paths)/sizeof(wdgt->sub.slider.res[ix].image_paths[0]); ++ix_txtr) {
                        tcache_unlock_texture(wdgt->sub.slider.res[ix].texture_ids[ix_txtr]);
                        wdgt->sub.slider.res[ix].texture_ids[ix_txtr] = 0;
                    }
                    for(int ix_img=0; ix_img < sizeof(wdgt->sub.slider.res[ix].image_paths)/sizeof(wdgt->sub.slider.res[ix].image_paths[0]); ++ix_img) {
                        if ( wdgt->sub.slider.res[ix].image_paths[ix_img] ) {
                            FREE(wdgt->sub.slider.res[ix].image_paths[ix_img]);
                        }
                    }
                }
                break;
            case WIDGET_TEXT:
                {
                    _text_data_ptr txt_w = &wdgt->sub.text;
                    tcache_unlock_texture(txt_w->texture_id);
                    FREE(txt_w->name);
                    FREE(txt_w->content);
                    FREE(txt_w->format);
                    FREE(txt_w->timedate_format);
                    if (txt_w->font) {
                        TTF_CloseFont(txt_w->font);
                        txt_w->font = NULL;
                    }
                }
                break;
        }
        if (wdgt->player_value_key) {
            FREE(wdgt->player_value_key);
        }
        if (wdgt->runtime_value_key) {
            FREE(wdgt->runtime_value_key);
        }
        if (wdgt->player_range_value_key) {
            FREE(wdgt->player_range_value_key);
        }
        if (wdgt->image_path != NULL) {
            FREE(wdgt->image_path);
        }
        FREE(wdgt);
    }
    return wdgt;
}

widget* widget_create_button(const view_context* view) {
    widget* wdgt = widget_create(view);
    if (wdgt) {
        *((widget_type*)&wdgt->type) = WIDGET_BUTTON;
        wdgt->action = ACTION_NONE;
        wdgt->render_backdrop = button_widget_render;
    }
    return wdgt;
}

widget* widget_set_renderhf(widget* wdgt) {
    if (wdgt && ! wdgt->render_as_foreground) {
        wdgt->render_as_foreground = true;
        if (render_none == wdgt->render_foreground && render_none != wdgt->render_backdrop) {
            wdgt->render_foreground = wdgt->render_backdrop;
            wdgt->render_backdrop = render_none;
        }
    }
    return wdgt;
}

widget* widget_unset_renderhf(widget* wdgt) {
    if (wdgt && wdgt->render_as_foreground) {
        wdgt->render_as_foreground = false;
        if (render_none != wdgt->render_foreground && wdgt->render_backdrop == render_none) {
            wdgt->render_backdrop = wdgt->render_foreground;
            wdgt->render_foreground = render_none;
        }
    }
    return wdgt;
}


static void image_widget_render(widget* wdgt) {
    wdgt->redraw_required = false;
    SDL_Rect image_rect;
    copyRect(&wdgt->rect, &image_rect);
    translate_image_rect(&image_rect);

    switch(wdgt->sub.image.scale_op) {
        case IMAGE_STRETCH_FILL:
            SDL_RenderCopyEx(wdgt->view->app->renderer,
                   tcache_quick_get_texture(wdgt->sub.image.texture_id, wdgt->view->app->renderer),
                   NULL, &image_rect,
//                   wdgt->view->app->orientation,
                   0.0,
                   NULL, flip);
            break;
        case IMAGE_FIT:
            SDL_RenderCopyEx(wdgt->view->app->renderer,
                   tcache_quick_get_texture(wdgt->sub.image.texture_id, wdgt->view->app->renderer),
                   NULL, &wdgt->sub.image.dst_rect,
//                   wdgt->view->app->orientation,
                   0.0,
                   NULL, flip);
            break;
        case IMAGE_CENTRED_FILL:
            SDL_RenderCopyEx(wdgt->view->app->renderer,
                    tcache_quick_get_texture(wdgt->sub.image.texture_id, wdgt->view->app->renderer),
                    &wdgt->sub.image.src_rect, &image_rect,
//                    wdgt->view->app->orientation,
                    0.0,
                    NULL, flip);
            break;
    }
}

widget* widget_create_image(const view_context* view) {
    widget* wdgt = widget_create(view);
    if (wdgt) {
        *((widget_type*)&wdgt->type) = WIDGET_IMAGE;
        wdgt->action = ACTION_NONE;
        wdgt->render_backdrop = image_widget_render;
    }
    return wdgt;
}

widget* widget_image_scaling(widget* wdgt, image_scaling op) {
    wdgt->sub.image.scale_op = op;
    setup_image_fit_src_rect(wdgt);
    return wdgt;
}

widget* widget_hotspot_edge(widget* wdgt, hotspot_edge edge, SDL_Rect *r) {
    if (r == NULL) {
        switch(edge) {
            case EDGE_NONE:
                break;
            case EDGE_LEFT:
            case EDGE_RIGHT:
                wdgt->input_rect.y = 0;
                wdgt->input_rect.h = 10000;
                break;
            case EDGE_TOP:
            case EDGE_BOTTOM:
                wdgt->input_rect.x = 0;
                wdgt->input_rect.w = 10000;
                break;
        }
    } else {
        SDL_Rect rect;
        copyRect(r, &rect);
        switch(edge) {
            case EDGE_NONE:
                break;
            case EDGE_LEFT:
                wdgt->input_rect.y = rect.y;
                wdgt->input_rect.h = rect.h;
                break;
            case EDGE_RIGHT:
                wdgt->input_rect.y = rect.y;
                wdgt->input_rect.h = rect.h;
                break;
            case EDGE_TOP:
                wdgt->input_rect.x = rect.x;
                wdgt->input_rect.w = rect.w;
                break;
            case EDGE_BOTTOM:
                wdgt->input_rect.x = rect.x;
                wdgt->input_rect.w = rect.w;
                break;
        }
    }
    return wdgt;
}

static void multistate_button_widget_render(widget* wdgt) {
    wdgt->redraw_required = false;
    if (widget_pressed(wdgt) && !wdgt->hotspot) {
    SDL_Rect draw_rect;
        copyRect(&wdgt->rect, &draw_rect);
        translate_draw_rect(&draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 128, 128, 128, 128);
        SDL_RenderFillRect(wdgt->view->app->renderer, &draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 0, 0, 0, 0);
    }
    if (wdgt->hotspot == false || widget_highlighted(wdgt))  {
        SDL_Rect image_rect;
        copyRect(&wdgt->rect, &image_rect);
        translate_image_rect(&image_rect);
        SDL_RenderCopyEx(wdgt->view->app->renderer,
            tcache_quick_get_texture(wdgt->sub.multistate_button.res[wdgt->sub.multistate_button.state].texture_id, wdgt->view->app->renderer),
            NULL, &image_rect,
//            wdgt->view->app->orientation,
            0.0,
            NULL, flip);
    }
}

widget* widget_create_multistate_button(const view_context* view, int state_count){
    widget* wdgt = widget_create(view);
    if (wdgt) {
        _btn_resource* res = calloc(state_count, sizeof(_btn_resource));
        if (res == NULL) {
            widget_destroy(wdgt);
            return NULL;
        }
        *((widget_type*)&wdgt->type) = WIDGET_MULTISTATE_BUTTON ;
        wdgt->sub.multistate_button.state_count = state_count;
        wdgt->sub.multistate_button.res = res;
        wdgt->action = ACTION_MULTISTATE_BUTTON;
        wdgt->render_backdrop = multistate_button_widget_render;
    }
    return wdgt;
}

widget* widget_multistate_button_addstate(widget* wdgt, unsigned statenum, const char* image_name, action_t dispatch_action, action_t sync_on_action) {
    if (wdgt->type == WIDGET_MULTISTATE_BUTTON && statenum <  wdgt->sub.multistate_button.state_count) {
        _btn_resource* res = wdgt->sub.multistate_button.res + statenum;
        // cleanup
        if (res->resource_path) {
            // already set up
            if (0 == strcmp(res->resource_path, image_name)) {
                return wdgt;
            }
            FREE(res->resource_path);
        }
        if (res->texture_id) {
            tcache_unlock_texture(res->texture_id);
            res->texture_id = 0;
        }
        if (image_name) {
            res->resource_path = strdup(image_name);
        }
        res->dispatch_action = dispatch_action;
        res->sync_on_action = sync_on_action;
    }
    return wdgt;
}

widget* widget_multistate_button_set_state(widget* wdgt, unsigned statenum) {
    if (wdgt->type == WIDGET_MULTISTATE_BUTTON && statenum < wdgt->sub.multistate_button.state_count) {
        wdgt->sub.multistate_button.state = statenum;
        wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hotspot;
    }
    return wdgt;
}

widget* widget_multistate_button_get_state(widget* wdgt, unsigned* statenum) {
    if (wdgt->type == WIDGET_MULTISTATE_BUTTON && statenum) {
        *statenum = wdgt->sub.multistate_button.state;
    }
    return wdgt;
}

widget* widget_multistate_button_sync_on_action(widget* wdgt, action_t act) {
    if (wdgt->type == WIDGET_MULTISTATE_BUTTON) {
        for(int ix_state=0; ix_state<wdgt->sub.multistate_button.state_count; ++ix_state) {
            if (act == wdgt->sub.multistate_button.res[ix_state].sync_on_action) {
                widget_multistate_button_set_state(wdgt, ix_state);
            }
        }
    }
    return wdgt;
}


widget* widget_hotspot(widget* wdgt, bool hotspot) {
    wdgt->hotspot = hotspot;
    return wdgt;
}

static inline bool slider_is_interactive(widget* wdgt) {
    return wdgt->sub.slider.defined_interactive && __atomic_load_n(&wdgt->sub.slider.interactive, __ATOMIC_ACQUIRE);
}

static inline bool slider_wk_is_initialised(widget* wdgt) {
    return __atomic_load_n(&wdgt->sub.slider.wk.initialised, __ATOMIC_ACQUIRE);
}

static inline void slider_set_wk_initialised(widget* wdgt, bool yn) {
    __atomic_store_n(&wdgt->sub.slider.wk.initialised, yn, __ATOMIC_RELEASE);
}

// workspace intialisation spin lock 
static SDL_threadID slider_wk_lock = 0;
// !!! DO NOT invoke from render thread !!!
static _slider_workspace* slider_widget_configure(widget* wdgt) {
#define ZAP_RECT(r) (r).x = (r).y = (r).w = (r).h = 0
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
    
    if (!slider_wk_is_initialised(wdgt)) {
        _slider_resource* pick = wdgt->sub.slider.res+SLIDER_PICK;
        _slider_workspace* wk = &wdgt->sub.slider.wk;

        ZAP_RECT(wk->bar_start_rect);
        ZAP_RECT(wk->bar_end_rect);
        copyRect(&wdgt->rect, &wk->bar_rect);
        copyRect(&wdgt->rect, &wk->pick_rect);

        wk->value_range_delta = wdgt->sub.slider.range.end - wdgt->sub.slider.range.start;

        {
            _slider_resource* bar_start = wdgt->sub.slider.res+SLIDER_BAR_START;
            wk->bar_start_rect.w = bar_start->w;
            wk->bar_start_rect.h = wdgt->rect.h;
            wk->bar_start_rect.x = wdgt->rect.x;
            wk->bar_start_rect.y = wdgt->rect.y;
            wk->bar_rect.x += bar_start->w;
            wk->bar_rect.w -= bar_start->w;
            translate_image_rect(&wk->bar_start_rect);
        }
        {
            _slider_resource* bar_end =  wdgt->sub.slider.res+SLIDER_BAR_END;
            wk->bar_end_rect.w = bar_end->w;
            wk->bar_end_rect.h = wdgt->rect.h;
            wk->bar_end_rect.x = wdgt->rect.x + wdgt->rect.w - bar_end->w -1;
            wk->bar_end_rect.y = wdgt->rect.y;
            wk->bar_rect.w -= bar_end->w;
            translate_image_rect(&wk->bar_end_rect);
        }

        if (slider_is_interactive(wdgt)) {
            wk->pick_rect.w = pick->w;
        } else {
            wk->pick_rect.w = 0;
        }
        wk->half_pw = pick->w/2;
        wk->current_pos = wk->min_pos = wdgt->rect.x + wk->half_pw;
        wk->max_pos = wdgt->rect.x + wdgt->rect.w - wk->half_pw;

        // restore input rectangle y extents.
        wdgt->input_rect.y = wdgt->rect.y;
        wdgt->input_rect.h = wdgt->rect.h;
        // then set input rectangle y extents to match the pick height spec,
        // typically this narrows (vertically) the area of the widget sensitivity
        if (pick->h > 0) {
            wdgt->input_rect.y = wdgt->rect.y + (wdgt->rect.h-pick->h)/2;
            wdgt->input_rect.h = pick->h;
        }
#if 0        
        {
            SDL_Rect *r = &wdgt->rect;
            printf("#### SLIDER %p\n", wdgt);        
            printf("     widget - %02d %02d %02d %02d\n", r->x, r->y, r->w, r->h);
            r = &wk->bar_start_rect;
            printf("     bs     - %02d %02d %02d %02d\n", r->x, r->y, r->w, r->h);
            r = &wk->bar_end_rect;
            printf("     be     - %02d %02d %02d %02d\n", r->x, r->y, r->w, r->h);
            r = &wk->bar_rect;
            printf("     b      - %02d %02d %02d %02d\n", r->x, r->y, r->w, r->h);
            r = &wk->pick_rect;
            printf("     pick   - %02d %02d %02d %02d\n", r->x, r->y, r->w, r->h);
        }
#endif
        slider_set_wk_initialised(wdgt, true);
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

static inline _slider_workspace*  slider_reconfigure(widget* wdgt) {
    slider_set_wk_initialised(wdgt, false);
    return slider_widget_configure(wdgt);
}


static void slider_widget_render(widget* wdgt) {
    wdgt->redraw_required = false;
    if (!slider_wk_is_initialised(wdgt)) { 
        return;
    }
    const _slider_workspace* wk = &wdgt->sub.slider.wk;
    if (wk->value_range_delta < 1) {
        // 0 or negative range => nothing to render
        return;
    }
    _slider_resource* pick = wdgt->sub.slider.res+SLIDER_PICK;

    SDL_Rect pick_rect;
    copyRect(&wk->pick_rect, &pick_rect);

    pick_rect.x = wk->current_pos - wk->half_pw;
    if (slider_is_interactive(wdgt) && widget_pressed(wdgt)) {
        pick_rect.x = wk->drag_pos - wk->half_pw;
    }

    {
        int ix_texture = wk->current_pos > wk->min_pos? 1: 0;
        _slider_resource* bar_start = wdgt->sub.slider.res[SLIDER_BAR_START].texture_ids[ix_texture]? wdgt->sub.slider.res+SLIDER_BAR_START:NULL;
        if (bar_start) {
            SDL_RenderCopyEx(wdgt->view->app->renderer,
                   tcache_quick_get_texture(bar_start->texture_ids[ix_texture], wdgt->view->app->renderer),
                   NULL, &wk->bar_start_rect,
//                   wdgt->view->app->orientation,
                   0.0,
                   NULL, flip);
        }
    }

    {
        int ix_texture = wk->current_pos < wk->max_pos? 1: 0;
        _slider_resource* bar_end = wdgt->sub.slider.res[SLIDER_BAR_END].texture_ids[ix_texture]? wdgt->sub.slider.res+SLIDER_BAR_END:NULL;
        if (bar_end) {
            SDL_RenderCopyEx(wdgt->view->app->renderer,
                   tcache_quick_get_texture(bar_end->texture_ids[ix_texture], wdgt->view->app->renderer),
                   NULL, &wk->bar_end_rect,
//                   wdgt->view->app->orientation,
                   0.0,
                   NULL, flip);
        }
    }

    _slider_resource* bar = wdgt->sub.slider.res[SLIDER_BAR].texture_ids[0]? wdgt->sub.slider.res+SLIDER_BAR:NULL;
    if (bar) {
        SDL_Rect image_rect;
        copyRect(&wk->bar_rect, &image_rect);
        image_rect.w = pick_rect.x - image_rect.x;
        translate_image_rect(&image_rect);
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(bar->texture_ids[0], wdgt->view->app->renderer),
                NULL, &image_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }

    if (pick_rect.w && pick_rect.h && slider_is_interactive(wdgt)) {
        SDL_Rect image_rect;
        copyRect(&pick_rect, &image_rect);
        translate_image_rect(&image_rect);
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(pick->texture_ids[0], wdgt->view->app->renderer),
                NULL, &image_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }

    if (bar) {
        SDL_Rect image_rect;
        copyRect(&wk->bar_rect, &image_rect);
        image_rect.w -= pick_rect.x + pick_rect.w - image_rect.x;
        image_rect.x = pick_rect.x + pick_rect.w;
        translate_image_rect(&image_rect);
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(bar->texture_ids[1], wdgt->view->app->renderer),
                NULL, &image_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }
}

widget *widget_create_slider(const view_context* view) {
    widget* wdgt = widget_create(view);
    if (wdgt) {
        *((widget_type*)&wdgt->type) = WIDGET_SLIDER;
        wdgt->action = ACTION_NONE;
        wdgt->render_backdrop = slider_widget_render;
        __atomic_store_n(&wdgt->sub.slider.interactive, true, __ATOMIC_RELEASE);
        wdgt->sub.slider.defined_interactive = true;
    }
    return wdgt;
}

widget *widget_slider_image_paths(widget* wdgt, slider_resource_ID id, const char* path1, const char* path2) {
    if (wdgt) {
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


widget *widget_slider_image_width(widget* wdgt, slider_resource_ID id, int width) {
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

widget *widget_slider_image_height(widget* wdgt, slider_resource_ID id, int height) {
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

static widget *widget_slider_track(widget* wdgt, const SDL_Point *pt) {
    if (slider_is_interactive(wdgt)) {
        if (widget_pressed(wdgt) && (wdgt->sub.slider.range.end - wdgt->sub.slider.range.start) > 0) {
            _slider_resource* pick = wdgt->sub.slider.res+SLIDER_PICK;
            _slider_workspace* wk = &wdgt->sub.slider.wk;
            if (pick) {
                if (pt->x < wk->min_pos) {
                    wk->drag_pos = wk->min_pos;
                } else if (pt->x > wk->max_pos) {
                    wk->drag_pos = wk->max_pos;
                } else {
                    wk->drag_pos = pt->x;
                }
            }
            wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hotspot;
        }
    }
    return wdgt;
}

static widget *widget_slider_tracking_commit(widget* wdgt, const SDL_Point *pt) {
    if (slider_is_interactive(wdgt)) {
        widget_slider_track(wdgt, pt);
        _slider_workspace* wk = &wdgt->sub.slider.wk;
        wk->current_pos = wk->drag_pos;
    }
    return wdgt;
}

/*
widget *widget_slider_set_value(widget* wdgt, int value) {
    if (wdgt && wdgt->type == WIDGET_SLIDER) {
        if (value >= wdgt->sub.slider.range.start && value <= wdgt->sub.slider.range.end) {
            _slider_workspace* wk = slider_reconfigure(wdgt);
            if (slider_wk_is_initialised(wdgt)) {
                if (wk->value_range_delta) {
                    // range must be non-zero to calculate the position of the pick
                    float offset = ((float)(value - wdgt->sub.slider.range.start)*(wk->max_pos - wk->min_pos))/wk->value_range_delta;
                    wk->current_pos = wk->min_pos + (int)offset;
                    dummy_printf("widget_slider_set_value (%d * %d)/%d = %d, for %d\n", 
                            value - wdgt->sub.slider.range.start,
                            (wk->max_pos - wk->min_pos),
                            wk->value_range_delta,
                            wk->current_pos,
                            value);
                    wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hotspot;
                }
            } else {
                error_printf("widget_slider_set_value: workspace is uninitialised\n");
            }
        } else {
            error_printf("widget_slider_set_value: %d not in range %d-%d\n",
                    value,
                    wdgt->sub.slider.range.start,
                    wdgt->sub.slider.range.end);
        }
    }
    return wdgt;
}
*/

widget *widget_slider_update_value(widget* wdgt, int value) {
    if (wdgt && wdgt->type == WIDGET_SLIDER) {
        if (value >= wdgt->sub.slider.range.start
                && value <= wdgt->sub.slider.range.end) {
            if (!slider_wk_is_initialised(wdgt)) {
                 slider_reconfigure(wdgt);
            }
            if (slider_wk_is_initialised(wdgt)) {
                _slider_workspace* wk = &wdgt->sub.slider.wk;
                if (wk->value_range_delta) {
                    // range must be non-zero to calculate the position of the pick
                    float offset = ((float)(value - wdgt->sub.slider.range.start)*(wk->max_pos - wk->min_pos))/wk->value_range_delta;
                    bool updated = wk->current_pos != wk->min_pos + (int)offset;
                    wk->current_pos = wk->min_pos + (int)offset;
                    dummy_printf("widget_slider_update_value (%d * %d)/%d = %d, for %d\n", 
                            value - wdgt->sub.slider.range.start,
                            (wk->max_pos - wk->min_pos),
                            wk->value_range_delta,
                            wk->current_pos,
                            value);
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

widget *widget_slider_range(widget* wdgt, int start, int end) {
    if (wdgt && wdgt->type == WIDGET_SLIDER) {
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

widget *widget_slider_set_interactive(widget* wdgt, bool yn) {
    if (wdgt && wdgt->type == WIDGET_SLIDER) {
        bool ny = ! yn;
        bool updated =  !wdgt->render_as_foreground && !wdgt->hotspot && __atomic_compare_exchange_n(&wdgt->sub.slider.interactive, &ny, yn, false, __ATOMIC_ACQ_REL, __ATOMIC_RELAXED);
        if (updated) {
            slider_reconfigure(wdgt);
        }
        wdgt->redraw_required = updated;
    }
    return wdgt;
}

widget *widget_slider_define_interactive(widget* wdgt, bool yn) {
    if (wdgt && wdgt->type == WIDGET_SLIDER) {
        wdgt->sub.slider.defined_interactive = yn;
    }
    return wdgt;
}


widget *widget_slider_get_value(widget* wdgt, int* value) {
    if (wdgt && wdgt->type == WIDGET_SLIDER) {
        const _slider_workspace* wk = &wdgt->sub.slider.wk;
        int v = (wk->current_pos - wk->min_pos);
        dummy_printf("widget_slider_get_value v=%d\n", v);
        v *= wk->value_range_delta;
        v /= (wk->max_pos - wk->min_pos);
        *value = wdgt->sub.slider.range.start + v;
    }
    return wdgt;
}

static void text_widget_render(widget* wdgt) {
    wdgt->redraw_required = false;
    _text_data_ptr txt_w = &wdgt->sub.text;
    if (widget_pressed(wdgt)&& !wdgt->hotspot) {
        SDL_Rect draw_rect;
        copyRect(&wdgt->rect, &draw_rect);
        translate_draw_rect(&draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 128, 128, 128, 128);
        SDL_RenderFillRect(wdgt->view->app->renderer, &draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 0, 0, 0, 0);
    }
    if (wdgt->hotspot == false || widget_highlighted(wdgt))  {
        SDL_Rect image_rect;
        copyRect(&txt_w->dst_rect, &image_rect);
        translate_image_rect(&image_rect);
        SDL_RenderCopyEx(wdgt->view->app->renderer,
                tcache_quick_get_texture(txt_w->texture_id, wdgt->view->app->renderer),
                NULL,
                &image_rect,
//                wdgt->view->app->orientation,
                0.0,
                NULL, flip);
    }
}

widget* widget_create_text(const view_context* view) {
    widget* wdgt = widget_create(view);
    if (wdgt) {
        static SDL_Color white = {255, 255, 255, 255};
        _text_data_ptr txt_w = &wdgt->sub.text;
        *((widget_type*)&wdgt->type) = WIDGET_TEXT;
        wdgt->action = ACTION_NONE;
        wdgt->render_backdrop = text_widget_render;
        char buffer[64];
        sprintf(buffer, "\\-text-%x-\\", __atomic_fetch_add(&text_widget_id, 1, __ATOMIC_ACQ_REL));
        txt_w->name = strdup(buffer);
        txt_w->content = strdup("");
        txt_w->colour = white;
    }
    return wdgt;
}

widget* widget_text_set_format(widget* wdgt, const char* format) {
    if (wdgt && wdgt->type == WIDGET_TEXT) {
        _text_data_ptr txt_w = &wdgt->sub.text;
        if (txt_w->format) {
            FREE(txt_w->format);
        }
        if (format) {
            txt_w->format = strdup(format);
        }
    }
    return wdgt;
}

widget* widget_text_set_timedate_format(widget* wdgt, const char* format) {
    if (wdgt && wdgt->type == WIDGET_TEXT) {
        _text_data_ptr txt_w = &wdgt->sub.text;
        if (txt_w->timedate_format) {
            FREE(txt_w->timedate_format);
        }
        if (format) {
            txt_w->timedate_format = strdup(format);
        }
    }
    return wdgt;
}


static void text_render_surface(widget* wdgt) {
    if (wdgt && wdgt->type == WIDGET_TEXT) {
        _text_data_ptr txt_w = &wdgt->sub.text;
//        txt_w->content_dim.x = txt_w->content_dim.y = txt_w->content_dim.w = txt_w->content_dim.h = 0;
        if (txt_w->texture_id) {
            SDL_Surface *surface = TTF_RenderUTF8_Blended(txt_w->font, txt_w->content, txt_w->colour);
            if (surface) {
                if (surface->w > wdgt->view->app->max_texture_width) {
                    // FIXME: 
                    // width exceeds the texture width supported by the renderer
                    // for now scale down the surface to match that limit
                    float scalef = (float)wdgt->view->app->max_texture_width/surface->w;
                    SDL_Surface *scaled_surface = SDL_CreateRGBSurfaceWithFormat(0,
                            surface->w*scalef, surface->h*scalef,
                            SDL_BITSPERPIXEL(wdgt->view->app->pixelFormat), wdgt->view->app->pixelFormat);
                    if (scaled_surface) {
                        SDL_BlitScaled(surface, NULL, scaled_surface, NULL);
                        SDL_free(surface);
                        error_printf("text_render_surface: renderer limit: is %d scaling down by %f from %dx%d to %dx%d\n%s\n",
                                wdgt->view->app->max_texture_width,
                                scalef,
                                surface->w, surface->h,
                                scaled_surface->w, scaled_surface->h,
                                txt_w->content
                                );
                        surface = scaled_surface;
                    }
                }
                tcache_set_surface(txt_w->texture_id, surface);
//                txt_w->content_dim.w = surface->w;
//                txt_w->content_dim.h = surface->h;

                txt_w->dst_rect.w = surface->w;
                txt_w->dst_rect.h = surface->h;
                switch(txt_w->justification) {
                    case TXT_LEFT:
                        txt_w->dst_rect.x = wdgt->rect.x;
                        txt_w->dst_rect.y = wdgt->rect.y + ((wdgt->rect.h - surface->h)/2);
                        break;
                    case TXT_RIGHT:
                        txt_w->dst_rect.x = wdgt->rect.x + ((wdgt->rect.w - surface->w));
                        txt_w->dst_rect.y = wdgt->rect.y + ((wdgt->rect.h - surface->h)/2);
                        break;
                    case TXT_CENTRED:
                    default:
                        txt_w->dst_rect.x = wdgt->rect.x + ((wdgt->rect.w - surface->w)/2);
                        txt_w->dst_rect.y = wdgt->rect.y + ((wdgt->rect.h - surface->h)/2);
                        break;
                }
                
                // for now scale text to fit content.
                float scale_x = (float)surface->w/wdgt->rect.w;
                float scale_y = (float)surface->h/wdgt->rect.h;
                if (scale_x > 1 || scale_y > 1) {
                    float scale = scale_x > scale_y ? scale_x : scale_y;
                    int scaled_w = surface->w / scale;
                    int scaled_h = surface->h / scale;
                    error_printf("text_render_surface: scaling text to fit %f (x=%f,y=%f) from %dx%d to %dx%d\n%s\n",
                            1/scale, 1/scale_x, 1/scale_y,
                            surface->w, surface->h,
                            scaled_w, scaled_h,
                            txt_w->content
                            );
                    txt_w->dst_rect.x = wdgt->rect.x + ((wdgt->rect.w - scaled_w)/2);
                    txt_w->dst_rect.y = wdgt->rect.y + ((wdgt->rect.h - scaled_h)/2);
                    txt_w->dst_rect.w = scaled_w;
                    txt_w->dst_rect.h = scaled_h;
                    
                    switch(txt_w->justification) {
                        case TXT_LEFT:
                            txt_w->dst_rect.x = wdgt->rect.x;
                            txt_w->dst_rect.y = wdgt->rect.y + ((wdgt->rect.h - scaled_h)/2);
                            break;
                        case TXT_RIGHT:
                            txt_w->dst_rect.x = wdgt->rect.x + ((wdgt->rect.w - scaled_w));
                            txt_w->dst_rect.y = wdgt->rect.y + ((wdgt->rect.h - scaled_h)/2);
                            break;
                        case TXT_CENTRED:
                        default:
                            txt_w->dst_rect.x = wdgt->rect.x + ((wdgt->rect.w - scaled_w)/2);
                            txt_w->dst_rect.y = wdgt->rect.y + ((wdgt->rect.h - scaled_h)/2);
                            break;
                    }
                }
            }
        }
        wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hotspot;
    }
}

widget* widget_text_set_content(widget* wdgt, const char* content) {
    if (wdgt && wdgt->type == WIDGET_TEXT) {
        _text_data_ptr txt_w = &wdgt->sub.text;
        if (NULL == content || 0 == strlen(content)) {
            // empty strings => TTF_Render returns a nil surface,
            // force change of texture
            content = " ";
        }
        if (txt_w->content) {
            if (0 == strcmp(content, txt_w->content)) {
                return wdgt;
            }
            FREE(txt_w->content);
//            txt_w->content_dim.w = txt_w->content_dim.h = 0;
        }
        txt_w->content = strdup(content);
        text_render_surface(wdgt);
    }
    return wdgt;
}

widget* widget_text_set_font(widget* wdgt, const char* font_path, int size) {
    if (wdgt && wdgt->type == WIDGET_TEXT) {
        _text_data_ptr txt_w = &wdgt->sub.text;
        txt_w->font = TTF_OpenFont(font_path, size);
        if (!txt_w->font) {
            error_printf("failed to create font %s %d %s\n", font_path, size, TTF_GetError());
        }
        text_render_surface(wdgt);
    }
    return wdgt;
}

widget* widget_text_set_colour(widget* wdgt, SDL_Color colour) {
    if (wdgt && wdgt->type == WIDGET_TEXT) {
        _text_data_ptr txt_w = &wdgt->sub.text;
        txt_w->colour.r = colour.r;
        txt_w->colour.g = colour.g;
        txt_w->colour.b = colour.b;
        txt_w->colour.a = colour.a;
    }
    text_render_surface(wdgt);
    return wdgt;
}

widget* widget_text_set_justification(widget* wdgt, const char* justif_name) {
    if (wdgt && wdgt->type == WIDGET_TEXT) {
        if (justif_name) {
            if (0 == strcmp("center", justif_name)) { wdgt->sub.text.justification = TXT_CENTRED;}
            else if (0 == strcmp("left", justif_name)) { wdgt->sub.text.justification = TXT_LEFT;}
            else if (0 == strcmp("right", justif_name)) { wdgt->sub.text.justification = TXT_RIGHT;}
            else error_printf("unsupported text justification %s\n", justif_name);
            text_render_surface(wdgt);
        }
    }
    return wdgt;
}

// !!! DO NOT call in render thread
widget* widget_configure(widget* wdgt) {
    switch(wdgt->type) {
        case WIDGET_NONE:
        case WIDGET_IMAGE:
        case WIDGET_BUTTON:
        case WIDGET_MULTISTATE_BUTTON:
        case WIDGET_VUMETER:
        case WIDGET_TEXT:
        case WIDGET_END:
            wdgt->configured = true;
            break;
        case WIDGET_SLIDER:
            wdgt->configured = true;
            slider_widget_configure(wdgt);
            break;
    }
    return wdgt;
}

static widget_list* widget_list_initialise(widget_list* list, view_context* view) {
    if (list) {
        list->head.next = &list->tail;
        *((widget_type*)(&list->head.type)) = WIDGET_NONE;
        list->tail.prev = &list->head;
        *((widget_type*)(&list->tail.type)) = WIDGET_END;
        for(widget* w=&list->head; w != NULL; w=w->next) {
            w->view = view;
            w->hidden = true;
            *((bool *)(&w->focus_disabled)) = true;
            w->rect.x = w->rect.y =  -100000;
            w->rect.w = w->rect.h =  0;
            w->input_rect.x = w->input_rect.y =  -100000;
            w->input_rect.w = w->input_rect.h =  0;
            w->render_backdrop = render_none;
        }
    }
    return list;
}

widget_list* create_widget_list(view_context* view) {
    return widget_list_initialise(calloc(sizeof(widget_list), 1), view);
}

widget_list* destroy_widgets_in_list(widget_list* list) {
    if (list) {
        widget* widget = list->head.next;
        while(widget != &list->tail) {
            list->head.next = widget->next;
            widget_destroy(widget);
            widget = list->head.next;
        }
        list->head.next = &list->tail;
        list->tail.prev = &list->head;
        return list;
    }
    return list;
}

widget_list* destroy_widget_list(widget_list* list) {
    if (list) {
        destroy_widgets_in_list(list);
        free(list);
    }
    return NULL;
}

void widget_list_load_media(const widget_list* list, const char* resource_path) {
    for (widget* widget = list->head.next; widget != NULL; widget = widget->next) {
        widget_load_media(widget, resource_path);
    }
}

void widget_list_react(const widget_list* list, const pointer_input input, SDL_Point* pt) {
    bool selected = false;
    input_printf("%d: %04d,%04d -> ", input, pt->x, pt->y);
    translate_point(pt);
    input_printf(" %04d,%04d\n", pt->x, pt->y);
    switch(input) {
        case POINTER_DOWN:
            for(widget* widget=list->tail.prev; widget != NULL; widget=widget->prev) {
                if (widget->hidden) { continue;}
                if (!selected) {
                    widget->focussed = SDL_PointInRect(pt, &widget->input_rect) && (!widget->focus_disabled);
                    widget_set_highlight(widget, widget->focussed);
                    widget_set_pressed(widget, widget->focussed);
                    selected = widget->focussed;
                    if (widget->type == WIDGET_SLIDER) {
                        widget_slider_track(widget, pt);
                    }
                } else {
                    widget->focussed = false;
                    widget_set_highlight(widget, false);
                    widget_set_pressed(widget, false);
                }
            }
            break;
        case POINTER_UP:
            for(widget* widget=list->tail.prev; widget != NULL; widget=widget->prev) {
                if (widget->hidden) { continue;}
                widget_set_pressed(widget, false);
                if (SDL_PointInRect(pt, &widget->input_rect) && widget->focussed) {
                   input_printf("HIT: {%04d,%04d} {%04d,%04d} %p\n",
                                widget->input_rect.x, widget->input_rect.y,
                                widget->input_rect.x + widget->input_rect.w,
                                widget->input_rect.y, widget->input_rect.h,
                                widget
                                );
                    widget->focussed = false;
                    widget_set_highlight(widget, widget->focussed);
                    if (widget->type == WIDGET_SLIDER) {
                        widget_slider_tracking_commit(widget, pt);
                        int value =  -987654321;
                        widget_slider_get_value(widget, &value);
                        dummy_printf("slider value = %d\n", value);
                    }
                    widget_dispatch_action(widget);
                } else {
                    widget->focussed = false;
                    widget_set_highlight(widget, false);
                }
            }
            break;
        case POINTER_MOTION:
            for (widget* widget=list->tail.prev; widget != NULL; widget=widget->prev) {
                if (widget->hidden) { continue;}
                if (!selected) {
                    widget_set_highlight(widget, SDL_PointInRect(pt, &widget->input_rect) && (!widget->focus_disabled));
                    selected = widget_highlighted(widget);
                    if (selected) {
                        if (widget->type == WIDGET_SLIDER) {
                            widget_slider_track(widget, pt);
                        }
                    }
                } else {
                    widget_set_highlight(widget, false);
                }
            }
            break;
    }
}


bool widget_list_query_render_backdrop(const widget_list* wdgt_list) {
    if (wdgt_list) {
        for(widget* widget=wdgt_list->head.next; widget != NULL; widget=widget->next) {
            if (widget->redraw_required) {
                return true;
            }
        }
    }
    return false;
}

void widget_list_render_backdrop(const widget_list* wdgt_list) {
    if (NULL == wdgt_list) {
        return;
    }
    for(widget* widget=wdgt_list->head.next; widget != NULL; widget=widget->next) {
        if (widget->hidden) {
            widget->redraw_required = false;
        } else {
            widget->render_backdrop(widget);
        }
    }

    for(widget* widget=wdgt_list->head.next; widget != NULL; widget=widget->next) {
        if (widget->render_backdrop != NULL && widget->redraw_required) { error_printf("!? %d\n", widget->type); }
    }

}

void widget_list_render_foreground(const widget_list* wdgt_list) {
    if (NULL == wdgt_list) {
        return;
    }
    for(widget* widget=wdgt_list->head.next; widget != NULL; widget=widget->next) {
        if (!widget->hidden) {
            widget->render_foreground(widget);
            widget_render_foreground_default(widget);
        }
    }
}

