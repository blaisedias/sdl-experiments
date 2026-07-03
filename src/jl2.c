#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL2/SDL_render.h>
#include <SDL2/SDL_ttf.h>
//#include <GL/glew.h>
//#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include "util.h"
#include "logging.h"
#include "timing.h"
#include "application.h"
#include "texture_cache.h"
#include "sdl_userevents.h"
#include "lyrion_player.h"
#include "widgets.h"
#include "widgets_json.h"
#include "vumeter_util.h"
#include "nowplaying.h"

lyrion_player_ptr get_player();

#define WINDOW_TITLE "Tsp"
#define HIDE_CURSOR_COUNT 50
static int show_cursor = 0;
static const char* help_text=""
"\n"
" - [help, -h, --h] : print this text and exit\n"
"\n"  
" - novsync : do not use vertical sync when rendering each frame\n"
" - max_secs <count> : time to run before terminating, infinite if not specified\n"
" - cycle <count> : number of seconds before cycling to the next the VU Meter\n"
" - [0.0, 90.0, 180.0, 270.0] : rotation. Default is 0.0\n"
"\n"  
" - printfdefbug enable printing of debug\n"
" - printfinput  enable printing of input data\n"
" - printfvol    enable printing of volume levels\n"
" - printfload   enable printing of media load times\n"
" - printfscale  enable printing of scaling parameters and data\n"
" - printfperf   enable printing of performance metrics\n"
" - profile      enable printing of render loop performance metrics (per frame)\n"
" - profile_fps_deviation      enable printing of render loop performance metrics (per frame) when fps has deviated\n"
" - profile_texture   enable printing of render loop texture metrics\n"
" - printfjson   enable printing of json processing\n"
" - printfaction enable printing of actions\n"
" - printftcache enable printing of texture cache module\n"
" - printftcacheeject enable printing of texture cache module ejects\n"
" - printfapp enable printing of application processing\n"
" - debug_redraw_backdrop enable printing when backdrop is redrawn\n"
"\n"  
" - list list the set of VU Meters and exit\n"
" - dl <path-to-object-file> : dynamically load VU meter in object file\n"
"\n"  
" - wxh <width>[x]<height> : window width and height, only works if window manager is available\n"
" - fullscreen\n"
"\n"
" - json path to json file\n"
"\n"
" - vu <vumeter_name> first VU meter to display\n"
"\n"
" - showrects       : show widget draw rectangles when pointer is over them\n"
" - showinputrects  : show widget input rectangles when pointer is over them\n"
" - debugrects      : show widget draw_rectangles\n"
"\n"
" - peakhold <count>: number of frames for VU peak hold\n"
" - decayhold <count>: number of frames for VU decay hold - reduces needle jitter\n"
"\n"
" - texture_cache_size <count>: maximum number of texture bytes\n"
"\n"
" - lms <name>: lyrion media server network name or ip address \n"
"\n"
" - monitor-tcache: print texture cache memory usage at regular intervals \n"
"\n";  

static char *json_files = "npvu.json,npvularge.json";
static bool dump_vu = false;
static bool monitor_tcache = false;

#define MAX_NP_VIEWS   10
static volatile view_context_ptr current_view = NULL;
static view_context_ptr main_view = NULL;
static view_context_ptr np_views[MAX_NP_VIEWS];
static volatile int np_view_indx=0;
static volatile bool refresh_widget_contents = false;
// TODO make value configurable, if true, visualiser change actions
// are propagated to all views
static volatile bool propagate_visualiser_change = true;
static SDL_sem* controller_sem;

static void my_render_backdrop(app_context_ptr app_ctx);
static void my_render_foreground(app_context_ptr app_ctx);
static bool my_query_render_backdrop(app_context_ptr app_ctx);
static void my_event_handler(app_context_ptr app_ctx, SDL_Event* eventp);
static void player_poll_loop(app_context_ptr app_ctx);


static app_context app_ctx = {
//        .window = NULL,
//        .renderer = NULL,
        .screen_width = 800,
        .screen_height = 480,
//        .orientation = 0,
        .vsync = true,
        .input_loop_sleep_millis = 100,
        .cb_query_render_backdrop = my_query_render_backdrop,
        .cb_render_backdrop = my_render_backdrop,
        .cb_render_foreground = my_render_foreground,
        .cb_input = my_event_handler,
    };

static inline void free_ex(void** tgt) {
    if (*tgt) {
        free(*tgt);
    }
    *tgt = NULL;
}
#define FREE(x) free_ex((void **)(&x))

static void controller(app_context_ptr app_ctx);

static void invalid_args(const char* opt) {
    puts(help_text);
    printf("Invalid number of arguments for command line option: %s\n", opt);
    exit(EXIT_FAILURE);
}

static void set_current_view(view_context_ptr new_view) {
    __atomic_store_n(&current_view, new_view, __ATOMIC_RELEASE);
}

static view_context_ptr get_current_view() {
    return __atomic_load_n(&current_view, __ATOMIC_ACQUIRE);
}

