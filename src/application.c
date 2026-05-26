#include <unistd.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_ttf.h>
#include <SDL2/SDL_syswm.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_blendmode.h>

#include "application.h"
#include "util.h"
#include "logging.h"
#include "timing.h"
#include "event_queue.h"
#include "touch_screen_sdl2.h"
#include "texture_cache.h"

#define HIDE_CURSOR_COUNT  50
#define IMAGE_FLAGS IMG_INIT_PNG

static SDL_RendererFlags render_flags = SDL_RENDERER_ACCELERATED;
static volatile bool input_loop = true;
static volatile bool render_loop = true;
static int64_t render_iters;
static bool render_ready = false;
static bool input_ready = false;

// metrics
#define FPS_SAMPLE_COUNT 60
#define FPS_DIST_MAX  120
static int fps_distribution[FPS_DIST_MAX+1];

static inline void free_ex(void** tgt) {
    if (*tgt) {
        free(*tgt);
    }
    *tgt = NULL;
}
#define FREE(x) free_ex((void **)(&x))

typedef struct {
    int64_t render_count;
    int64_t  micros;
}fps_sample_point;

void ___app_input_loop(app_context* app_ctx);

bool app_initialize(app_context_ptr app_ctx_in, const char* window_title) {
    app_context* app_ctx = (app_context*)app_ctx_in;
    app_ctx->input_thread =  SDL_CreateThread((SDL_ThreadFunction)___app_input_loop, "input", app_ctx);

//    app_ctx->workspace.player_mode = PLAYER_MODE_UNDEFINED;

//    app_ctx->player = open_local_player(app_ctx->lms);
    app_ctx->default_font_path = "fonts/FreeSans.ttf";

    if (app_ctx->vsync) {
        render_flags |=  SDL_RENDERER_PRESENTVSYNC;
    }

    if (SDL_Init(SDL_INIT_EVERYTHING)) {
        error_printf("initializing SDL: %s\n", SDL_GetError());
        return true;
    }

    app_ctx->keystate = SDL_GetKeyboardState(NULL);
    int img_init = IMG_Init(IMAGE_FLAGS);
    if ((img_init & IMAGE_FLAGS) != IMAGE_FLAGS) {
        error_printf("initializing SDL_image: %s\n", IMG_GetError());
        return true;
    }

    if (TTF_Init()) {
        error_printf("initializing SDL_ttf: %s\n", IMG_GetError());
        return true;
    }

    int num_displays = SDL_GetNumVideoDisplays();
    for (int i_display = 0; i_display < num_displays; ++ i_display) {
            SDL_DisplayMode dm;
            if (0 == SDL_GetCurrentDisplayMode(i_display, &dm)) {
                // TODO: handle multiple displays?
                if (i_display == 0) {
                    if (dm.refresh_rate) {
                        app_ctx->refresh_rate = dm.refresh_rate;
                        app_ctx->frame_time_millis = 1000/dm.refresh_rate;
                        app_ctx->frame_time_micros = 1000000/dm.refresh_rate;
                    } else {
                        printf("Warning refresh rate returned is %d defaulting to 60Hz\n", dm.refresh_rate);
                        app_ctx->frame_time_millis = 1000/60;
                        app_ctx->frame_time_micros = 1000000/60;
                    }
                    printf("Display:%d fmt=%x, w=%d, h=%d, refresh rate:%d %d milliSeconds %d microSeconds\n",
                            i_display,
                            dm.format, dm.w, dm.h,
                            (int)dm.refresh_rate,
                            (int)app_ctx->frame_time_millis,
                            (int)app_ctx->frame_time_micros);
                }
            } 
    }

    SDL_Window *window;

    sleep(1);
    window =  SDL_CreateWindow(window_title, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 0, 0, SDL_WINDOW_FULLSCREEN_DESKTOP);

    SDL_SysWMinfo swmi;
    if (SDL_GetWindowWMInfo(window, &swmi)) {
        SDL_DestroyWindow(window);
        switch(swmi.subsystem) {
           case SDL_SYSWM_UNKNOWN:
               // puts("SDL_SYSWM_UNKNOWN");
               break;
           case SDL_SYSWM_WINDOWS:
               // puts("SDL_SYSWM_WINDOWS");
               break;
           case SDL_SYSWM_X11:
               // puts("SDL_SYSWM_X11");
               break;
           case SDL_SYSWM_DIRECTFB:
               // puts("SDL_SYSWM_DIRECTFB");
               break;
           case SDL_SYSWM_COCOA:
               // puts("SDL_SYSWM_COCOA");
               break;
           case SDL_SYSWM_UIKIT:
               // puts("SDL_SYSWM_UIKIT");
               break;
           case SDL_SYSWM_WAYLAND:
/*               
               if (orientation == 90.0 || orientation ==270) {
                   int tmp = screen_width;
                   screen_width = screen_height;
                   screen_height = tmp;
               }
*/
                // puts("SDL_SYSWM_WAYLAND");
                if (app_ctx->fullscreen) {
                    app_ctx->window = SDL_CreateWindow(window_title,
                           SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                           0, 0,
                           SDL_WINDOW_FULLSCREEN_DESKTOP);
                } else {
                    app_ctx->window = SDL_CreateWindow(window_title,
                            SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                            app_ctx->screen_width, app_ctx->screen_height,
                            0);
                }
               break;
           case SDL_SYSWM_MIR:
               // puts("SDL_SYSWM_MIR");
               break;
           case SDL_SYSWM_WINRT:
               // puts("SDL_SYSWM_WINRT");
               break;
           case SDL_SYSWM_ANDROID:
               // puts("SDL_SYSWM_ANDROID");
               break;
           case SDL_SYSWM_VIVANTE:
               // puts("SDL_SYSWM_VIVANTE");
               break;
           case SDL_SYSWM_OS2:
               // puts("SDL_SYSWM_OS2");
               break;
           case SDL_SYSWM_HAIKU:
               // puts("SDL_SYSWM_HAIKU");
               break;
           case SDL_SYSWM_KMSDRM:
                puts("SDL_SYSWM_KMSDRM");
                app_ctx->window = SDL_CreateWindow(window_title,
                       SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                       0, 0,
                       SDL_WINDOW_FULLSCREEN_DESKTOP);
               break;
           case SDL_SYSWM_RISCOS:
               // puts("SDL_SYSWM_RISCOS");
               break;
        }
    }

    if (!app_ctx->window) {
        error_printf("creating window: %s\n", SDL_GetError());
        return true;
    }
//    app_ctx->renderer = SDL_CreateRenderer(app_ctx->window, -1, 0);
    app_ctx->renderer = SDL_CreateRenderer(app_ctx->window, -1, render_flags);
    if (!app_ctx->renderer) {
        error_printf("creating renderer: %s\n", SDL_GetError());
        return true;
    }
    tcache_set_renderer_tid(SDL_GetThreadID(NULL));
    SDL_GetWindowSize(app_ctx->window, &app_ctx->screen_width, &app_ctx->screen_height);
    app_ctx->pixelFormat = SDL_GetWindowPixelFormat(app_ctx->window);
    app_ctx->bytes_per_pixel = SDL_BYTESPERPIXEL(app_ctx->pixelFormat);

//    srand((unsigned)time(NULL));
    SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "linear");
    SDL_RenderSetLogicalSize(app_ctx->renderer, app_ctx->screen_width, app_ctx->screen_height);
    if (SDL_SetRenderDrawBlendMode(app_ctx->renderer, SDL_BLENDMODE_ADD)) {
        error_printf("Failed to set renderer blend mode\n");
    }

    printf("pixelFormat: 0x%x %u bytes/pixel ", app_ctx->pixelFormat, app_ctx->bytes_per_pixel);
    switch(app_ctx->pixelFormat) {
        case SDL_PIXELFORMAT_UNKNOWN: printf("SDL_PIXELFORMAT_UNKNOWN\n"); break;
        case SDL_PIXELFORMAT_INDEX1LSB: printf("SDL_PIXELFORMAT_INDEX1LSB\n"); break;
        case SDL_PIXELFORMAT_INDEX1MSB: printf("SDL_PIXELFORMAT_INDEX1MSB\n"); break;
        case SDL_PIXELFORMAT_INDEX4LSB: printf("SDL_PIXELFORMAT_INDEX4LSB\n"); break;
        case SDL_PIXELFORMAT_INDEX4MSB: printf("SDL_PIXELFORMAT_INDEX4MSB\n"); break;
        case SDL_PIXELFORMAT_INDEX8: printf("SDL_PIXELFORMAT_INDEX8\n"); break;
        case SDL_PIXELFORMAT_RGB332: printf("SDL_PIXELFORMAT_RGB332\n"); break;
        case SDL_PIXELFORMAT_RGB444: printf("SDL_PIXELFORMAT_RGB444\n"); break;
        case SDL_PIXELFORMAT_RGB555: printf("SDL_PIXELFORMAT_RGB555\n"); break;
        case SDL_PIXELFORMAT_BGR555: printf("SDL_PIXELFORMAT_BGR555\n"); break;
        case SDL_PIXELFORMAT_ARGB4444: printf("SDL_PIXELFORMAT_ARGB4444\n"); break;
        case SDL_PIXELFORMAT_RGBA4444: printf("SDL_PIXELFORMAT_RGBA4444\n"); break;
        case SDL_PIXELFORMAT_ABGR4444: printf("SDL_PIXELFORMAT_ABGR4444\n"); break;
        case SDL_PIXELFORMAT_BGRA4444: printf("SDL_PIXELFORMAT_BGRA4444\n"); break;
        case SDL_PIXELFORMAT_ARGB1555: printf("SDL_PIXELFORMAT_ARGB1555\n"); break;
        case SDL_PIXELFORMAT_RGBA5551: printf("SDL_PIXELFORMAT_RGBA5551\n"); break;
        case SDL_PIXELFORMAT_ABGR1555: printf("SDL_PIXELFORMAT_ABGR1555\n"); break;
        case SDL_PIXELFORMAT_BGRA5551: printf("SDL_PIXELFORMAT_BGRA5551\n"); break;
        case SDL_PIXELFORMAT_RGB565: printf("SDL_PIXELFORMAT_RGB565\n"); break;
        case SDL_PIXELFORMAT_BGR565: printf("SDL_PIXELFORMAT_BGR565\n"); break;
        case SDL_PIXELFORMAT_RGB24: printf("SDL_PIXELFORMAT_RGB24\n"); break;
        case SDL_PIXELFORMAT_BGR24: printf("SDL_PIXELFORMAT_BGR24\n"); break;
        case SDL_PIXELFORMAT_RGB888: printf("SDL_PIXELFORMAT_RGB888\n"); break;
        case SDL_PIXELFORMAT_RGBX8888: printf("SDL_PIXELFORMAT_RGBX8888\n"); break;
        case SDL_PIXELFORMAT_BGR888: printf("SDL_PIXELFORMAT_BGR888\n"); break;
        case SDL_PIXELFORMAT_BGRX8888: printf("SDL_PIXELFORMAT_BGRX8888\n"); break;
        case SDL_PIXELFORMAT_ARGB8888: printf("SDL_PIXELFORMAT_ARGB8888\n"); break;
        case SDL_PIXELFORMAT_RGBA8888: printf("SDL_PIXELFORMAT_RGBA8888\n"); break;
        case SDL_PIXELFORMAT_ABGR8888: printf("SDL_PIXELFORMAT_ABGR8888\n"); break;
        case SDL_PIXELFORMAT_BGRA8888: printf("SDL_PIXELFORMAT_BGRA8888\n"); break;
        case SDL_PIXELFORMAT_ARGB2101010: printf("SDL_PIXELFORMAT_ARGB2101010\n"); break;
//        case SDL_PIXELFORMAT_RGBA32: printf("SDL_PIXELFORMAT_RGBA32\n"); break;
//        case SDL_PIXELFORMAT_ARGB32: printf("SDL_PIXELFORMAT_ARGB32\n"); break;
//        case SDL_PIXELFORMAT_BGRA32: printf("SDL_PIXELFORMAT_BGRA32\n"); break;
//        case SDL_PIXELFORMAT_ABGR32: printf("SDL_PIXELFORMAT_ABGR32\n"); break;
        case SDL_PIXELFORMAT_YV12: printf("SDL_PIXELFORMAT_YV12\n"); break;
        case SDL_PIXELFORMAT_IYUV: printf("SDL_PIXELFORMAT_IYUV\n"); break;
        case SDL_PIXELFORMAT_YUY2: printf("SDL_PIXELFORMAT_YUY2\n"); break;
        case SDL_PIXELFORMAT_UYVY: printf("SDL_PIXELFORMAT_UYVY\n"); break;
        case SDL_PIXELFORMAT_YVYU: printf("SDL_PIXELFORMAT_YVYU\n"); break;
        case SDL_PIXELFORMAT_NV12: printf("SDL_PIXELFORMAT_NV12\n"); break;
        case SDL_PIXELFORMAT_NV21: printf("SDL_PIXELFORMAT_NV21\n"); break;
    }

    setup_orientation(app_ctx->orientation, app_ctx->screen_width, app_ctx->screen_height, &app_ctx->window_rect);
    app_ctx->target_texture = SDL_CreateTexture(app_ctx->renderer, app_ctx->pixelFormat, SDL_TEXTUREACCESS_TARGET, app_ctx->window_rect.w, app_ctx->window_rect.h);
    app_printf("target_texture = %p %d x %d\n", app_ctx->target_texture, app_ctx->window_rect.w, app_ctx->window_rect.h);
    {
        SDL_RendererInfo info;
        if (0 == SDL_GetRendererInfo(app_ctx->renderer, &info)) {
            app_ctx->max_texture_width = info.max_texture_width;
            app_ctx->max_texture_width = info.max_texture_height;
            printf(
                "Renderer info:\n"
                "    name=%s\n"
                "    max_texture_width=%d\n"
                "    max_texture_height=%d\n",
                info.name,
                info.max_texture_width,
                info.max_texture_height
               );
        } else {
            error_printf("Failed to retrieve renderer information\n");
        }
        printf("display:%dx%d Orientation:%f,  max seconds:%u cycle secs %u\n",
           app_ctx->screen_width,
           app_ctx->screen_height,
           app_ctx->orientation,
           app_ctx->max_secs,
           app_ctx->cycle_secs);
    }
    
    return false;
}

