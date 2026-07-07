#ifndef __jl_widgets_internal_h_
#define __jl_widgets_internal_h_

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include "application.h"
#include "widgets.h"

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
 
typedef struct {
    bool initialised;
    int  value_range_delta;
    int  half_pick_dim;
    int  min_pos;
    int  max_pos;
    int  current_pos;
    int  drag_pos;
    SDL_Rect bar_start_rect;
    SDL_Rect bar_end_rect;
    SDL_Rect bar_rect;
//    SDL_Rect bar_empty_rect;
    SDL_Rect pick_rect;
}_slider_workspace_t;

// default value 1.25 => 80% which is an empricially determined value.
#define DEFAULT_WIDGET_TEXT_Y_SCALING_THRESHOLD 1.25
typedef struct {
    texture_id_t texture_id;
    TTF_Font* font;
    const char* name;   // name is used for texture cache
    const char* format; // player format string, can be NULL
    const char* timedate_format; // time date format string, can be NULL
    const char* content;
    SDL_Color   colour;
    SDL_Rect    dst_rect;
    // threshold for scaling text based on height
    float       y_scaling_threshold;
    text_justification_t justification;
}_text_data_t,*_text_data_ptr;

struct widget_s_t {
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

struct widget_list_s_t {
    widget_t head;
    widget_t tail;
};

// Generic
widget_t* widget_create(const view_context_t *view);
void render_none(widget_t* wdgt);
void widget_render_foreground_default(widget_t* wdgt);
void widget_dispatch_action(widget_t* wdgt);

// VUmeter
widget_t *vumeter_widget_destroy(widget_t *wdgt);
void vumeter_widget_load_media(widget_t *wdgt, const char* resource_path);
// Slider
widget_t *widget_slider_track(widget_t* wdgt, const SDL_Point *pt);
widget_t *widget_slider_tracking_commit(widget_t* wdgt, const SDL_Point *pt);
_slider_workspace_t* slider_widget_configure(widget_t* wdgt);
#endif