int main(int argc, char** argv) {

    for(int i = 1; i < argc; ++i) {
        if (0 == strcmp(argv[i], "max_secs")) {
            if (argc > i+1) {
                app_ctx.max_secs = atoi(argv[i+1]);
                i += 1;
            } else { invalid_args(argv[i]); }
        } else if (0 == strcmp(argv[i], "cycle")) {
            if (argc > i+1) {
                app_ctx.cycle_secs = atoi(argv[i+1]);
                i += 1;
            } else { invalid_args(argv[i]); }
        } else if (0 == strcmp(argv[i], "wxh")) {
            if (argc > i+1) {
                char *hvalue = strchr(argv[i+1], 'x');
                if (hvalue) {
                    app_ctx.screen_width = atoi(argv[i+1]);
                    app_ctx.screen_height = atoi(hvalue+1);
                    i += 1;
                }
                else if (argc > i+2) {
                    app_ctx.screen_width = atoi(argv[i+1]);
                    app_ctx.screen_height = atoi(argv[i+2]);
                    i += 2;
                } else { invalid_args(argv[i]); }
            } else { invalid_args(argv[i]); }
        } else if (0 == strcmp(argv[i], "novsync")) {
            app_ctx.vsync = false;
        } else if (0 == strcmp(argv[i], "0.0")) {
            app_ctx.orientation = 0.0;
        } else if (0 == strcmp(argv[i], "180.0")) {
            app_ctx.orientation = 180.0;
        } else if (0 == strcmp(argv[i], "90.0")) {
            app_ctx.orientation = 90.0;
        } else if (0 == strcmp(argv[i], "270.0")) {
            app_ctx.orientation = 270.0;
        } else if (0 == strcmp(argv[i], "printfdebug")) {
            enable_printf(DEBUG_PRINTF);
        } else if (0 == strcmp(argv[i], "printfinput")) {
            enable_printf(INPUT_PRINTF);
        } else if (0 == strcmp(argv[i], "printfvol")) {
            enable_printf(VOL_PRINTF);
        } else if (0 == strcmp(argv[i], "printfload")) {
            enable_printf(LOAD_PRINTF);
        } else if (0 == strcmp(argv[i], "printfscale")) {
            enable_printf(SCALE_PRINTF);
        } else if (0 == strcmp(argv[i], "printfperf")) {
            enable_printf(PERF_PRINTF);
        } else if (0 == strcmp(argv[i], "profile")) {
            enable_printf(PROFILE_PERF_PRINTF);
        } else if (0 == strcmp(argv[i], "profile_fps_deviation")) {
            enable_printf(PROFILE_PERF_PRINTF);
            app_ctx.profile_fps_deviation = true;
         } else if (0 == strcmp(argv[i], "profile_texture")) {
            enable_printf(PROFILE_TEXTURE_PERF_PRINTF);
         } else if (0 == strcmp(argv[i], "printfjson")) {
            enable_printf(JSON_PRINTF);
        } else if (0 == strcmp(argv[i], "printfaction")) {
            enable_printf(ACTION_PRINTF);
        } else if (0 == strcmp(argv[i], "printftcache")) {
            enable_printf(TEXTURE_CACHE_PRINTF);
        } else if (0 == strcmp(argv[i], "printftcacheeject")) {
            enable_printf(TEXTURE_CACHE_EJECT_PRINTF);
        } else if (0 == strcmp(argv[i], "printfapp")) {
            enable_printf(APP_PRINTF);
        } else if (0 == strcmp(argv[i], "debug_redraw_backdrop")) {
            app_ctx.debug_redraw_backdrop = true;
        } else if (0 == strcmp(argv[i], "fs") || 0 == strcmp(argv[i], "fullscreen")) {
            app_ctx.fullscreen = true;
        } else if (0 == strcmp(argv[i], "texture_cache_size")) {
            if (argc > i+1) {
                tcache_set_limit(atoi(argv[i+1]));
                i += 1;
            } else { invalid_args(argv[i]); }
        } else if (0 == strcmp(argv[i], "lms")) {
            if (argc > i+1) {
                app_ctx.lms = strdup(argv[i+1]);
                i += 1;
            } else { invalid_args(argv[i]); } 
            
        } else if (0 == strcmp(argv[i], "dumpvu")) {
            dump_vu = true;
        } else if (0 == strcmp(argv[i], "texture_cache_size")) {
            if (argc > i+1) {
                tcache_set_limit(atoi(argv[i+1]));
                i += 1;
            }
        } else if (0 == strcmp(argv[i], "lms")) {
            if (argc > i+1) {
                app_ctx.lms = strdup(argv[i+1]);
                i += 1;
            } 
        } else if (0 == strcmp(argv[i], "debugrects")) {
            debug_rects = true;
        } else if (0 == strcmp(argv[i], "showrects")) {
            show_rects = true;
        } else if (0 == strcmp(argv[i], "showinputrects")) {
            show_input_rects = true;
        } else if (0 == strcmp(argv[i], "monitor-tcache")) {
            monitor_tcache = true;
        } else if (0 == strcmp(argv[i], "profile_level")) {
            if (argc > i+1) {
                VUMeter_set_profile_level(atoi(argv[i+1]));
                i += 1;
            }
/*            
         } else if (0 == strcmp(argv[i], "vu")) {
            if (argc > i+1) {
                i += 1;
                app_ctx.first_vu_meter = argv[i];
            }
*/            
        } else if (0 == strcmp(argv[i], "list") ){
            const vumeter_properties_t *p = VUMeter_get_props_list();
            while(p != NULL) {
                for(int iv=0; iv < p->vumeter_count;  ++iv) {
                    printf("%s\n", p->vumeters[iv].name);
                }
                p = p->next;
            }
            exit(EXIT_SUCCESS);
        } else if (0 == strcmp(argv[i], "dl")) {
            if (argc > i+1) {
                VUMeter_loadlib(argv[i+1]);
                i += 1;
            }
        } else if (0 == strcmp(argv[i], "json")) {
            if (argc > i+1) {
                json_files = argv[i+1];
                i += 1;
            }
        } else if (0 == strcmp(argv[i], "peakhold")) {
            if (argc > i+1) {
                VUMeter_set_peak_hold(atoi(argv[i+1]));
                i += 1;
            }
        } else if (0 == strcmp(argv[i], "decayhold")) {
            if (argc > i+1) {
                VUMeter_set_decay_hold(atoi(argv[i+1]));
                i += 1;
            }
        } else if (0 == strcmp(argv[i], "help")
                || (argv[i][0] == '-' && (argv[i][1] == 'h' || argv[i][1] == '?'))
                || (argv[i][0] == '-' && argv[i][1] == '-' && argv[i][2] == 'h')
                ) {
            puts(help_text);
            exit(EXIT_SUCCESS);
        } else {
            puts(help_text);
            error_printf("Unknown command line option %d) %s\n", i, argv[i]);
            exit(EXIT_FAILURE);
        }
    }

    app_printf("screen= %dx%d orientation=%f\n", app_ctx.screen_width, app_ctx.screen_height, app_ctx.orientation);

    controller_sem = SDL_CreateSemaphore(0);
    SDL_Thread* main_thread = SDL_CreateThread((SDL_ThreadFunction)controller, "controller", &app_ctx);
    SDL_Thread* player_thread = SDL_CreateThread((SDL_ThreadFunction)player_poll_loop, "player", &app_ctx);
//    SDL_Thread* input_thread = SDL_CreateThread((SDL_ThreadFunction)app_input_loop, "input", &app_ctx);

    if (app_initialize(&app_ctx, WINDOW_TITLE)) {
        app_cleanup(&app_ctx, EXIT_FAILURE);
    }

    app_render_loop(&app_ctx);

//    app_printf("Waiting for input thread\n");
//    SDL_WaitThread(input_thread, NULL);
    app_printf("Waiting for player thread\n");
    SDL_WaitThread(player_thread, NULL);

    SDL_SemPost(controller_sem);
    app_printf("Waiting for controller thread\n");
    SDL_WaitThread(main_thread, NULL);

    app_cleanup(&app_ctx, EXIT_SUCCESS);

    return 0;
}