void app_cleanup(app_context_ptr app_ctx_in, int exit_status) {
    app_context* app_ctx = (app_context*)app_ctx_in;
    if (app_ctx->input_thread) {
        input_loop = false;
        __atomic_store_n(&render_ready, true, __ATOMIC_RELEASE);
        __atomic_store_n(&input_ready, true, __ATOMIC_RELEASE);
        SDL_WaitThread(app_ctx->input_thread, NULL);
        app_ctx->input_thread = NULL;
    }
    app_printf("app_cleanup\n");
//    close_local_player(app_ctx->player);
    tcache_shutdown();
    if (app_ctx->target_texture) {
        SDL_DestroyTexture(app_ctx->target_texture);
        app_ctx->target_texture = NULL;
    }
    SDL_DestroyRenderer(app_ctx->renderer);
    SDL_DestroyWindow(app_ctx->window);
    TTF_Quit();
    IMG_Quit();
    SDL_Quit();
    exit(exit_status);
}

void app_render_loop(app_context_ptr app_ctx_in) {
    app_context* app_ctx = (app_context*)app_ctx_in;
    app_printf("+++ render start\n"); fflush(stdout);

    SDL_Rect dst_rect;

    copyRect(&app_ctx->window_rect, &dst_rect);
    translate_screen_rect(&dst_rect);
    app_printf("display rect {%d,%d,%d,%d} -> dest rotated {%d,%d,%d,%d}\n",
            app_ctx->window_rect.x, app_ctx->window_rect.y, app_ctx->window_rect.w, app_ctx->window_rect.h,
            dst_rect.x, dst_rect.y, dst_rect.w, dst_rect.h);

    // metrics valriables
    int64_t ms_00 = 0;
    uint32_t low_fps_count=0;
    unsigned fps_sample_counter = 0;

    if (app_ctx->vsync) {
        SDL_RenderSetVSync(app_ctx->renderer, 1);
    }
    SDL_RenderClear(app_ctx->renderer);
    SDL_RenderPresent(app_ctx->renderer);
    SDL_RenderClear(app_ctx->renderer);
    SDL_RenderPresent(app_ctx->renderer);
    __atomic_store_n(&render_ready, true, __ATOMIC_RELEASE);
    fps_sample_point fsp1 = {.render_count=render_iters, .micros=get_micro_seconds()};
    while(render_loop) {
        {
            SDL_Event event;
            int64_t endtime = get_micro_seconds() + 1000;
            while (SDL_PollEvent(&event) && endtime > get_micro_seconds()) {
                evt_q_put(&event);
            }
        }
        tcache_render_prep(app_ctx->renderer);
        SDL_RenderClear(app_ctx->renderer);
        SDL_SetRenderTarget(app_ctx->renderer, app_ctx->target_texture);
        SDL_RenderClear(app_ctx->renderer);

        app_ctx->cb_render(app_ctx);

        SDL_SetRenderTarget(app_ctx->renderer, NULL);
        SDL_RenderCopyEx(app_ctx->renderer,
                    app_ctx->target_texture,
                    NULL, &dst_rect, app_ctx->orientation, NULL, SDL_FLIP_NONE);
        
        SDL_RenderPresent(app_ctx->renderer);
        int64_t ms_6 = get_micro_seconds();
        if (ms_00) {
            int64_t fps = 1000000/(ms_6 - ms_00);
            if (fps < FPS_DIST_MAX) {
                ++fps_distribution[fps];
            } else {
                ++fps_distribution[FPS_DIST_MAX];
            }
            if (fps < app_ctx->refresh_rate - 1) {
                ++low_fps_count;
            }
            ++fps_sample_counter;
            if ( fps_sample_counter >= FPS_SAMPLE_COUNT ) {
                if (ms_6 > fsp1.micros) {
                    app_ctx->workspace.reported_fps = (render_iters - fsp1.render_count)*1000000/(ms_6-fsp1.micros);
                }
                fps_sample_counter = 0;
                fsp1.render_count = render_iters;
                fsp1.micros = ms_6;
            }
        }
        ms_00 = ms_6;
        ++render_iters;
//        if (app_ctx->vsync) {
//            sleep_micro_seconds(1000);
//        }
    }
    input_loop = false;
    app_printf("*** render loop done ****\n");
    SDL_WaitThread(app_ctx->input_thread, NULL);
    printf("low_fps_count=%u/%ld %f\n", low_fps_count, (long)render_iters, (float)low_fps_count*100/render_iters);
    for(int i =0; i < sizeof(fps_distribution)/sizeof(fps_distribution[0]); ++i) {
        if (fps_distribution[i]) {
            printf("    %d -> %d\n", i, fps_distribution[i]);
        }
    }
}

