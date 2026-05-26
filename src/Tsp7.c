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

#define WINDOW_TITLE "Tsp"
#define HIDE_CURSOR_COUNT 50
static int show_cursor = 0;
static const char* help_text=""
"\n"
" - [help, -h, --h] : print this text and exit\n"
"\n"  
" - vsync : use vertical sync when rendering each frame\n"
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


SDL_Rect enclosure;

typedef struct vmtr {
    texture_id_t background;
    texture_id_t sprite_image;
    SDL_Rect bg_rect1;
    SDL_Rect bg_rect2;
    SDL_Rect sprite_rect1;
    SDL_Point axle1;
    SDL_Rect sprite_rect2;
    SDL_Point axle2;
    SDL_Point sprite_pivot;
}*vmtr_ptr;

void vu_cleanup(vmtr_ptr pvu);
static struct vmtr vu = {
        .background = 0,
        .sprite_image = 0,
        .bg_rect1 = {0, 0, 0, 0},
        .bg_rect2 = {0, 0, 0, 0},
        .sprite_rect1 = {0, 0, 0, 0},
        .sprite_rect2 = {0, 0, 0, 0},
        .sprite_pivot = {0, 0},
    };

static volatile vmtr_ptr vu_ptr = NULL;

typedef struct render_context {
    bool once;
    int mul;
    int mmul;
    float rot_inc;
    float rot;
}render_context, *render_context_ptr;

static render_context render_data = { .once = true, };
static render_context_ptr render_ctx = &render_data;

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
        } else if (0 == strcmp(argv[i], "vsync")) {
            app_ctx.vsync = 1;
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
/*            
        } else if (0 == strcmp(argv[i], "dumpvu")) {
            app_ctx.dump_vu = true;
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
         } else if (0 == strcmp(argv[i], "vu")) {
            if (argc > i+1) {
                i += 1;
                app_ctx.first_vu_meter = argv[i];
            }
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
                app_ctx.json_file = argv[i+1];
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
*/            
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
    vu_cleanup(vu_ptr);

    app_cleanup(&app_ctx, EXIT_SUCCESS);

    return 0;
}


void scaleRect(SDL_Rect* rect, float scalef) {
    rect->x = rect->x * scalef + 0.5;
    rect->y = rect->y * scalef + 0.5;
    rect->w = rect->w * scalef + 0.5;
    rect->h = rect->h * scalef + 0.5;
}

void scalePoint(SDL_Point* pt, float scalef) {
    pt->x = pt->x * scalef + 0.5;
    pt->y = pt->y * scalef + 0.5;
}


void vu_cleanup(vmtr_ptr pvu) {
    tcache_quick_delete_texture(pvu->sprite_image);
    tcache_quick_delete_texture(pvu->background);
}