void next_np_view() {
    view_context_ptr view = get_current_view();
    if (view != main_view) {
        for (int ix=1; ix < MAX_NP_VIEWS; ++ix) {
            int indx = (np_view_indx + ix) % MAX_NP_VIEWS;
            if (np_views[indx]) {
                set_current_view(np_views[indx]);
                np_view_indx = indx;
                refresh_widget_contents = true;
                return;
            }
        }
    }
}

void prev_np_view() {
    view_context_ptr view = get_current_view();
    if (view != main_view) {
        for (int ix=1; ix < MAX_NP_VIEWS; ++ix) {
            int indx = (np_view_indx - ix);
            if (indx < 0) {
                indx += MAX_NP_VIEWS;
            }
            if (np_views[indx]) {
                set_current_view(np_views[indx]);
                np_view_indx = indx;
                refresh_widget_contents = true;
                return;
            }
        }
    }
}

void select_np_view() {
    view_context_ptr view = get_current_view();
    if (view == main_view) {
        if (np_views[np_view_indx]) {
            set_current_view(np_views[np_view_indx]);
            refresh_widget_contents = true;
        } else {
            next_np_view();
        }
    }
}

void select_main_view() {
    view_context_ptr view = get_current_view();
    if (view != main_view) {
        set_current_view(main_view);
        refresh_widget_contents = true;
    }
}


