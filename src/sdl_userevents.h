#ifndef __jl_userevents_h
#define __jl_userevents_h
#include <SDL2/SDL.h>

typedef enum {
    USEREVENT_FINGERDOWN = SDL_USEREVENT + 1,
    USEREVENT_FINGERUP,
    USEREVENT_FINGERMOTION,
    USEREVENT_NEXT_VISU,
    USEREVENT_PREV_VISU,
    USEREVENT_NEXT_VU,
    USEREVENT_PREV_VU,
    USEREVENT_NEXT_SP,
    USEREVENT_PREV_SP,
} user_event;

#endif
