#ifndef __jl_touch_screen_sdl2_h_
#define __jl_touch_screen_sdl2_h_
#include <SDL2/SDL.h>
#include "touch_screen.h"
#include "sdl_userevents.h"

int start_touch_screen_event_generator(touch_screen_svc_config* config);
int stop_touch_screen_event_generator(void);
#endif  // __jl_touch_screen_sdl2_h_