static view_context_ptr load_json_view(const char* json_path, app_context_ptr app_ctx) {
    view_context_ptr vw = calloc(sizeof(*vw),1);
    if(NULL == vw) {
        return vw;
    }
    vw->app = app_ctx;
    vw->list = create_widget_list(vw);
    if (0 != deserialise_widgets_file(json_path, vw)) {
        error_printf("failed to deserialise widgets from file %s\n", json_path);
        destroy_widget_list(vw->list);
        FREE(vw);
    } else {
        widget_list_load_media(vw->list, "./images");
        for(widget_t* t = widget_list_tail(vw->list); t != NULL; t = widget_list_prev(vw->list, t)) {
            const char* player_value_key = widget_get_player_value_key(t);
            const char* runtime_value_key = widget_get_runtime_value_key(t);
            if ((player_value_key && 0 == strcmp("time", player_value_key))
               || 
               (runtime_value_key && 0 == strcmp("fps", runtime_value_key))
               ||
               widget_get_hotspot(t)) {
                widget_set_renderhf(t);
                log_printf("widget_set_renderhf %d\n", widget_get_type_name(t));
            }
        }
    }
    return vw;
}

static void controller(app_context_ptr app_ctx) {
    app_wait_ready();
//debug    
printf("starting controller\n"); fflush(stdout);
    SDL_ShowCursor(SDL_DISABLE);
    int64_t endtime = app_ctx->max_secs * 1000;
    if (endtime) {
        endtime += get_milli_seconds();
    }
    view_context_ptr vw = load_json_view("main.json", app_ctx);
    if(NULL == vw) {
        exit(EXIT_FAILURE);
    }
    main_view = vw;

    if (VUMeter_get_props_list() == NULL) {
        error_printf("No VU Meters found\n");
//        app_stop(app_ctx);
    }

    if (json_files && strlen(json_files)) {
        char *tmp = strdup(json_files);
        char *json_file = tmp;
        char *comma_p = NULL;
        do {
            comma_p = strchr(json_file, ',');
            if (comma_p) {
                *comma_p = 0;
            }
            vw = load_json_view(json_file, app_ctx);
            if(vw) {
               np_views[np_view_indx] = vw;
                ++np_view_indx;
            }
            if (comma_p) {
                json_file = comma_p+1;
            }
        }while(comma_p);
        free(tmp);
    }

    if (np_view_indx > 1) {
        app_set_multiple_views(app_ctx, np_view_indx > 1);
    } else {
        for(int ix =0; ix < MAX_NP_VIEWS && propagate_visualiser_change; ++ix) {
            view_context_ptr pv = np_views[ix];
            if (pv) {
                for(widget_t* t = widget_list_tail(pv->list); t != NULL; t = widget_list_prev(pv->list, t)) {
                    if (widget_has_action(t, ACTION_NEXT_NP_VIEW)
                           ||
                           widget_has_action(t, ACTION_PREV_NP_VIEW)) {
                        widget_hide(t, true);
                    }
                }
            }
        }
    }
    np_view_indx = 0;
    set_current_view(np_views[np_view_indx]);
//    set_current_view(main_view);

    //TODO select view when previously shutdown
    
    int64_t next_vu_time = app_ctx->cycle_secs * 1000;
    if (next_vu_time) {
        next_vu_time += get_milli_seconds();
    }
//    size_t num_texture_bytes = 0;
//    size_t num_surface_bytes = 0;
    unsigned iters = 0;
    while(app_running(app_ctx)) {
        ++iters;
        sleep_milli_seconds(100);
        if (show_cursor) {
            if (0 >= --show_cursor) {
                SDL_ShowCursor(SDL_DISABLE);
                show_cursor = 0;
                view_context_ptr view = get_current_view();
                if (view) {
                    for(widget_t* widget=widget_list_tail(view->list); widget != NULL; widget=widget_list_prev(view->list, widget)) {
                        if (widget_is_hidden(widget)) { continue;}
                        widget_set_focussed(widget, false);
                        widget_set_highlight(widget, false);
                    }
                }            
            }
        }
        if (endtime) {
            if (get_milli_seconds() > endtime) {
                app_stop(app_ctx);
            }
        }
        if (next_vu_time && get_milli_seconds() > next_vu_time) {
            SDL_Event next_visu_event = {.type = USEREVENT_NEXT_VISU };
            SDL_PushEvent(&next_visu_event);
            next_vu_time = app_ctx->cycle_secs * 1000 + get_milli_seconds();
        }
        size_t nt = tcache_get_texture_bytes_count();
        size_t ns = tcache_get_surface_bytes_count();
//        if (nt != num_texture_bytes || ns != num_surface_bytes)
        if (monitor_tcache && 0 == (iters%50)) {
//            printf("+++ t=%09lu %.02f s=%09lu %.02f (delta t=%ld s=%ld)\n",
//                    nt, (float)nt/(1024*1024),
//                    ns, (float)ns/(1024*1024),
//                    (long)nt-(long)num_texture_bytes, (long)ns-(long)num_surface_bytes);
            log_printf("textures:%.02f MiB surfaces:=%.02f MiB\n",
                    (float)nt/(1024*1024),
                    (float)ns/(1024*1024));
//            num_texture_bytes = nt;
//            num_surface_bytes = ns;
        }
    }
    SDL_SemWait(controller_sem);
    set_current_view(NULL);
    for(int ix = 0; ix < MAX_NP_VIEWS; ++ix) {
        if(np_views[ix]) {
            destroy_widget_list(np_views[ix]->list);
            free(np_views[ix]);
            np_views[ix] = NULL;
        }
    }
    if(main_view) {
        destroy_widget_list(main_view->list);
        free(main_view);
    }
}

