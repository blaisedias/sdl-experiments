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
"\n"
" - peakhold <count>: number of frames for VU peak hold\n"
" - decayhold <count>: number of frames for VU decay hold - reduces needle jitter\n"
"\n"
" - texture_cache_size <count>: maximum number of texture bytes\n"
"\n"
" - lms <name>: lyrion media server network name or ip address \n"
"\n";  

static volatile view_context_ptr view = NULL;
static char *json_file = "npvu.json";
static bool dump_vu = false;

static void my_render(app_context_ptr app_ctx);
static void my_event_handler(app_context_ptr app_ctx, SDL_Event* eventp);
static void player_poll_loop(app_context_ptr app_ctx);

static SDL_sem* controller_sem;

static app_context app_ctx = {
//        .window = NULL,
//        .renderer = NULL,
        .screen_width = 800,
        .screen_height = 480,
//        .orientation = 0,
        .vsync = true,
        .input_loop_sleep_millis = 100,
        .cb_render = my_render,
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
        } else if (0 == strcmp(argv[i], "perf_level")) {
            if (argc > i+1) {
                VUMeter_set_perf_level(atoi(argv[i+1]));
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
            const vumeter_properties *p = VUMeter_get_props_list();
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
                json_file = argv[i+1];
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

static void controller(app_context_ptr app_ctx) {
    app_wait_ready();
//debug    
printf("starting controller\n"); fflush(stdout);
    SDL_ShowCursor(SDL_DISABLE);
    int64_t endtime = app_ctx->max_secs * 1000;
    if (endtime) {
        endtime += get_milli_seconds();
    }
    view_context vw = { .app = app_ctx, .list=create_widget_list(&vw) };
    if (VUMeter_get_props_list() == NULL) {
        error_printf("No VU Meters found\n");
//        app_stop(app_ctx);
    }

    if (json_file && strlen(json_file)) {
        if ( 0 != deserialise_widgets_file(json_file, &vw)) {
            error_printf("failed to deserialise widgets from file %s\n", json_file);
//            app_stop(app_ctx);
        } else {
            if (dump_vu) {
                const vumeter_properties* vp = VUMeter_get_props_list();
                while(vp) {
                    VUMeter_dump_props(vp);
                    vp = vp->next;
                }
            }
    
            widget_list_load_media(vw.list, "./images");
        }
        view = &vw;
    }
    
    int64_t next_vu_time = app_ctx->cycle_secs * 1000;
    if (next_vu_time) {
        next_vu_time += get_milli_seconds();
    }
    while(app_running(app_ctx)) {
        sleep_milli_seconds(100);
        if (show_cursor) {
            if (0 >= --show_cursor) {
                SDL_ShowCursor(SDL_DISABLE);
                show_cursor = 0;
                if (view) {
                    for(widget* widget=view->list->tail.prev; widget != NULL; widget=widget->prev) {
                        if (widget->hidden) { continue;}
                        widget->focussed = false;
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
    }
}

static void my_render(app_context_ptr app_ctx) {
    if (view) {
        for(widget* widget=view->list->head.next; widget != NULL; widget=widget->next) {
            if (!widget->hidden) {
                widget->render(widget);
            }
        }
    }
}

static void my_event_handler(app_context_ptr app_ctx, SDL_Event* eventp) {
    switch (eventp->type) {
            case USEREVENT_NEXT_VISU:
            case USEREVENT_NEXT_VU:
                {
                    if(view) {
                        for(widget* t = view->list->tail.prev; t != NULL; t = t->prev) {
                            if (t->type == WIDGET_VUMETER) {
                                widget_vumeter_select_next(t);
                            }
                        }
                    }
                }break;
            case USEREVENT_PREV_VISU:
            case USEREVENT_PREV_VU:
                {
                    if (view) {
                        for(widget* t = view->list->tail.prev; t != NULL; t = t->prev) {
                            if (t->type == WIDGET_VUMETER) {
                                widget_vumeter_select_prev(t);
                            }
                        }
                    }
                }break;
            case SDL_QUIT:
                puts("");
                app_stop(app_ctx);
                break;
            case SDL_KEYDOWN:
                switch (eventp->key.keysym.scancode) {
                case SDL_SCANCODE_ESCAPE: 
                    puts("");
                    app_stop(app_ctx);
                    break;
                case SDL_SCANCODE_SPACE:
                    {
                        unsigned texture_bytes = tcache_get_texture_bytes_count();
                        unsigned surface_bytes = tcache_get_surface_bytes_count();
                        printf("\n texture:%u %fMiB surface:%u %fMib\n", texture_bytes, (float)texture_bytes/(1024*1024), surface_bytes, (float)surface_bytes/(1024*1024));
                    }
                    break;
                case SDL_SCANCODE_T:
                    tcache_dump();
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
                    if (view) { widget_list_react(view->list, POINTER_MOTION, &pt); }
                } break;
            case SDL_MOUSEBUTTONDOWN:
                {
                    SDL_ShowCursor(SDL_ENABLE);
                    show_cursor = HIDE_CURSOR_COUNT;
                    SDL_Point pt = {.x=eventp->button.x, .y=eventp->button.y};
                    if (view) { widget_list_react(view->list, POINTER_DOWN, &pt); }
                } break;
            case SDL_MOUSEBUTTONUP:
                {
                    SDL_ShowCursor(SDL_ENABLE);
                    show_cursor = HIDE_CURSOR_COUNT;
                    SDL_Point pt = {.x=eventp->button.x, .y=eventp->button.y};
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
                    if (view) { widget_list_react(view->list, POINTER_MOTION, &pt); }
                }
                break;
            case USEREVENT_FINGERMOTION:
                {
                    SDL_Point pt = { .x = eventp->motion.x, .y = eventp->motion.y };
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
                    if (view) { widget_list_react(view->list, POINTER_DOWN, &pt); }
                }
                break;
            case USEREVENT_FINGERDOWN:
                {
                    SDL_Point pt = { .x = eventp->motion.x, .y = eventp->motion.y };
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
                    if (view) { widget_list_react(view->list, POINTER_UP, &pt); }
                }
                break;
            case USEREVENT_FINGERUP:
                {
                    SDL_Point pt = { .x = eventp->motion.x, .y = eventp->motion.y };
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

//debug
printf("starting player_poll_loop\n"); fflush(stdout);
    unsigned fps = 0;
    int force_visual_refresh=0;
    while(app_running(app_ctx)) {
        sleep_milli_seconds(500);
        // ensure that the player is initialised - if possible, nop if the player is initialised
        if (poll_player(player, &pts) || --force_visual_refresh <=0) {
            force_visual_refresh=4;
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

            if (view) {
            for(widget* t = view->list->tail.prev; t != NULL; t = t->prev) {
                if (t->player_range_value_key) {
                    player_value pvalue;
                    switch(get_player_value(player, &pvalue, t->player_range_value_key)) {
                        case PFV_NONE:
                            error_printf("got nothing for player range value %s\n", t->player_range_value_key);
                            break;
                        case PFV_INT:
                            debug_printf("got int %d for player range value %s\n", pvalue.integer, t->player_range_value_key);
                            if (t->type == WIDGET_SLIDER) {
                                widget_slider_range(t, 0, pvalue.integer);
                            }
                            break;
                        case PFV_STRINGPTR:
                            error_printf("got string %s for player range value %s\n", pvalue.strptr, t->player_range_value_key);
                            FREE(pvalue.strptr);
                            break;
                    }
                }
                if (t->player_value_key) {
                    player_value pvalue;
                    switch(get_player_value(player, &pvalue, t->player_value_key)) {
                        case PFV_NONE:
                            error_printf("got nothing for player value %s\n", t->player_value_key);
                            break;
                        case PFV_INT:
                            debug_printf("got int %d for player value %s\n", pvalue.integer, t->player_value_key);
                            if (t->type == WIDGET_MULTISTATE_BUTTON) {
                                widget_multistate_button_set_state(t, pvalue.integer);
                            } else if (t->type == WIDGET_SLIDER) {
                                widget_slider_set_value(t, pvalue.integer);
                            }
                            break;
                        case PFV_STRINGPTR:
                            error_printf("got string %s for player value %s\n", pvalue.strptr, t->player_value_key);
                            FREE(pvalue.strptr);
                            break;
                    }
                }
                if (t->type == WIDGET_SLIDER && 0 == strcmp(t->player_value_key, "time")) {
                    widget_slider_set_interactive(t, can_seek);
                }
                if (t->type == WIDGET_TEXT && t->sub.text.format) {
                    player_sprintf(player, buffer, sizeof(buffer), t->sub.text.format);
                    debug_printf("'%s' -> '%s'\n", t->sub.text.format, buffer);
                    widget_text_set_content(t, buffer);
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
        if (view) {
        for(widget* t = view->list->tail.prev; t != NULL; t = t->prev) {
            if (t->player_value_key) {
                if (0 == strcmp("VOLUME", t->player_value_key)) {
                    if (t->type == WIDGET_SLIDER) {
                        widget_slider_set_value(t, volume);
                    }
                }
                if (duration && 0 == strcmp("time", t->player_value_key)) {
                    if (t->type == WIDGET_SLIDER) {
                        widget_slider_set_value(t, elapsed);
                    }
                }
                if (t->type == WIDGET_TEXT) {
                    if(0 == strcmp("time", t->player_value_key)) {
                        player_sprintf(player, buffer, sizeof(buffer), t->sub.text.format);
                        widget_text_set_content(t, buffer);
                    }
                }
            }
            if (t->runtime_value_key) {
                if (t->type == WIDGET_TEXT) {
                    if(0 == strcmp("fps", t->runtime_value_key)) {
                        snprintf(buffer, sizeof(buffer), "FPS:%u", app_ctx->workspace.reported_fps);
                        widget_text_set_content(t, buffer);
                    }
                }
            }
        }
        }
    }
    close_local_player(player);
    player = NULL;
    puts("\n\n");
}

