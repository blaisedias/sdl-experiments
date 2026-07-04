#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include "application.h"
#include "widgets_internal.h"
#include "actions.h"
#include "util.h"
#include "logging.h"
#include "timing.h"

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
    "Text",
    "VSlider",
    "none"
};

static unsigned text_widget_id = 1;
static void text_render_surface(widget_t* wdgt);

static inline void free_ex(void** tgt) {
    if (*tgt) {
        free(*tgt);
    }
    *tgt = NULL;
}

#define FREE(x) free_ex((void **)(&x))

bool widget_highlighted(widget_t* wdgt) {
    return  __atomic_load_n(&wdgt->atomic_highlight, __ATOMIC_ACQUIRE);
}

void widget_set_highlight(widget_t* wdgt, bool onoff) {
     __atomic_store_n(&wdgt->atomic_highlight, onoff, __ATOMIC_RELEASE);
}

bool widget_pressed(widget_t* wdgt) {
    return  __atomic_load_n(&wdgt->atomic_pressed, __ATOMIC_ACQUIRE);
}

void widget_set_pressed(widget_t* wdgt, bool onoff) {
     __atomic_store_n(&wdgt->atomic_pressed, onoff, __ATOMIC_RELEASE);
     if (onoff) {
         wdgt->pressed_millis_start = get_milli_seconds();
     } else {
         wdgt->pressed_millis_end = get_milli_seconds();
     }
     wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hidden;
}

int widget_get_pressed_millis(widget_t* wdgt) {
    return (int)(wdgt->pressed_millis_end - wdgt->pressed_millis_start);
}

const char* widget_type_name(widget_type_t typ) {
    if (typ >= WIDGET_NONE && typ <= WIDGET_END) {
        return widget_type_strings[typ];
    }
    return "";
}

const char* widget_get_type_name(widget_t* wdgt) {
    if (wdgt) {
        return widget_type_name(wdgt->type);
    }
    return "";
}

widget_type_t widget_get_type(widget_t* wdgt) {
    if (wdgt) {
        return wdgt->type;
    }
    return WIDGET_NONE;
}

void render_none(widget_t* btn) {
}

static void _debug_draw_rect(widget_t* wdgt) {
    if (wdgt) {
        SDL_Rect draw_rect;
        copyRect(&wdgt->rect, &draw_rect);
        translate_draw_rect(&draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 255, 0, 0, 128);
        SDL_RenderDrawRect(wdgt->view->app->renderer, &draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 0, 0, 0, 0);
    }
}

static void _show_draw_rect(widget_t* wdgt) {
    if (wdgt) {
        SDL_Rect draw_rect;
        copyRect(&wdgt->rect, &draw_rect);
        translate_draw_rect(&draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 128, 128, 64, 128);
        SDL_RenderDrawRect(wdgt->view->app->renderer, &draw_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 0, 0, 0, 0);
    }
}

static void _show_input_rect(widget_t* wdgt) {
    if (wdgt) {
        SDL_Rect input_rect;
        copyRect(&wdgt->input_rect, &input_rect);
        translate_draw_rect(&input_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 128, 128, 0, 128);
        SDL_RenderDrawRect(wdgt->view->app->renderer, &input_rect);
        SDL_SetRenderDrawColor(wdgt->view->app->renderer, 0, 0, 0, 0);
    }
}