static bool my_query_render_backdrop(app_context_ptr app_ctx) {
    view_context_ptr view = get_current_view();
    static view_context_ptr previous_view = NULL;
    if (view) {
        if (previous_view != view) {
            previous_view = view;
            return true;
        }
        return widget_list_query_render_backdrop(view->list);
    }
    return false;
}

static void my_render_backdrop(app_context_ptr app_ctx) {
    view_context_ptr view = get_current_view();
    if (view) {
        widget_list_render_backdrop(view->list);
    }
}

static void my_render_foreground(app_context_ptr app_ctx) {
    view_context_ptr view = get_current_view();
    if (view) {
        widget_list_render_foreground(view->list);
    }
}

static void my_event_handler(app_context_ptr app_ctx, SDL_Event* eventp) {
    static  SDL_Scancode prev_keydown;
    static int64_t keydown_start_time = 0;
    int key_press_duration = 0;
    switch (eventp->type) {
            case USEREVENT_NEXT_VISU:
            case USEREVENT_NEXT_VU:
                {
                    view_context_ptr view = get_current_view();
                    if(view) {
                        widget_list_react(view->list, USEREVENT_NEXT_VISU ? NEXT_VISU: NEXT_VU, NULL);
                    }
                    for(int ix =0; ix < MAX_NP_VIEWS && propagate_visualiser_change; ++ix) {
                        view_context_ptr pv = np_views[ix];
                        if (pv && pv != view) {
                            widget_list_react(pv->list, USEREVENT_NEXT_VISU ? NEXT_VISU: NEXT_VU, NULL);
                        }
                    }
                }break;
            case USEREVENT_PREV_VISU:
            case USEREVENT_PREV_VU:
                {
                    view_context_ptr view = get_current_view();
                    if (view) {
                        widget_list_react(view->list, USEREVENT_PREV_VISU ? PREV_VISU: PREV_VU, NULL);
                    }
                    for(int ix =0; ix < MAX_NP_VIEWS && propagate_visualiser_change; ++ix) {
                        view_context_ptr pv = np_views[ix];
                        if (pv && pv != view) {
                            widget_list_react(pv->list, USEREVENT_PREV_VISU ? PREV_VISU: PREV_VU, NULL);
                        }
                    }
                }break;
            case SDL_QUIT:
                puts("");
                app_stop(app_ctx);
                break;
            case SDL_KEYDOWN:
                if (eventp->key.keysym.scancode != prev_keydown) {
                    prev_keydown = eventp->key.keysym.scancode;
                    keydown_start_time = get_milli_seconds();
                }
                break;
            case SDL_KEYUP:
                print_sdl_key_scancode(eventp->key.keysym.scancode);
                {
                    if (eventp->key.keysym.scancode == prev_keydown) {
                        key_press_duration = get_milli_seconds() - keydown_start_time;
                        prev_keydown = SDL_SCANCODE_UNKNOWN;
                    }
                }
                app_printf("key press duration = %d ms\n", key_press_duration);
                switch (eventp->key.keysym.scancode) {
                case SDL_SCANCODE_ESCAPE: 
                    {
                        if (get_current_view() == main_view) {
                            puts("");
                            app_stop(app_ctx);
                        } else {
                            select_main_view();
                        }
                    }
                    break;
                case SDL_SCANCODE_LEFTBRACKET:
                    select_np_view();
                    break;
                case SDL_SCANCODE_SPACE:
                    dispatch_action(ACTION_PLAY_PAUSE);
                    break;
                case SDL_SCANCODE_TAB:
                    {
                        unsigned texture_bytes = tcache_get_texture_bytes_count();
                        unsigned surface_bytes = tcache_get_surface_bytes_count();
                        printf("\n texture:%u %fMiB surface:%u %fMib\n", texture_bytes, (float)texture_bytes/(1024*1024), surface_bytes, (float)surface_bytes/(1024*1024));
                    }
                    break;
                case SDL_SCANCODE_PRINTSCREEN:
                    tcache_dump();
                    break;
                case SDL_SCANCODE_LEFT:
                case SDL_SCANCODE_KP_4:
                    dispatch_action(ACTION_PREV_VISU);
                    break;
                case SDL_SCANCODE_UP:
                    dispatch_action(ACTION_NEXT_NP_VIEW);
                    break;
                case SDL_SCANCODE_DOWN:
                    dispatch_action(ACTION_PREV_NP_VIEW);
                    break;
                case SDL_SCANCODE_RIGHT:
                case SDL_SCANCODE_KP_6:
                    dispatch_action(ACTION_NEXT_VISU);
                    break;
                case SDL_SCANCODE_AUDIOPLAY:
                    dispatch_action(ACTION_PLAY_PAUSE);
                    break;
                case SDL_SCANCODE_AUDIOSTOP:
                    dispatch_action(ACTION_STOP);
                    break;
                case SDL_SCANCODE_AUDIOPREV:
                    dispatch_action(ACTION_PREV_TRACK);
                    break;
                case SDL_SCANCODE_AUDIONEXT:
                    dispatch_action(ACTION_NEXT_TRACK);
                    break;
                case SDL_SCANCODE_SCROLLLOCK:
                case SDL_SCANCODE_F9:
                    dispatch_action(ACTION_LOCK_VISU);
                    break;
                case SDL_SCANCODE_F10:
                    dispatch_action(ACTION_UNLOCK_VISU);
                    break;
                case SDL_SCANCODE_AUDIOREWIND:
                    error_printf("rewind is not supported\n");
                    break;
                case SDL_SCANCODE_AUDIOFASTFORWARD:
                    error_printf("fastforward is not supported\n");
                    break;
                case SDL_SCANCODE_VOLUMEUP:
                    player_volume_inc(get_player());
                    break;
                case SDL_SCANCODE_VOLUMEDOWN:
                    player_volume_dec(get_player());
                    break;
                case SDL_SCANCODE_MUTE:
                    // TODO
                    puts("TODO volume mute\n"); fflush(stdout);
                    break;
                default:
                    break;
                }
                break;
            case SDL_MOUSEMOTION:
                {
                    SDL_ShowCursor(SDL_ENABLE);
                    show_cursor = HIDE_CURSOR_COUNT;
                    SDL_Point pt = {.x=eventp->button.x, .y=eventp->button.y};
                    view_context_ptr view = get_current_view();
                    if (view) { widget_list_react(view->list, POINTER_MOTION, &pt); }
                } break;
            case SDL_MOUSEBUTTONDOWN:
                {
                    SDL_ShowCursor(SDL_ENABLE);
                    show_cursor = HIDE_CURSOR_COUNT;
                    SDL_Point pt = {.x=eventp->button.x, .y=eventp->button.y};
                    view_context_ptr view = get_current_view();
                    if (view) { widget_list_react(view->list, POINTER_DOWN, &pt); }
                } break;
            case SDL_MOUSEBUTTONUP:
                {
                    SDL_ShowCursor(SDL_ENABLE);
                    show_cursor = HIDE_CURSOR_COUNT;
                    SDL_Point pt = {.x=eventp->button.x, .y=eventp->button.y};
                    view_context_ptr view = get_current_view();
                    widget_list_react(view->list, POINTER_UP, &pt);
                } break;
            case SDL_FINGERMOTION:
//                if (ignore_SDL_FINGER) {
//                    input_printf("IGNORING SFMO: %04d, %04d\n",(int)(eventp->tfinger.x*app_ctx->screen_width), (int)(eventp->tfinger.y*app_ctx->screen_height));
//                } else {
                {
                    SDL_Point pt = { 
                        .x = (int)(eventp->tfinger.x*app_ctx->screen_width),
                        .y = (int)(eventp->tfinger.y*app_ctx->screen_height)
                    };
                    view_context_ptr view = get_current_view();
                    if (view) { widget_list_react(view->list, POINTER_MOTION, &pt); }
                }
                break;
            case USEREVENT_FINGERMOTION:
                {
                    SDL_Point pt = { .x = eventp->motion.x, .y = eventp->motion.y };
                    view_context_ptr view = get_current_view();
                    if (view) { widget_list_react(view->list, POINTER_MOTION, &pt); }
                } break;
            case SDL_FINGERDOWN:
//                if (ignore_SDL_FINGER) {
//                    input_printf("IGNORING SFDN: %04d, %04d\n", (int)(eventp->tfinger.x*app_ctx->screen_width), (int)(eventp->tfinger.y*app_ctx->screen_height));
//                } else {
                {
                    SDL_Point pt = { 
                        .x = (int)(eventp->tfinger.x*app_ctx->screen_width),
                        .y = (int)(eventp->tfinger.y*app_ctx->screen_height)
                    };
                    view_context_ptr view = get_current_view();
                    if (view) { widget_list_react(view->list, POINTER_DOWN, &pt); }
                }
                break;
            case USEREVENT_FINGERDOWN:
                {
                    SDL_Point pt = { .x = eventp->motion.x, .y = eventp->motion.y };
                    view_context_ptr view = get_current_view();
                    if (view) { widget_list_react(view->list, POINTER_DOWN, &pt); }
                } break;
            case SDL_FINGERUP:
//                if (ignore_SDL_FINGER) {
//                    input_printf("IGNORING SFUP: %04d, %04d\n", (int)(eventp->tfinger.x*app_ctx->screen_width), (int)(eventp->tfinger.y*app_ctx->screen_height));
//                } else {
                {
                    SDL_Point pt = { 
                        .x = (int)(eventp->tfinger.x*app_ctx->screen_width),
                        .y = (int)(eventp->tfinger.y*app_ctx->screen_height)
                    };
                    view_context_ptr view = get_current_view();
                    if (view) { widget_list_react(view->list, POINTER_UP, &pt); }
                }
                break;
            case USEREVENT_FINGERUP:
                {
                    SDL_Point pt = { .x = eventp->motion.x, .y = eventp->motion.y };
                    view_context_ptr view = get_current_view();
                    if (view) { widget_list_react(view->list, POINTER_UP, &pt); }
                } break;
        
        default:
            break;
    }
}

