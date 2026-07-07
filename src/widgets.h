#ifndef __jl_widgets_h_
#define __jl_widgets_h_
#include <SDL2/SDL.h>
#include <SDL2/SDL_ttf.h>

#include "types.h"
#include "application.h"
#include "util.h"
#include "actions.h"
#include "texture_cache.h"
#include "sdl_userevents.h"

#define  WH_FILL (-1)

// TODO: use setter/getter functions
extern bool debug_rects;
extern bool show_rects;
extern bool show_input_rects;

typedef enum {
    WIDGET_NONE,
    WIDGET_IMAGE,
    WIDGET_BUTTON,
    WIDGET_MULTISTATE_BUTTON,
    WIDGET_VUMETER,
    WIDGET_SLIDER,
    WIDGET_TEXT,
    WIDGET_VSLIDER,
    WIDGET_END
}widget_type_t;

typedef enum {
    IMAGE_FIT,
    IMAGE_CENTRED_FILL,
    IMAGE_STRETCH_FILL,
}image_scaling_t;

typedef enum {
    EDGE_NONE,
    EDGE_LEFT,
    EDGE_TOP,
    EDGE_RIGHT,
    EDGE_BOTTOM,
}hotspot_edge_t;

typedef enum {
    POINTER_DOWN,
    POINTER_UP,
    POINTER_MOTION,
    NEXT_VISU,
    NEXT_VU,
    PREV_VISU,
    PREV_VU
} pointer_input_t;

typedef enum {
    TXT_CENTRED,
    TXT_LEFT,
    TXT_RIGHT
} text_justification_t;

typedef enum {
    SLIDER_BAR,
    SLIDER_PICK,
    SLIDER_BAR_START,
    SLIDER_BAR_END,
    SLIDER_RESOURCE_COUNT
}slider_reosurce_ID_t;

typedef struct vumeter_widget vumeter_widget_t;
typedef struct spmeter_widget spmeter_widget_t;

typedef struct widget widget_t;
typedef struct view_context view_context_t, *view_context_ptr;

bool widget_highlighted(widget_t* wdgt); 
void widget_set_highlight(widget_t* wdgt, bool onoff);
bool widget_pressed(widget_t* wdgt);
void widget_set_pressed(widget_t* wdgt, bool onoff);
int  widget_get_pressed_millis(widget_t* wdgt);

widget_type_t widget_get_type(widget_t* wdgt);
const char* widget_type_name(widget_type_t typ);
const char* widget_get_type_name(widget_t *);
widget_t* widget_rect(widget_t *wdgt, const SDL_Rect *rect);
widget_t* widget_bounds(widget_t *wdgt, int x, int y, int w, int h);
widget_t* widget_set_player_value_key(widget_t* wdgt, const char* key);
const char* widget_get_player_value_key(widget_t* wdgt);
widget_t* widget_set_runtime_value_key(widget_t* wdgt, const char* key);
const char* widget_get_runtime_value_key(widget_t* wdgt);
// TODO: fix implicit range start value of 0
widget_t* widget_set_player_range_value_key(widget_t* wdgt, const char* key);
const char* widget_get_player_range_value_key(widget_t* wdgt);

widget_t* widget_load_media(widget_t* wdgt, const char* resource_path);
widget_t* widget_destroy(widget_t* wdgt);
widget_t* widget_action(widget_t* wdgt, action_t action);
bool widget_has_action(widget_t* wdgt, action_t action);
action_t widget_get_action(widget_t* wdgt);
widget_t* widget_hide(widget_t* wdgt, bool hide);
bool widget_is_hidden(widget_t* wdgt);
widget_t* widget_hotspot(widget_t* wdgt, bool hotspot);
widget_t* widget_hotspot_edge(widget_t* wdgt, hotspot_edge_t edge, SDL_Rect *r);
bool widget_get_hotspot(widget_t* wdgt);
widget_t* widget_image_path(widget_t* wdgt, const char* path);
widget_t* widget_focus_enable(widget_t* wdgt, bool f);
widget_t* widget_set_focussed(widget_t* wdgt, bool f);
bool widget_get_focussed(widget_t* wdgt);