static bool load_media(app_context_ptr app_ctx, vmtr_ptr pvu) {
    bool loaded = false; 
    pvu->background = tcache_load_media("./tsp/x-white-face-1024x600.png", app_ctx->renderer, &loaded, &pvu->bg_rect1);
    if (!loaded) {
        error_printf("Error creating Texture: %s\n", IMG_GetError());
        return true;
    }

    loaded = false;
    pvu->sprite_image = tcache_load_media("./tsp/needle-white.png", app_ctx->renderer, &loaded, &pvu->sprite_rect1);
    if (!loaded) {
        error_printf("Error creating Texture: %s\n", IMG_GetError());
        return true;
    }

    int leftmargin = 50;
    int rightmargin = 50;
    int spacing = 50;
//    app_ctx->sprite_rect.x = 512 - app_ctx->sprite_rect.w;
    pvu->bg_rect1.x += leftmargin;
    pvu->axle1.x = 512 + leftmargin;
    pvu->axle1.y = 550;
    pvu->sprite_rect1.x = 512 - 21 + leftmargin;
    pvu->sprite_rect1.y = 550-367;
    pvu->sprite_pivot.x = 21;
    pvu->sprite_pivot.y = 367;


    copyRect(&pvu->bg_rect1, &pvu->bg_rect2);
    pvu->bg_rect2.x += 1024 + spacing;
    copyRect(&pvu->sprite_rect1, &pvu->sprite_rect2);
    pvu->sprite_rect2.x += 1024 + spacing;
    pvu->axle2.x = pvu->axle1.x + 1024 + spacing;
    pvu->axle2.y = pvu->axle1.y;

    debug_printf("window   =  %dx%d\n", app_ctx->screen_width, app_ctx->screen_height);
    debug_printf("screen   =  %d,%d %d,%d\n", app_ctx->window_rect.x, app_ctx->window_rect.y, app_ctx->window_rect.w, app_ctx->window_rect.h);
    copyRect(&app_ctx->window_rect, &enclosure);
    enclosure.w /=2;
    enclosure.h /=2;
    enclosure.x += enclosure.w;
    enclosure.y += enclosure.h;
    debug_printf("enclosure=  %d,%d %d,%d\n", enclosure.x, enclosure.y, enclosure.w, enclosure.h);
    debug_printf("original:\n");
    debug_printf("bg       = {%d,%d,%d,%d}\n", pvu->bg_rect1.x, pvu->bg_rect1.y, pvu->bg_rect1.w, pvu->bg_rect1.h);
    debug_printf("  axle   = {%d,%d}\n", pvu->axle1.x, pvu->axle1.y);
    debug_printf("         = {%d,%d,%d,%d}\n", pvu->bg_rect2.x, pvu->bg_rect2.y, pvu->bg_rect2.w, pvu->bg_rect2.h);
    debug_printf("  axle   = {%d,%d}\n", pvu->axle2.x, pvu->axle2.y);
    debug_printf("needle   = {%d,%d,%d,%d}\n", pvu->sprite_rect1.x, pvu->sprite_rect1.y, pvu->sprite_rect1.w, pvu->sprite_rect1.h);
    debug_printf("         = {%d,%d,%d,%d}\n", pvu->sprite_rect2.x, pvu->sprite_rect2.y, pvu->sprite_rect2.w, pvu->sprite_rect2.h);
    debug_printf("pivot    = {%d,%d}\n", pvu->sprite_pivot.x, pvu->sprite_pivot.y);


//    float scalef =  (float)enclosure.w/(pvu->bg_rect1.w *2);
    float scalef =  (float)enclosure.w/( leftmargin + spacing + rightmargin + pvu->bg_rect1.w *2);
    scaleRect(&pvu->bg_rect1, scalef);
    scaleRect(&pvu->bg_rect2, scalef);
    scaleRect(&pvu->sprite_rect1, scalef);
    scaleRect(&pvu->sprite_rect2, scalef);
    scalePoint(&pvu->sprite_pivot, scalef);
    scalePoint(&pvu->axle1, scalef);
    scalePoint(&pvu->axle2, scalef);
    int dy = (enclosure.h - pvu->bg_rect1.h)/2;
    pvu->bg_rect1.y += dy;
    pvu->bg_rect2.y += dy;
    pvu->sprite_rect1.y += dy;
    pvu->sprite_rect2.y += dy;
    pvu->axle1.y += dy;
    pvu->axle2.y += dy;
    /*
    vcenterRect(&pvu->bg_rect1);
    vcenterRect(&pvu->bg_rect2);
    vcenterRect(&pvu->sprite_rect1);
    vcenterRect(&pvu->sprite_rect2);
    */

    debug_printf("---------------------------------------------\n");
    debug_printf("scaled:\n");
    debug_printf("bg       = {%d,%d,%d,%d}\n", pvu->bg_rect1.x, pvu->bg_rect1.y, pvu->bg_rect1.w, pvu->bg_rect1.h);
    debug_printf("  axle   = {%d,%d}\n", pvu->axle1.x, pvu->axle1.y);
    debug_printf("         = {%d,%d,%d,%d}\n", pvu->bg_rect2.x, pvu->bg_rect2.y, pvu->bg_rect2.w, pvu->bg_rect2.h);
    debug_printf("  axle   = {%d,%d}\n", pvu->axle2.x, pvu->axle2.y);
    debug_printf("needle   = {%d,%d,%d,%d}\n", pvu->sprite_rect1.x, pvu->sprite_rect1.y, pvu->sprite_rect1.w, pvu->sprite_rect1.h);
    debug_printf("         = {%d,%d,%d,%d}\n", pvu->sprite_rect2.x, pvu->sprite_rect2.y, pvu->sprite_rect2.w, pvu->sprite_rect2.h);
    debug_printf("   pivot = {%d,%d}\n", pvu->sprite_pivot.x, pvu->sprite_pivot.y);
    
    translate_axle(&pvu->bg_rect1, &pvu->axle1, &pvu->sprite_rect1);
    translate_axle(&pvu->bg_rect2, &pvu->axle2, &pvu->sprite_rect2);

    translate_image_rect(&pvu->bg_rect1);
    translate_image_rect(&pvu->bg_rect2);
    translate_draw_rect(&enclosure);

    debug_printf("---------------------------------------------\n");
    debug_printf("oriented:\n");
    debug_printf("enclosure=  %d,%d %d,%d\n", enclosure.x, enclosure.y, enclosure.w, enclosure.h);
    debug_printf("bg       = {%d,%d,%d,%d}\n", pvu->bg_rect1.x, pvu->bg_rect1.y, pvu->bg_rect1.w, pvu->bg_rect1.h);
    debug_printf("  axle   = {%d,%d}\n", pvu->axle1.x, pvu->axle1.y);
    debug_printf("         = {%d,%d,%d,%d}\n", pvu->bg_rect2.x, pvu->bg_rect2.y, pvu->bg_rect2.w, pvu->bg_rect2.h);
    debug_printf("  axle   = {%d,%d}\n", pvu->axle2.x, pvu->axle2.y);
    debug_printf("needle   = {%d,%d,%d,%d}\n", pvu->sprite_rect1.x, pvu->sprite_rect1.y, pvu->sprite_rect1.w, pvu->sprite_rect1.h);
    debug_printf("         = {%d,%d,%d,%d}\n", pvu->sprite_rect2.x, pvu->sprite_rect2.y, pvu->sprite_rect2.w, pvu->sprite_rect2.h);
    debug_printf("   pivot = {%d,%d}\n", pvu->sprite_pivot.x, pvu->sprite_pivot.y);
 
    return false;
}

