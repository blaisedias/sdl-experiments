#ifndef __jl_action_h_
#define __jl_action_h_

typedef enum {
    ACTION_NONE,
    ACTION_QUIT,
    ACTION_NEXT_VISU,
    ACTION_PREV_VISU,
    ACTION_NEXT_VU,
    ACTION_PREV_VU,
    ACTION_NEXT_SP,
    ACTION_PREV_SP,
    ACTION_LOCK_VUMETER,
    ACTION_UNLOCK_VUMETER,
    ACTION_LOCK_VISU,
    ACTION_UNLOCK_VISU,

    ACTION_PLAY,
    ACTION_PAUSE,
    ACTION_STOP,
    ACTION_PLAY_PAUSE,

    ACTION_NEXT_TRACK,
    ACTION_PREV_TRACK,

    ACTION_REPEAT_ONCE,
    ACTION_REPEAT,
    ACTION_REPEAT_OFF,

    ACTION_SHUFFLE,
    ACTION_SHUFFLE_ALBUM,
    ACTION_SHUFFLE_OFF,

    ACTION_MUSIC_INFORMATION,

    ACTION_SET_VOLUME,
    ACTION_INCREMENT_VOLUME,
    ACTION_DECREMENT_VOLUME,

    ACTION_SEEK,

    ACTION_NEXT_NP_VIEW,
    ACTION_PREV_NP_VIEW,

    ACTION_NP_VIEW,
    ACTION_MAIN_VIEW,

    ACTION_END,
} action_t;

action_t action_from_string(const char* str);
const char* action_to_string(action_t action);
void dispatch_action(action_t, int value);
#endif // __jl_action_h_