static lyrion_player_ptr    player = NULL;
lyrion_player_ptr get_player() {
    return player;
}

static void player_poll_loop(app_context_ptr app_ctx) {
    player_mode_t    player_mode = PLAYER_MODE_UNDEFINED;
//    int64_t          player_mode_start_timestamp = 0;

    player_transient_state pts;
    // workspace is non const
    char buffer[512];
    uint64_t sig=0;

    app_wait_ready();

    player = open_local_player(app_ctx->lms);
    //TODO: configure volume step
    player_set_volume_step(player, 3);

//debug
printf("starting player_poll_loop\n"); fflush(stdout);
    unsigned fps = 0;

    while(app_running(app_ctx)) {
        sleep_milli_seconds(500);
        // ensure that the player is initialised - if possible, nop if the player is initialised
        if (poll_player(player, &pts) || refresh_widget_contents) {
            refresh_widget_contents = false;
//            puts("P poll data");
            bool can_seek = true;
            {
                player_value pvalue;
                switch(get_player_value(player, &pvalue, "CAN_SEEK")) {
                        case PFV_NONE:
                            error_printf("got nothing for player value CAN_SEEK\n");
                            break;
                        case PFV_INT:
                            debug_printf("got int %d for player CAN_SEEK\n", pvalue.integer);
                            can_seek = pvalue.integer;
                            break;
                        case PFV_STRINGPTR:
                            error_printf("got string %s for player value CAN_SEEK\n", pvalue.strptr);
                            FREE(pvalue.strptr);
                            break;
                }
            }

            view_context_ptr view = get_current_view();
            if (view) {
            for(widget_t* t = widget_list_tail(view->list); t != NULL; t = widget_list_prev(view->list, t)) {
                const char* player_value_key = widget_get_player_value_key(t);
                const char* player_range_value_key = widget_get_player_range_value_key(t);
                widget_type_t wtype = widget_get_type(t);
                if (player_range_value_key) {
                    player_value pvalue;
                    switch(get_player_value(player, &pvalue, player_range_value_key)) {
                        case PFV_NONE:
                            error_printf("got nothing for player range value %s\n", player_range_value_key);
                            break;
                        case PFV_INT:
                            debug_printf("got int %d for player range value %s\n", pvalue.integer, player_range_value_key);
                            if (wtype == WIDGET_SLIDER) {
                                widget_slider_range(t, 0, pvalue.integer);
                            }
                            break;
                        case PFV_STRINGPTR:
                            error_printf("got string %s for player range value %s\n", pvalue.strptr, player_range_value_key);
                            FREE(pvalue.strptr);
                            break;
                    }
                }
                if (*player_value_key) {
                    player_value pvalue;
                    switch(get_player_value(player, &pvalue, player_value_key)) {
                        case PFV_NONE:
                            error_printf("got nothing for player value %s\n", player_value_key);
                            break;
                        case PFV_INT:
                            debug_printf("got int %d for player value %s\n", pvalue.integer, player_value_key);
                            if (wtype == WIDGET_MULTISTATE_BUTTON) {
                                widget_multistate_button_set_state(t, pvalue.integer);
                            } else if (wtype == WIDGET_SLIDER) {
                                if (strcmp("time", player_value_key)) {
                                    widget_slider_update_value(t, pvalue.integer);
                                }
                            }
                            break;
                        case PFV_STRINGPTR:
                            error_printf("got string %s for player value %s\n", pvalue.strptr, player_value_key);
                            FREE(pvalue.strptr);
                            break;
                    }
                }
                if (wtype == WIDGET_SLIDER && player_value_key && 0 == strcmp(player_value_key, "time")) {
                    widget_slider_set_interactive(t, can_seek);
                }
                if (wtype == WIDGET_TEXT) {
                    const char* fmt = widget_text_get_format(t);
                    if (*fmt) {
                        player_sprintf(player, buffer, sizeof(buffer), fmt);
                        debug_printf("'%s' -> '%s'\n", fmt, buffer);
                        widget_text_set_content(t, buffer);
                    }
                }
            }
            }
            player_sprintf(player, buffer, sizeof(buffer), "{playlist_cur_index}{TITLE}{ARTIST}{ALBUM_OR_REMOTE_TITLE}");
            uint64_t new_sig = compute_player_hash(buffer);
            player_sprintf(player, buffer, sizeof(buffer), "Title:{TITLE} Artist:{ARTIST} Album:{ALBUM_OR_REMOTE_TITLE}");
//            fprintf(stderr, "\n<< 0x%lx==0x%lx %s >>\n",(unsigned long)sig, (unsigned long)new_sig, buffer);
            if (sig && new_sig != sig) {
                // TODO only change visualiser if user setting is set
                SDL_Event next_visu_event = {.type = USEREVENT_NEXT_VISU };
                SDL_PushEvent(&next_visu_event);
            }
            sig = new_sig;
            player_value pv;
            if (PFV_INT == get_player_value(player, &pv, "MODE")) {
                if (player_mode != pv.integer) {
                    player_mode = pv.integer;
//                    player_mode_start_timestamp = get_milli_seconds();
                }
            }
        }
        player_value pvalue;
        get_player_value(player, &pvalue, "CAN_CHANGE_VOLUME");
        bool can_change_volume = pvalue.integer;
        get_player_value(player, &pvalue, "VOLUME");
        int volume = pvalue.integer;
        get_player_value(player, &pvalue, "DURATION");
        int duration = pvalue.integer;
        get_player_value(player, &pvalue, "time");
        int elapsed = pvalue.integer;
//        fprintf(stderr, "%d %d:%02d/%d:%02d %d:%02d fps:%u                          \r", volume,
//                elapsed/60, elapsed%60,
//                duration/60, duration%60,
//                (elapsed-duration)/60, abs((elapsed-duration)%60),
//                app_ctx->workspace.reported_fps
//                );
//        fflush(stderr);
        if (fps != app_ctx->workspace.reported_fps) {
            profile_printf("fps:%u\n", app_ctx->workspace.reported_fps);
            fps = app_ctx->workspace.reported_fps;
        }
        view_context_ptr view = get_current_view();
        if (view) {
        for(widget_t* t = widget_list_tail(view->list); t != NULL; t = widget_list_prev(view->list, t)) {
            const char* player_value_key = widget_get_player_value_key(t);
            const char* runtime_value_key = widget_get_runtime_value_key(t);
            widget_type_t wtype = widget_get_type(t);
            if (player_value_key) {
                if (0 == strcmp("VOLUME", player_value_key)) {
                    if (wtype == WIDGET_SLIDER) {
                        widget_slider_update_value(t, volume);
                        widget_slider_set_interactive(t, can_change_volume);
                    }
                }
                if (duration && 0 == strcmp("time", player_value_key)) {
                    if (wtype == WIDGET_SLIDER) {
                        widget_slider_update_value(t, elapsed);
                    }
                }
                if (wtype == WIDGET_TEXT) {
                    if(0 == strcmp("time", player_value_key)) {
                        player_sprintf(player, buffer, sizeof(buffer), widget_text_get_format(t));
                        widget_text_set_content(t, buffer);
                    }
                }
            }

                if (wtype == WIDGET_TEXT) {
                    if(0 == strcmp("fps", runtime_value_key)) {
                        snprintf(buffer, sizeof(buffer), "FPS:%u", app_ctx->workspace.reported_fps);
                        widget_text_set_content(t, buffer);
                    }
                }

            if (wtype == WIDGET_TEXT && *widget_text_get_timedate_format(t)) {
                timedate_sprintf(buffer, sizeof(buffer), widget_text_get_timedate_format(t));
                debug_printf("'%s' -> '%s'\n", widget_text_get_timedate_format(t), buffer);
                widget_text_set_content(t, buffer);
            }
        }
        }
    }
    close_local_player(player);
    player = NULL;
    puts("\n\n");
}

static void set_visualiser_lock(bool lock, action_t action) {
    for(int ix =0; ix < MAX_NP_VIEWS && propagate_visualiser_change; ++ix) {
        view_context_ptr pv = np_views[ix];
        if (pv) {
            for(widget_t* t = widget_list_tail(pv->list); t != NULL; t = widget_list_prev(pv->list, t)) {
                if (widget_get_type(t) == WIDGET_VUMETER) {
                    widget_vumeter_select_lock(t, lock);
                }
                if (widget_get_type(t) == WIDGET_MULTISTATE_BUTTON) {
                    widget_multistate_button_sync_on_action(t, action);
                }
            }
        }
    }
}

void lock_vu_meters() {
    set_visualiser_lock(true, ACTION_LOCK_VUMETER);
}

void unlock_vu_meters() {
    set_visualiser_lock(false, ACTION_UNLOCK_VUMETER);
}

void lock_visualisers() {
    set_visualiser_lock(true, ACTION_LOCK_VUMETER);
    set_visualiser_lock(true, ACTION_LOCK_VISU);
}

void unlock_visualisers() {
    set_visualiser_lock(false, ACTION_UNLOCK_VUMETER);
    set_visualiser_lock(false, ACTION_UNLOCK_VISU);
}