static void controller(app_context_ptr app_ctx) {
    app_wait_ready();
//debug    
printf("starting controller\n"); fflush(stdout);

    render_ctx->rot = 6;
    render_ctx->rot_inc = 3.5;
    render_ctx->mul  = 1;
    render_ctx->mmul  = 1;

//    bool tmp = set_printf_onoff(DEBUG_PRINTF, false);
    if (load_media(app_ctx, &vu)) {
        app_stop(app_ctx);
    }
    vu_ptr = &vu;
//    set_printf_onoff(DEBUG_PRINTF, tmp);

    SDL_ShowCursor(SDL_DISABLE);
    int64_t endtime = app_ctx->max_secs * 1000;
    if (endtime) {
        endtime += get_milli_seconds();
    }
    while(app_running(app_ctx)) {
        sleep_milli_seconds(100);
        if (show_cursor) {
            if (0 >= --show_cursor) {
                SDL_ShowCursor(SDL_DISABLE);
                show_cursor = 0;
/*                
                for(widget* widget=view->list->tail.prev; widget != NULL; widget=widget->prev) {
                    if (widget->hidden) { continue;}
                    widget->focussed = false;
                    widget_set_highlight(widget, false);
                }
*/            
            }
        }
        if (endtime) {
            if (get_milli_seconds() > endtime) {
                app_stop(app_ctx);
            }
        }
    }
}

static void crossHairs(SDL_Renderer* renderer, SDL_Point *pt, int d) {
    SDL_RenderDrawLine(renderer, pt->x, pt->y-d, pt->x, pt->y+d);
    SDL_RenderDrawLine(renderer, pt->x-d, pt->y, pt->x+d, pt->y);
}