// DO NOT invoke in render thread
widget_t* widget_configure(widget_t* wdgt);

// Button widget
widget_t* widget_create_button(const view_context_t*);

// Multistate Button widget
widget_t* widget_create_multistate_button(const view_context_t*, int state_count);
widget_t* widget_multistate_button_addstate(widget_t* wdgt, unsigned statenum, const char* resource_path, action_t dispatch_action, action_t sync_on_action);
widget_t* widget_multistate_button_set_state(widget_t* wdgt, unsigned statenum);
widget_t* widget_multistate_button_get_state(widget_t* wdgt, unsigned* statenum);
widget_t* widget_multistate_button_sync_on_action(widget_t* wdgt, action_t act);

// Image widget
widget_t* widget_create_image(const view_context_t*);
widget_t* widget_image_scaling(widget_t *wdgt, image_scaling_t op);

// VUMeter widget
widget_t* widget_create_vumeter(const view_context_t*);
widget_t* widget_vumeter_select_next(widget_t *wdgt);
widget_t* widget_vumeter_select_prev(widget_t *wdgt);
widget_t* widget_vumeter_select_by_name(widget_t *wdgt, const char* name);
widget_t* widget_vumeter_select_lock(widget_t *wdgt, bool lock);
widget_t* widget_vumeter_equal_horizontal_spacing(widget_t *wdgt, bool val);

// Slider widgets
widget_t* widget_create_slider(const view_context_t*);
widget_t *widget_create_vslider(const view_context_t* view);
widget_t* widget_slider_image_paths(widget_t* , slider_reosurce_ID_t id, const char* path1, const char* path2);
widget_t* widget_slider_image_width(widget_t* , slider_reosurce_ID_t id, int width);
widget_t* widget_slider_image_height(widget_t* , slider_reosurce_ID_t id, int height);
widget_t* widget_slider_range(widget_t* , int start, int end);
widget_t* widget_slider_update_value(widget_t* wdgt, int value, bool* in_range);
widget_t* widget_slider_set_interactive(widget_t* wdgt, bool yn);
widget_t* widget_slider_define_interactive(widget_t* wdgt, bool yn);
widget_t* widget_slider_get_value(widget_t* wdgt, int* value);

// Text widget
widget_t* widget_create_text(const view_context_t*);
widget_t* widget_text_set_format(widget_t*, const char* format);
const char* widget_text_get_format(widget_t*);
widget_t* widget_text_set_timedate_format(widget_t*, const char* format);
const char* widget_text_get_timedate_format(widget_t*);
widget_t* widget_text_set_content(widget_t*, const char* content);
widget_t* widget_text_set_font(widget_t*, const char* font_path, int size);
widget_t* widget_text_set_colour(widget_t*, SDL_Color colour);
widget_t* widget_text_set_justification(widget_t*, const char*);
widget_t* widget_text_set_y_scaling_threshold(widget_t* wdgt, float threshold);

// widget render 
widget_t* widget_set_renderhf(widget_t* wdgt);
widget_t* widget_unset_renderhf(widget_t* wdgt);

// Widget list and view
typedef struct widget_list widget_list_t;
struct view_context {
    app_context_ptr     app;
    widget_list_t*      list;
};

widget_list_t* create_widget_list(view_context_t* view);
widget_list_t* destroy_widget_list(widget_list_t*);
widget_list_t* destroy_widgets_in_list(widget_list_t*);

void widget_list_load_media(const widget_list_t* list, const char* resource_path);
void widget_list_react(const widget_list_t* list, const pointer_input_t input, SDL_Point* pt);
bool widget_list_query_render_backdrop(const widget_list_t* wdgt_list);
void widget_list_render_backdrop(const widget_list_t* wdgt_list);
void widget_list_render_foreground(const widget_list_t* wdgt_list);
widget_t* widget_list_next(const widget_list_t* widget_list, widget_t *widget);
widget_t* widget_list_prev(const widget_list_t* widget_list, widget_t *widget);
widget_t* widget_list_head(const widget_list_t* wdgt_list);
widget_t* widget_list_tail(const widget_list_t* wdgt_list);

bool widget_is_slider(widget_t* w);
#endif // __jl_widgets_h_
