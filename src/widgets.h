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
    POINTER_MOTION
} pointer_input_t;

typedef enum {
    TXT_CENTRED,
    TXT_LEFT,
    TXT_RIGHT
} text_justification_t;

typedef struct vumeter_widget vumeter_widget_t;
typedef struct spmeter_widget spmeter_widget_t;

typedef struct widget widget_t;
typedef struct view_context view_context_t, *view_context_ptr;

typedef struct {
    texture_id_t    texture_id;
    const char      *resource_path;
    action_t        dispatch_action;
    action_t        sync_on_action;
}_bnt_resource_t;

typedef struct {
    const char*     image_paths[2];
    texture_id_t    texture_ids[2];
    int w;
    int h;
}_slider_resource_t;
    
typedef enum {
    SLIDER_BAR,
    SLIDER_PICK,
    SLIDER_BAR_START,
    SLIDER_BAR_END,
    SLIDER_RESOURCE_COUNT
}slider_reosurce_ID_t;

typedef struct {
    bool initialised;
    int  value_range_delta;
    int  half_pw;
    int  min_pos;
    int  max_pos;
    int  current_pos;
    int  drag_pos;
    SDL_Rect bar_start_rect;
    SDL_Rect bar_end_rect;
    SDL_Rect bar_rect;
//    SDL_Rect bar_empty_rect;
    SDL_Rect pick_rect;
    int  pick_x2;
}_slider_workspace_t;

typedef struct {
    texture_id_t texture_id;
    TTF_Font* font;
    const char* name;   // name is used for texture cache
    const char* format; // player format string, can be NULL
    const char* timedate_format; // time date format string, can be NULL
    const char* content;
//    SDL_Rect content_dim;
    SDL_Color colour;
    SDL_Rect dst_rect;
    text_justification_t justification;
}_text_data_t,*_text_data_ptr;

struct widget {
    widget_t*    next;
    widget_t*   prev;
    const       widget_type_t type;
    const       view_context_t* view;
    
    action_t    action;
    void        (*render_backdrop)(widget_t*);
    void        (*render_foreground)(widget_t*);
    bool        render_as_foreground;
    bool        focussed;
    // 
    bool        atomic_highlight;
    bool        hidden;
    bool        hotspot;
    const bool  focus_disabled;
    // generic image path for all widgets with single images
    // can be NULL
    const char* image_path;
    
    SDL_Rect    rect;
    SDL_Rect    input_rect;
    // For now 2 translated rectangles are used to handle 
    // orientation correctly. 
    // TODO further investigation.
    // 1 - for rotated images
    // SDL_Rect    image_rect;
    // 2 - for unrotated operations like DrawRect, FillRect
    //SDL_Rect    draw_rect;

    bool         atomic_pressed;
    int64_t      pressed_millis_start;
    int64_t      pressed_millis_end;
    const char*  player_value_key;
    const char*  player_range_value_key;
    const char*  runtime_value_key;
    volatile     bool redraw_required;
    volatile     bool foreground;
    volatile     bool configured;
    union {
        vumeter_widget_t* vu;
        spmeter_widget_t* sp;
        struct {
            texture_id_t texture_id;
            int w;
            int h;
            image_scaling_t scale_op;
            SDL_Rect src_rect;
            SDL_Rect dst_rect;
        }image;
        struct {
            texture_id_t texture_id;
        }button;
        struct {
            unsigned state;
            unsigned state_count;
            _bnt_resource_t* res;
        }multistate_button;
        struct {
            // interactive property as defined
            bool defined_interactive;
            // interactive property runtime controlled
            bool interactive;
            struct {
                int start;
                int end;
            }range;
            _slider_resource_t res[SLIDER_RESOURCE_COUNT];
            _slider_workspace_t wk;
        }slider;
        _text_data_t text;
    }sub;
};

bool widget_highlighted(widget_t* wdgt); 
void widget_set_highlight(widget_t* wdgt, bool onoff);
bool widget_pressed(widget_t* wdgt);
void widget_set_pressed(widget_t* wdgt, bool onoff);
int  widget_get_pressed_millis(widget_t* wdgt);

const char* widget_type_name(widget_type_t typ);
widget_t* widget_rect(widget_t *wdgt, const SDL_Rect *rect);
widget_t* widget_bounds(widget_t *wdgt, int x, int y, int w, int h);
widget_t* widget_set_player_value_key(widget_t* wdgt, const char* key);
widget_t* widget_set_runtime_value_key(widget_t* wdgt, const char* key);
// TODO: fix implicit range start value of 0
widget_t* widget_set_player_range_value_key(widget_t* wdgt, const char* key);