static void my_render(app_context_ptr app_ctx) {
//        mul  = 0;
//        render_ctx->mmul  = 1;
//        SDL_RenderClear(app_ctx->renderer);

        if (!vu_ptr) {
            return;
        }
        SDL_Rect bg_rect1;
        SDL_Rect bg_rect2;
        SDL_Rect sprite_rect1;
        SDL_Rect sprite_rect2;
        SDL_Point axle1;
        SDL_Point axle2;
        rebaseRect(&enclosure, &vu_ptr->bg_rect1, &bg_rect1);
        rebaseRect(&enclosure, &vu_ptr->bg_rect2, &bg_rect2);
        rebaseRect(&enclosure, &vu_ptr->sprite_rect1, &sprite_rect1);
        rebaseRect(&enclosure, &vu_ptr->sprite_rect2, &sprite_rect2);
        rebasePoint(&enclosure, &vu_ptr->axle1, &axle1);
        rebasePoint(&enclosure, &vu_ptr->axle2, &axle2);
        if (render_ctx->once) {
            render_ctx->once = false;
    debug_printf("++bg1      = {%d,%d,%d,%d}\n", bg_rect1.x, bg_rect1.y, bg_rect1.w, bg_rect1.h);
    debug_printf("++axle1    = {%d,%d}\n", axle1.x, axle1.y);
    debug_printf("++sp1      = {%d,%d,%d,%d}\n", sprite_rect1.x, sprite_rect1.y, sprite_rect1.w, sprite_rect1.h);
    debug_printf("++bg2      = {%d,%d,%d,%d}\n", bg_rect2.x, bg_rect2.y, bg_rect2.w, bg_rect2.h);
    debug_printf("++axle2    = {%d,%d}\n", axle2.x, axle2.y);
    debug_printf("++sp2      = {%d,%d,%d,%d}\n", sprite_rect2.x, sprite_rect2.y, sprite_rect2.w, sprite_rect2.h);
        }

        {
            SDL_RenderCopyEx(app_ctx->renderer,
                    tcache_quick_get_texture(vu_ptr->background, app_ctx->renderer),
                    NULL, &bg_rect1, 0, NULL, SDL_FLIP_NONE);
            SDL_RenderCopyEx(app_ctx->renderer,
                    tcache_quick_get_texture(vu_ptr->background, app_ctx->renderer),
                    NULL, &bg_rect2, 0, NULL, SDL_FLIP_NONE);
        }

        if (true) {
            SDL_RenderCopyEx(app_ctx->renderer,
                    tcache_quick_get_texture(vu_ptr->sprite_image, app_ctx->renderer),
                    NULL, &sprite_rect1, 0 + render_ctx->rot-90, &vu_ptr->sprite_pivot, SDL_FLIP_NONE);
            SDL_RenderCopyEx(app_ctx->renderer,
                    tcache_quick_get_texture(vu_ptr->sprite_image, app_ctx->renderer),
                    NULL, &sprite_rect2, 0 + render_ctx->rot-90, &vu_ptr->sprite_pivot, SDL_FLIP_NONE);
        }

        SDL_SetRenderDrawColor(app_ctx->renderer, 128, 128, 128, 128);
        SDL_SetRenderDrawColor(app_ctx->renderer, 128, 0, 0, 128);
        SDL_RenderDrawRect(app_ctx->renderer, &bg_rect1);
        SDL_SetRenderDrawColor(app_ctx->renderer, 0, 128, 0, 128);
        SDL_RenderDrawRect(app_ctx->renderer, &bg_rect2);
        SDL_SetRenderDrawColor(app_ctx->renderer, 0, 255, 0, 255);
        SDL_RenderDrawRect(app_ctx->renderer, &sprite_rect1);
        SDL_SetRenderDrawColor(app_ctx->renderer, 255, 255, 0, 255);
        SDL_RenderDrawRect(app_ctx->renderer, &sprite_rect2);
        SDL_SetRenderDrawColor(app_ctx->renderer, 255, 0, 0, 255);
        crossHairs(app_ctx->renderer, &axle1, 30);
        crossHairs(app_ctx->renderer, &axle2, 30);
        SDL_SetRenderDrawColor(app_ctx->renderer, 255, 255, 0, 255);
        SDL_RenderDrawRect(app_ctx->renderer, &enclosure);
        SDL_SetRenderDrawColor(app_ctx->renderer, 0, 0, 0, 255);

        render_ctx->rot += render_ctx->rot_inc * render_ctx->mul;
        if (render_ctx->rot > 174) {
            render_ctx->rot_inc *= -1;
            render_ctx->rot = 174;
        }
        if (render_ctx->rot < 6) {
            render_ctx->rot_inc *= -1;
            render_ctx->rot = 6;
        }
        render_ctx->mul *= render_ctx->mmul;
}

