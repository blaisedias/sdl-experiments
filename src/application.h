#include <SDL2/SDL.h>
#ifndef __jl_application_h_
#define __jl_application_h_
#include "types.h"
//#include "lyrion_player.h"

typedef struct {
    unsigned        reported_fps;
} app_workspace_t;

typedef struct app_context app_context;
typedef const struct app_context* app_context_ptr;

typedef void (*renderfn)(app_context_ptr app_ctx);
typedef void (*inputfn)(app_context_ptr app_ctx, SDL_Event* event);

struct app_context {
    float           orientation;
    SDL_Window*     window;
    SDL_Rect        window_rect;
    const char*     window_title;

    SDL_Renderer*       renderer;
    const SDL_threadID  renderer_tid;
    renderfn            cb_render;
    SDL_Texture*        target_texture;

    Uint32          pixelFormat;
    unsigned        bytes_per_pixel;
    bool            fullscreen;

    const Uint8*    keystate;

    int             screen_width;
    int             screen_height;

    int             max_secs;
    int             cycle_secs;
    int             vsync;
    int             refresh_rate;
    int             frame_time_millis;
    int             frame_time_micros;

    bool            profile_fps_deviation;

    const           char* lms;

    const char*     default_font_path;

    int             max_texture_width;
    int             max_texture_height;

    SDL_Thread*     input_thread;
    inputfn         cb_input;
    int64_t         input_loop_sleep_millis;

    app_workspace_t workspace; 
};

bool app_initialize(app_context_ptr app_ctx, const char* window_title);
void app_cleanup(app_context_ptr app, int exit_status);
void app_stop(app_context_ptr app_ctx);
//void app_input_loop(app_context_ptr app_ctx);
void app_render_loop(app_context_ptr app_ctx);
int app_render(app_context* app_ctx,  SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, const SDL_Point *center, const SDL_RendererFlip flip);
int app_render_rotated(app_context* app_ctx,  SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, const SDL_Point *center, const SDL_RendererFlip flip, double rotate);
bool app_running(app_context_ptr app_ctx);
void app_wait_ready();
int64_t app_get_render_count();

#endif // __jl_application_h_