int app_render(app_context* app_ctx,  SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, const SDL_Point *center, const SDL_RendererFlip flip) {
    return SDL_RenderCopyEx(app_ctx->renderer, texture, srcrect, dstrect, 0, center, flip);
}

int app_render_rotated(app_context* app_ctx,  SDL_Texture * texture, const SDL_Rect * srcrect, const SDL_Rect * dstrect, const SDL_Point *center, const SDL_RendererFlip flip, double angle) {
    return SDL_RenderCopyEx(app_ctx->renderer, texture, srcrect, dstrect, angle, center, flip);
}


//void app_input_loop(app_context* app_ctx) {
//}

void ___app_input_loop(app_context* app_ctx) {
    while(__atomic_load_n(&render_ready, __ATOMIC_ACQUIRE) == 0) {
        sleep_milli_seconds(100);
    }
    app_printf("+++ input start\n"); fflush(stdout);
    __atomic_store_n(&input_ready, true, __ATOMIC_RELEASE);

    bool ignore_SDL_FINGER = 0 == start_touch_screen_event_generator(NULL);
    
    while(input_loop) {
        SDL_Event event;
        while(evt_q_get(&event)) {
            switch(event.type) {
                case SDL_QUIT:
                    app_stop(app_ctx);
                    break;
                case SDL_FINGERMOTION:
                case SDL_FINGERDOWN:
                case SDL_FINGERUP:
                    if (!ignore_SDL_FINGER) {
                        app_ctx->cb_input(app_ctx, &event);
                    }
                    break;
                default:
                    app_ctx->cb_input(app_ctx, &event);
                    break;
            }
        }
        sleep_milli_seconds(app_ctx->input_loop_sleep_millis);
    }
}

void app_stop(app_context_ptr app_ctx) {
    render_loop = false;
    input_loop = false;
}

bool app_running(app_context_ptr app_ctx) {
    return render_loop;
}

void app_wait_ready() {
    while(__atomic_load_n(&render_ready, __ATOMIC_ACQUIRE) == 0) {
        sleep_milli_seconds(100);
    }
    while(__atomic_load_n(&input_ready, __ATOMIC_ACQUIRE) == 0) {
        sleep_milli_seconds(100);
    }
}

int64_t app_get_render_count() {
    return render_iters;
}