static volatile bool input_once = true;
static void my_event_handler(app_context_ptr app_ctx, SDL_Event* eventp) {
    if (input_once) {
        printf("input %d\n", eventp->type);
        input_once = false;
    }
    switch (eventp->type) {
        case SDL_QUIT:
            app_stop(app_ctx);
            break;
        case SDL_KEYUP:
            switch (eventp->key.keysym.scancode) {
            case SDL_SCANCODE_ESCAPE:
                app_stop(app_ctx);
                break;
            case SDL_SCANCODE_SPACE:
                render_ctx->mul = render_ctx->mmul ^= 1;
                break;
            case SDL_SCANCODE_KP_MINUS:
                render_ctx->rot_inc *= -1;
                break;
            default:
                break;
            }
            break;
        case SDL_KEYDOWN:
            switch (eventp->key.keysym.scancode) {
            case SDL_SCANCODE_KP_PLUS:
                render_ctx->mul = 1;
                render_ctx->mmul = 0;
                break;
            default:
                break;
            }
            break;
        case SDL_FINGERUP:
        case USEREVENT_FINGERUP:
            app_stop(app_ctx);
            break;
        case SDL_MOUSEMOTION:
            {
                SDL_ShowCursor(SDL_ENABLE);
                show_cursor = HIDE_CURSOR_COUNT;
            } break;
        case SDL_MOUSEBUTTONDOWN:
            {
                SDL_ShowCursor(SDL_ENABLE);
                show_cursor = HIDE_CURSOR_COUNT;
            } break;
        case SDL_MOUSEBUTTONUP:
            {
                SDL_ShowCursor(SDL_ENABLE);
                show_cursor = HIDE_CURSOR_COUNT;
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
    while(app_running(app_ctx)) {
        sleep_milli_seconds(500);
        // ensure that the player is initialised - if possible, nop if the player is initialised
        if (poll_player(player, &pts)) {
            puts("P poll data");
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

/*            
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
*/
            player_sprintf(player, buffer, sizeof(buffer), "{playlist_cur_index}{TITLE}{ARTIST}{ALBUM_OR_REMOTE_TITLE}");
            uint64_t new_sig = compute_player_hash(buffer);
            player_sprintf(player, buffer, sizeof(buffer), "{playlist_cur_index}) Title:{TITLE} Artist:{ARTIST} Album:{ALBUM_OR_REMOTE_TITLE}");
            printf("<< %lu %s >>\n", (unsigned long)new_sig, buffer);
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
/*                
        player_value pvalue;
        get_player_value(player, &pvalue, "VOLUME");
        int volume = pvalue.integer;
        get_player_value(player, &pvalue, "DURATION");
        int duration = pvalue.integer;
        get_player_value(player, &pvalue, "time");
        int elapsed = pvalue.integer;

        printf("%d %d:%02d/%d:%02d %d:%02d\r", volume,
                elapsed/60, elapsed%60,
                duration/60, duration%60,
                (elapsed-duration)/60, abs((elapsed-duration)%60)
                );
        fflush(stdout);
*/
        static unsigned fps;
        if (fps !=  app_ctx->workspace.reported_fps) {
            profile_printf("fps: %u\n", app_ctx->workspace.reported_fps);
            fps = app_ctx->workspace.reported_fps;
        }
        /*
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
                        snprintf(buffer, sizeof(buffer), "FPS:%u", app_wksp->reported_fps);
                        widget_text_set_content(t, buffer);
                    }
                }
            }
        }
        */
    }
    close_local_player(player);
    player = NULL;
}