widget_t* widget_load_media(widget_t* wdgt, const char* resource_path);
widget_t* widget_destroy(widget_t* wdgt);
widget_t* widget_action(widget_t* wdgt, action_t action);
bool widget_has_action(widget_t* wdgt, action_t action);
widget_t* widget_hide(widget_t* wdgt, bool hide);
widget_t* widget_hotspot(widget_t* wdgt, bool hotspot);
widget_t* widget_hotspot_edge(widget_t* wdgt, hotspot_edge_t edge, SDL_Rect *r);
widget_t* widget_image_path(widget_t* wdgt, const char* path);
widget_t* widget_focus_enable(widget_t* wdgt, bool f);

// DO NOT invoke in render thread
widget_t* widget_configure(widget_t* wdgt);

widget_t* widget_create_button(const view_context_t*);
widget_t* widget_create_multistate_button(const view_context_t*, int state_count);
widget_t* widget_multistate_button_addstate(widget_t* wdgt, unsigned statenum, const char* resource_path, action_t dispatch_action, action_t sync_on_action);
widget_t* widget_multistate_button_set_state(widget_t* wdgt, unsigned statenum);
widget_t* widget_multistate_button_get_state(widget_t* wdgt, unsigned* statenum);
widget_t* widget_multistate_button_sync_on_action(widget_t* wdgt, action_t act);

widget_t* widget_create_image(const view_context_t*);
widget_t* widget_image_scaling(widget_t *wdgt, image_scaling_t op);

widget_t* widget_create_vumeter(const view_context_t*);
widget_t* widget_vumeter_select_next(widget_t *wdgt);
widget_t* widget_vumeter_select_prev(widget_t *wdgt);
widget_t* widget_vumeter_select_by_name(widget_t *wdgt, const char* name);
widget_t* widget_vumeter_select_lock(widget_t *wdgt, bool lock);
widget_t* widget_vumeter_equal_horizontal_spacing(widget_t *wdgt, bool val);

widget_t* widget_create_slider(const view_context_t*);
widget_t* widget_slider_range(widget_t* , int start, int end);
//widget_t* widget_slider_set_value(widget_t* wdgt, int value);
widget_t* widget_slider_update_value(widget_t* wdgt, int value);
widget_t* widget_slider_set_interactive(widget_t* wdgt, bool yn);
widget_t* widget_slider_define_interactive(widget_t* wdgt, bool yn);
widget_t* widget_slider_get_value(widget_t* wdgt, int* value);
widget_t* widget_slider_image_paths(widget_t* , slider_reosurce_ID_t id, const char* path1, const char* path2);
widget_t* widget_slider_image_width(widget_t* , slider_reosurce_ID_t id, int width);
widget_t* widget_slider_image_height(widget_t* , slider_reosurce_ID_t id, int height);

widget_t* widget_create_text(const view_context_t*);
widget_t* widget_text_set_format(widget_t*, const char* format);
widget_t* widget_text_set_timedate_format(widget_t*, const char* format);
widget_t* widget_text_set_content(widget_t*, const char* content);
widget_t* widget_text_set_font(widget_t*, const char* font_path, int size);
widget_t* widget_text_set_colour(widget_t*, SDL_Color colour);
widget_t* widget_text_set_justification(widget_t*, const char*);

typedef struct {
    widget_t head;
    widget_t tail;
}widget_list_t;

struct view_context {
    app_context_ptr     app;
    widget_list_t*      list;
};

widget_list_t* create_widget_list(view_context_t* view);
widget_list_t* destroy_widget_list(widget_list_t*);
widget_list_t* destroy_widgets_in_list(widget_list_t*);

void widget_dispatch_action(widget_t* wdgt);
void widget_list_load_media(const widget_list_t* list, const char* resource_path);
void widget_list_react(const widget_list_t* list, const pointer_input_t input, SDL_Point* pt);
bool widget_list_query_render_backdrop(const widget_list_t* wdgt_list);
void widget_list_render_backdrop(const widget_list_t* wdgt_list);
void widget_list_render_foreground(const widget_list_t* wdgt_list);

widget_t* widget_set_renderhf(widget_t* wdgt);
widget_t* widget_unset_renderhf(widget_t* wdgt);
void widget_render_foreground_default(widget_t* wdgt);

#endif // __jl_widgets_h_