void widget_render_foreground_default(widget_t* wdgt) {
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


static void button_widget_render(widget_t* wdgt) {
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

static void setup_image_fit_src_rect(widget_t *wdgt) {
    if (wdgt->type == WIDGET_IMAGE) {
        if (0 == wdgt->sub.image.texture_id) {
            return;
        }
        if (!tcache_quick_get_texture_dimensions(wdgt->sub.image.texture_id, &wdgt->sub.image.w, &wdgt->sub.image.h)) {
            error_printf("unable to retrieve texture dimensions from cache for texture_id%d\n",
                            wdgt->sub.image.texture_id);
            return;
        }
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
                wdgt->sub.image.dst_rect.x = wdgt->rect.x + (wdgt->rect.w -  wdgt->sub.image.dst_rect.w)/2;
                wdgt->sub.image.dst_rect.y = wdgt->rect.y + (wdgt->rect.h -  wdgt->sub.image.dst_rect.h)/2;
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

widget_t* widget_load_media(widget_t* wdgt, const char* resource_path) {
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
                    _bnt_resource_t* res = wdgt->sub.multistate_button.res + ims;
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
            case WIDGET_VSLIDER:
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

widget_t* widget_rect(widget_t *wdgt, const SDL_Rect *rect) {
    if (wdgt) {
        copyRect(rect, &wdgt->rect);
        copyRect(rect, &wdgt->input_rect);
//        copyRect(&wdgt->rect, &wdgt->image_rect);
//        translate_image_rect(&wdgt->image_rect);

//        copyRect(&wdgt->rect, &wdgt->draw_rect);
//        translate_draw_rect(&wdgt->draw_rect);
    }
    return wdgt;
}

widget_t* widget_bounds(widget_t *wdgt, int x, int y, int w, int h) {
    SDL_Rect rect = {.x=x, .y=y, .w=w, .h=h};
    return widget_rect(wdgt, &rect);
}


widget_t* widget_set_player_value_key(widget_t* wdgt, const char* key) {
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

const char* widget_get_player_value_key(widget_t* wdgt) {
    if (wdgt) {
        return wdgt->player_value_key ? wdgt->player_value_key : "";
    }
    return "";
}


widget_t* widget_set_player_range_value_key(widget_t* wdgt, const char* key) {
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

const char* widget_get_player_range_value_key(widget_t* wdgt) {
    if (wdgt) {
        return wdgt->player_range_value_key;
    }
    return "";
}

widget_t* widget_set_runtime_value_key(widget_t* wdgt, const char* key) {
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

const char* widget_get_runtime_value_key(widget_t* wdgt) {
    if (wdgt) {
        return wdgt->runtime_value_key ? wdgt->runtime_value_key : "";
    }
    return "";
}

widget_t* widget_action(widget_t* wdgt, action_t action) {
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

bool widget_has_action(widget_t* wdgt, action_t action) {
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

action_t widget_get_action(widget_t* wdgt) {
    return ACTION_NONE;
}

widget_t* widget_hide(widget_t* wdgt, bool hide) {
    if (wdgt) {
        wdgt->hidden = hide;
    }
    return wdgt;
}

bool widget_is_hidden(widget_t* wdgt) {
    if (wdgt) {
        return wdgt->hidden;
    }
    return true;
}


widget_t* widget_image_path(widget_t* wdgt, const char* path) {
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

widget_t* widget_focus_enable(widget_t* wdgt, bool f) {
    if (wdgt) {
        *((bool *)(&wdgt->focus_disabled)) = !f;
    }
    return wdgt;
}

widget_t* widget_set_focussed(widget_t* wdgt, bool f) {
    if (wdgt) {
        wdgt->focussed = f;
    }
    return wdgt;
}

bool widget_get_focussed(widget_t* wdgt) {
    if (wdgt) {
        return wdgt->focussed;
    }
    return false;
}

widget_t* widget_create(const view_context_t *view) {
    widget_t* wdgt = calloc(sizeof(*wdgt), 1);
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

widget_t* widget_destroy(widget_t* wdgt) {
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
                    _bnt_resource_t* res =  wdgt->sub.multistate_button.res;
                    for(int ims=0; ims < wdgt->sub.multistate_button.state_count; ++ims) {
                        tcache_unlock_texture(res[ims].texture_id);
                        FREE(res[ims].resource_path);
                    }
                    FREE(res);
                }break;
            case WIDGET_SLIDER:
            case WIDGET_VSLIDER:
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

widget_t* widget_create_button(const view_context_t* view) {
    widget_t* wdgt = widget_create(view);
    if (wdgt) {
        *((widget_type_t*)&wdgt->type) = WIDGET_BUTTON;
        wdgt->action = ACTION_NONE;
        wdgt->render_backdrop = button_widget_render;
    }
    return wdgt;
}

widget_t* widget_set_renderhf(widget_t* wdgt) {
    if (wdgt && ! wdgt->render_as_foreground) {
        wdgt->render_as_foreground = true;
        if (render_none == wdgt->render_foreground && render_none != wdgt->render_backdrop) {
            wdgt->render_foreground = wdgt->render_backdrop;
            wdgt->render_backdrop = render_none;
        }
    }
    return wdgt;
}

widget_t* widget_unset_renderhf(widget_t* wdgt) {
    if (wdgt && wdgt->render_as_foreground) {
        wdgt->render_as_foreground = false;
        if (render_none != wdgt->render_foreground && wdgt->render_backdrop == render_none) {
            wdgt->render_backdrop = wdgt->render_foreground;
            wdgt->render_foreground = render_none;
        }
    }
    return wdgt;
}


static void image_widget_render(widget_t* wdgt) {
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

widget_t* widget_create_image(const view_context_t* view) {
    widget_t* wdgt = widget_create(view);
    if (wdgt) {
        *((widget_type_t*)&wdgt->type) = WIDGET_IMAGE;
        wdgt->action = ACTION_NONE;
        wdgt->render_backdrop = image_widget_render;
    }
    return wdgt;
}

widget_t* widget_image_scaling(widget_t* wdgt, image_scaling_t op) {
    wdgt->sub.image.scale_op = op;
    if (wdgt->configured) {
        setup_image_fit_src_rect(wdgt);
    }
    return wdgt;
}

widget_t* widget_hotspot_edge(widget_t* wdgt, hotspot_edge_t edge, SDL_Rect *r) {
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

static void multistate_button_widget_render(widget_t* wdgt) {
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

widget_t* widget_create_multistate_button(const view_context_t* view, int state_count){
    widget_t* wdgt = widget_create(view);
    if (wdgt) {
        _bnt_resource_t* res = calloc(state_count, sizeof(_bnt_resource_t));
        if (res == NULL) {
            widget_destroy(wdgt);
            return NULL;
        }
        *((widget_type_t*)&wdgt->type) = WIDGET_MULTISTATE_BUTTON ;
        wdgt->sub.multistate_button.state_count = state_count;
        wdgt->sub.multistate_button.res = res;
        wdgt->action = ACTION_END;
        wdgt->render_backdrop = multistate_button_widget_render;
    }
    return wdgt;
}

widget_t* widget_multistate_button_addstate(widget_t* wdgt, unsigned statenum, const char* image_name, action_t dispatch_action, action_t sync_on_action) {
    if (wdgt->type == WIDGET_MULTISTATE_BUTTON && statenum <  wdgt->sub.multistate_button.state_count) {
        _bnt_resource_t* res = wdgt->sub.multistate_button.res + statenum;
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

widget_t* widget_multistate_button_set_state(widget_t* wdgt, unsigned statenum) {
    if (wdgt->type == WIDGET_MULTISTATE_BUTTON && statenum < wdgt->sub.multistate_button.state_count) {
        wdgt->sub.multistate_button.state = statenum;
        wdgt->redraw_required = !wdgt->render_as_foreground && !wdgt->hotspot;
    }
    return wdgt;
}

widget_t* widget_multistate_button_get_state(widget_t* wdgt, unsigned* statenum) {
    if (wdgt->type == WIDGET_MULTISTATE_BUTTON && statenum) {
        *statenum = wdgt->sub.multistate_button.state;
    }
    return wdgt;
}

widget_t* widget_multistate_button_sync_on_action(widget_t* wdgt, action_t act) {
    if (wdgt->type == WIDGET_MULTISTATE_BUTTON) {
        for(int ix_state=0; ix_state<wdgt->sub.multistate_button.state_count; ++ix_state) {
            if (act == wdgt->sub.multistate_button.res[ix_state].sync_on_action) {
                widget_multistate_button_set_state(wdgt, ix_state);
            }
        }
    }
    return wdgt;
}


widget_t* widget_hotspot(widget_t* wdgt, bool hotspot) {
    if (wdgt) {
        wdgt->hotspot = hotspot;
    }
    return wdgt;
}

bool widget_get_hotspot(widget_t* wdgt) {
    if (wdgt) {
        return wdgt->hotspot;
    }
    return false;
}

static void text_widget_render(widget_t* wdgt) {
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

widget_t* widget_create_text(const view_context_t* view) {
    widget_t* wdgt = widget_create(view);
    if (wdgt) {
        static SDL_Color white = {255, 255, 255, 255};
        _text_data_ptr txt_w = &wdgt->sub.text;
        *((widget_type_t*)&wdgt->type) = WIDGET_TEXT;
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

widget_t* widget_text_set_format(widget_t* wdgt, const char* format) {
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

const char* widget_text_get_format(widget_t* wdgt) {
    if (wdgt && wdgt->type == WIDGET_TEXT) {
        _text_data_ptr txt_w = &wdgt->sub.text;
        if (txt_w->format) {
            return txt_w->format;
        }
    }
    return "";
}


widget_t* widget_text_set_timedate_format(widget_t* wdgt, const char* format) {
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

const char* widget_text_get_timedate_format(widget_t* wdgt) {
    if (wdgt && wdgt->type == WIDGET_TEXT) {
        _text_data_ptr txt_w = &wdgt->sub.text;
        if (txt_w->timedate_format) {
            return txt_w->timedate_format;
        }
    }
    return "";
}

static void text_render_surface(widget_t* wdgt) {
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

widget_t* widget_text_set_content(widget_t* wdgt, const char* content) {
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

widget_t* widget_text_set_font(widget_t* wdgt, const char* font_path, int size) {
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

widget_t* widget_text_set_colour(widget_t* wdgt, SDL_Color colour) {
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

widget_t* widget_text_set_justification(widget_t* wdgt, const char* justif_name) {
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
widget_t* widget_configure(widget_t* wdgt) {
    switch(wdgt->type) {
        case WIDGET_NONE:
        case WIDGET_BUTTON:
        case WIDGET_MULTISTATE_BUTTON:
        case WIDGET_VUMETER:
        case WIDGET_TEXT:
        case WIDGET_END:
            wdgt->configured = true;
            break;
        case WIDGET_IMAGE:
            wdgt->configured = true;
            setup_image_fit_src_rect(wdgt);
            break;
        case WIDGET_SLIDER:
        case WIDGET_VSLIDER:
            wdgt->configured = true;
            slider_widget_configure(wdgt);
            break;
    }
    return wdgt;
}

void widget_dispatch_action(widget_t* wdgt) {
    if (widget_is_slider(wdgt)
                && (!wdgt->sub.slider.defined_interactive || !wdgt->sub.slider.interactive)) {
        return;
    }

    action_t act = wdgt->action;
    if (wdgt->type == WIDGET_MULTISTATE_BUTTON) {
        action_printf("action_multistate_button action state=%d %d %s\n", 
            wdgt->sub.multistate_button.state,
            wdgt->sub.multistate_button.res[wdgt->sub.multistate_button.state].dispatch_action,
            action_to_string( wdgt->sub.multistate_button.res[wdgt->sub.multistate_button.state].dispatch_action));
        act = wdgt->sub.multistate_button.res[wdgt->sub.multistate_button.state].dispatch_action;
    }

    action_printf("%p %d %s\n", wdgt, act, action_to_string(act));
    switch(act) {
        case ACTION_NONE:
        case ACTION_QUIT:
        case ACTION_NEXT_VISU:
        case ACTION_PREV_VISU:
        case ACTION_NEXT_VU:
        case ACTION_PREV_VU:
        case ACTION_NEXT_SP:
        case ACTION_PREV_SP:
        case ACTION_LOCK_VUMETER:
        case ACTION_UNLOCK_VUMETER:
        case ACTION_LOCK_VISU:
        case ACTION_UNLOCK_VISU:

        case ACTION_PLAY:
        case ACTION_PAUSE:
        case ACTION_STOP:
        case ACTION_PLAY_PAUSE:

        case ACTION_NEXT_TRACK:
        case ACTION_PREV_TRACK:

        case ACTION_REPEAT_ONCE:
        case ACTION_REPEAT:
        case ACTION_REPEAT_OFF:

        case ACTION_SHUFFLE:
        case ACTION_SHUFFLE_ALBUM:
        case ACTION_SHUFFLE_OFF:

        case ACTION_MUSIC_INFORMATION:

        case ACTION_NEXT_NP_VIEW:
        case ACTION_PREV_NP_VIEW:

        case ACTION_NP_VIEW:
        case ACTION_MAIN_VIEW:

        case ACTION_INCREMENT_VOLUME:
        case ACTION_DECREMENT_VOLUME:

        case ACTION_END:
            dispatch_action(act, 0);
            break;

        case ACTION_SET_VOLUME:
            if (widget_is_slider(wdgt)) {
                int level;
                widget_slider_get_value(wdgt, &level);
                dispatch_action(act, level);
            }
            break;

        case ACTION_SEEK:
            if (widget_is_slider(wdgt)) {
                int track_time;
                widget_slider_get_value(wdgt, &track_time);
                dispatch_action(act, track_time);
            }
            break;

        default:
            error_printf("unknown action %d for %p type=%d\n", act, wdgt, wdgt->type);
            break;
    }
}

// widget list

static widget_list_t* widget_list_initialise(widget_list_t* list, view_context_t* view) {
    if (list) {
        list->head.next = &list->tail;
        *((widget_type_t*)(&list->head.type)) = WIDGET_NONE;
        list->tail.prev = &list->head;
        *((widget_type_t*)(&list->tail.type)) = WIDGET_END;
        for(widget_t* w=&list->head; w != NULL; w=w->next) {
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

widget_list_t* create_widget_list(view_context_t* view) {
    return widget_list_initialise(calloc(sizeof(widget_list_t), 1), view);
}

widget_list_t* destroy_widgets_in_list(widget_list_t* list) {
    if (list) {
        widget_t* widget = list->head.next;
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

widget_list_t* destroy_widget_list(widget_list_t* list) {
    if (list) {
        destroy_widgets_in_list(list);
        free(list);
    }
    return NULL;
}

void widget_list_load_media(const widget_list_t* list, const char* resource_path) {
    for (widget_t* widget = list->head.next; widget != NULL; widget = widget->next) {
        widget_load_media(widget, resource_path);
    }
}

void widget_list_react(const widget_list_t* list, const pointer_input_t input, SDL_Point* pt) {
    bool selected = false;
    if(pt) {
        input_printf("%d: %04d,%04d -> ", input, pt->x, pt->y);
        translate_point(pt);
        input_printf(" %04d,%04d\n", pt->x, pt->y);
    }
    switch(input) {
        case POINTER_DOWN:
            if (NULL == pt) {
                return;
            }
            for(widget_t* widget=list->tail.prev; widget != NULL; widget=widget->prev) {
                if (widget->hidden) { continue;}
                if (!selected) {
                    widget->focussed = SDL_PointInRect(pt, &widget->input_rect) && (!widget->focus_disabled);
                    widget_set_highlight(widget, widget->focussed);
                    widget_set_pressed(widget, widget->focussed);
                    selected = widget->focussed;
                    if (widget_is_slider(widget)) {
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
            if (NULL == pt) {
                return;
            }
            for(widget_t* widget=list->tail.prev; widget != NULL; widget=widget->prev) {
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
                    if (widget_is_slider(widget)) {
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
            if (NULL == pt) {
                return;
            }
            for (widget_t* widget=list->tail.prev; widget != NULL; widget=widget->prev) {
                if (widget->hidden) { continue;}
                if (!selected) {
                    widget_set_highlight(widget, SDL_PointInRect(pt, &widget->input_rect) && (!widget->focus_disabled));
                    selected = widget_highlighted(widget);
                    if (selected) {
                        if (widget_is_slider(widget)) {
                            widget_slider_track(widget, pt);
                        }
                    }
                } else {
                    widget_set_highlight(widget, false);
                }
            }
            break;
        case NEXT_VISU:
        case NEXT_VU:
            for (widget_t* widget=list->tail.prev; widget != NULL; widget=widget->prev) {
                if (widget->type == WIDGET_VUMETER) {
                    widget_vumeter_select_next(widget);
                }
            }
            break;
        case PREV_VISU:
        case PREV_VU:
            for (widget_t* widget=list->tail.prev; widget != NULL; widget=widget->prev) {
                if (widget->type == WIDGET_VUMETER) {
                    widget_vumeter_select_prev(widget);
                }
            }
            break;
    }
}


bool widget_list_query_render_backdrop(const widget_list_t* wdgt_list) {
    if (wdgt_list) {
        for(widget_t* widget=wdgt_list->head.next; widget != NULL; widget=widget->next) {
            if (widget->redraw_required) {
                return true;
            }
        }
    }
    return false;
}

void widget_list_render_backdrop(const widget_list_t* wdgt_list) {
    if (NULL == wdgt_list) {
        return;
    }
    for(widget_t* widget=wdgt_list->head.next; widget != NULL; widget=widget->next) {
        if (widget->hidden) {
            widget->redraw_required = false;
        } else {
            widget->render_backdrop(widget);
        }
    }

    for(widget_t* widget=wdgt_list->head.next; widget != NULL; widget=widget->next) {
        if (widget->render_backdrop != NULL && widget->redraw_required) { error_printf("!? %d\n", widget->type); }
    }

}

void widget_list_render_foreground(const widget_list_t* wdgt_list) {
    if (NULL == wdgt_list) {
        return;
    }
    for(widget_t* widget=wdgt_list->head.next; widget != NULL; widget=widget->next) {
        if (!widget->hidden) {
            widget->render_foreground(widget);
            widget_render_foreground_default(widget);
        }
    }
}

widget_t* widget_list_head(const widget_list_t* wdgt_list) {
    return wdgt_list->head.next;
}
widget_t* widget_list_next(const widget_list_t* widget_list, widget_t *widget) {
    return widget->next != &widget_list->tail ? widget->next: NULL; 
}


widget_t* widget_list_tail(const widget_list_t* wdgt_list) {
    return wdgt_list->tail.prev;
}
widget_t* widget_list_prev(const widget_list_t* widget_list, widget_t *widget) {
    return widget->prev != &widget_list->head ? widget->prev: NULL; 
}
