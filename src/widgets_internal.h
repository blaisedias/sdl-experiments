#ifndef __jl_widgets_internal_h_
#define __jl_widgets_internal_h_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include "application.h"
#include "widgets.h"

// Generic
widget* widget_create(const view_context *view);
void render_none(widget* wdgt);

// VUmeter
widget *vumeter_widget_destroy(widget *wdgt);
void vumeter_widget_load_media(widget *wdgt, const char* resource_path);
// Slider
widget *widget_slider_track(widget* wdgt, const SDL_Point *pt);
widget *widget_slider_tracking_commit(widget* wdgt, const SDL_Point *pt);
_slider_workspace* slider_widget_configure(widget* wdgt);

#endif
