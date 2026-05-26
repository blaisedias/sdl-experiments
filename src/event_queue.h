#ifndef __jl_event_queue_h_
#define __jl_event_queue_h_
#include <SDL2/SDL.h>
#include "types.h"

bool evt_q_put(SDL_Event* evt);
bool evt_q_get(SDL_Event* evt);
bool evt_q_peek(SDL_Event* evt);
bool evt_q_try_put(SDL_Event* evt, unsigned try_count, int64_t sleepmicros);

#endif // __jl_event_queue_h_
