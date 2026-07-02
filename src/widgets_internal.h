#ifndef __jl_widgets_internal_h_
#define __jl_widgets_internal_h_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include "application.h"
#include "widgets.h"

// Generic
widget_t* widget_create(const view_context_t *view);
void render_none(widget_t* wdgt);

// VUmeter
widget_t *vumeter_widget_destroy(widget_t *wdgt);
void vumeter_widget_load_media(widget_t *wdgt, const char* resource_path);
// Slider
widget_t *widget_slider_track(widget_t* wdgt, const SDL_Point *pt);
widget_t *widget_slider_tracking_commit(widget_t* wdgt, const SDL_Point *pt);
_slider_workspace_t* slider_widget_configure(widget_t* wdgt);

#endif
