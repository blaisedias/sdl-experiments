#include "application.h"
#include "actions.h"
//FIXME
#include "widgets.h"
#include "logging.h"
#include "lyrion_player.h"
#include "nowplaying.h"

static SDL_Event quit_event = {.type = SDL_QUIT };
static SDL_Event next_visu_event = {.type = USEREVENT_NEXT_VISU };
static SDL_Event prev_visu_event = {.type = USEREVENT_PREV_VISU };
static SDL_Event next_vu_event = {.type = USEREVENT_NEXT_VU };
static SDL_Event prev_vu_event = {.type = USEREVENT_PREV_VISU };
static SDL_Event next_sp_event = {.type = USEREVENT_NEXT_SP };
static SDL_Event prev_sp_event = {.type = USEREVENT_PREV_SP };

static const char* action_strings[] = {
        "",                 /* NONE */
        "quit",
        "next_visu",
        "prev_visu",
        "next_vu",
        "prev_vu",
        "next_sp",
        "prev_sp",
        "lock_vumeter",
        "unlock_vumeter",
        "lock_visu",
        "unlock_visu",

        "play",
        "pause",
        "stop",
        "play_pause",

        "next-track",
        "previous-track",

        "repeat-once",
        "repeat",
        "repeat-off",

        "shuffle",
        "shuffle-album",
        "shuffle-off",

        "music-information",

        "set-volume-level",
        "increment-volume",
        "decrement-volume",

        "seek",

        "next-nowplaying-view",
        "prev-nowplaying-view",

        "nowplaying_view",
        "main_view",
        "",                 /* END */
};

action_t action_from_string(const char* str) {
    if (str != NULL ) {
        for(int a=0; a < sizeof(action_strings)/sizeof(action_strings[0]); ++a) {
            if (0 == strcmp(action_strings[a], str)) {
                return a;
            }
        }
    }
    if (str) {
        error_printf("unknown action %s\n", str);
    }
    return ACTION_NONE;
}

const char* action_to_string(action_t action) {
    if (action > ACTION_NONE && action < ACTION_END ) {
        return action_strings[action];
    }
    if (action == ACTION_NONE) return "ACTION_NONE";
    if (action == ACTION_END) return "ACTION_NONE(END)";
    return "ACTION_UNKNOWN";
}

void dispatch_action(action_t act, int value) {
    switch(act) {
        case ACTION_NONE:
            break;
        case ACTION_QUIT:
            SDL_PushEvent(&quit_event);
            break;
        case ACTION_NEXT_VISU:
            SDL_PushEvent(&next_visu_event);
            break;
        case ACTION_PREV_VISU:
            SDL_PushEvent(&prev_visu_event);
            break;
        case ACTION_NEXT_VU:
            SDL_PushEvent(&next_vu_event);
            break;
        case ACTION_PREV_VU:
            SDL_PushEvent(&prev_vu_event);
            break;
        case ACTION_NEXT_SP:
            SDL_PushEvent(&next_sp_event);
            break;
        case ACTION_PREV_SP:
            SDL_PushEvent(&prev_sp_event);
            break;
        case ACTION_LOCK_VUMETER:
            lock_vu_meters();
            break;
        case ACTION_UNLOCK_VUMETER:
            unlock_vu_meters();
            break;
        case ACTION_LOCK_VISU:
            lock_visualisers();
            break;
        case ACTION_UNLOCK_VISU:
            unlock_visualisers();
            break;
            
        case ACTION_PLAY:
            player_play(get_player());
            break;
        case ACTION_PAUSE:
            player_pause(get_player());
            break;
        case ACTION_STOP:
            player_stop(get_player());
            break;
        case ACTION_PLAY_PAUSE:
            player_play_pause_toggle(get_player());
            break;

        case ACTION_NEXT_TRACK:
            player_fwd(get_player());
            break;
        case ACTION_PREV_TRACK:
            player_rew(get_player());
            break;

        case ACTION_REPEAT_ONCE:
            player_repeat_one(get_player());
            break;
        case ACTION_REPEAT:
            player_repeat_toggle(get_player());
            break;
        case ACTION_REPEAT_OFF:
            player_repeat_off(get_player());
            break;

        case ACTION_SHUFFLE:
            player_shuffle_on(get_player());
            break;
        case ACTION_SHUFFLE_ALBUM:
            player_shuffle_on(get_player());
            player_shuffle_toggle(get_player());
            break;
        case ACTION_SHUFFLE_OFF:
            player_shuffle_off(get_player());
            break;

        case ACTION_MUSIC_INFORMATION:
            // TODO
            break;

        case ACTION_SET_VOLUME:
            action_printf("volume level = %d\n", value);
            player_volume_set(get_player(), value);
            break;

        case ACTION_INCREMENT_VOLUME:
            player_volume_inc(get_player());
            break;
        case ACTION_DECREMENT_VOLUME:
            player_volume_dec(get_player());
            break;

        case ACTION_SEEK:
            action_printf("seek = %d\n", value);
            player_seek(get_player(), value);
            // TODO nothing?
            break;

        case ACTION_NEXT_NP_VIEW:
            next_np_view();
            break;
        case ACTION_PREV_NP_VIEW:
            prev_np_view();
            break;

        case ACTION_NP_VIEW:
            select_np_view();
            break;
        case ACTION_MAIN_VIEW:
            select_main_view();
            break;

        case ACTION_END:
            break;
    }
}


