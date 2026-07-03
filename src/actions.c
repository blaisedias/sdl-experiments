#include "application.h"
#include "actions.h"
//FIXME
#include "widgets_internal.h"
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


static void widget_dispatch_action_explicit(widget_t* wdgt, action_t act) {
    action_printf("%p %d %s\n", wdgt, act, action_to_string(act));
    switch(act) {
        case ACTION_NONE:
        case ACTION_QUIT:
        case ACTION_NEXT_VISU:
        case ACTION_PREV_VISU:
        case ACTION_NEXT_VU:
        case ACTION_PREV_VU:
        case ACTION_NEXT_SP:
        case ACTION_PREV_SP:
        case ACTION_LOCK_VUMETER:
        case ACTION_UNLOCK_VUMETER:
        case ACTION_LOCK_VISU:
        case ACTION_UNLOCK_VISU:
            dispatch_action(act);
            break;

        case ACTION_MULTISTATE_BUTTON:
            error_printf("multistate action %d for %p type=%d\n", act, wdgt, wdgt->type);
            break;

        case ACTION_PLAY:
        case ACTION_PAUSE:
        case ACTION_STOP:
        case ACTION_PLAY_PAUSE:

        case ACTION_NEXT_TRACK:
        case ACTION_PREV_TRACK:

        case ACTION_REPEAT_ONCE:
        case ACTION_REPEAT:
        case ACTION_REPEAT_OFF:

        case ACTION_SHUFFLE:
        case ACTION_SHUFFLE_ALBUM:
        case ACTION_SHUFFLE_OFF:
            dispatch_action(act);
            break;

        case ACTION_MUSIC_INFORMATION:
            break;

        case ACTION_SET_VOLUME:
            if (wdgt->type == WIDGET_SLIDER) {
                int level;
                widget_slider_get_value(wdgt, &level);
                action_printf("volume level = %d\n", level);
                player_volume_set(get_player(), level);
            }
            break;

        case ACTION_INCREMENT_VOLUME:
        case ACTION_DECREMENT_VOLUME:
            dispatch_action(act);
            break;

        case ACTION_SEEK:
            if (wdgt->type == WIDGET_SLIDER) {
                int track_time;
                widget_slider_get_value(wdgt, &track_time);
                action_printf("seek = %d\n", track_time);
                player_seek(get_player(), track_time);
            }
            break;

        case ACTION_NEXT_NP_VIEW:
        case ACTION_PREV_NP_VIEW:

        case ACTION_NP_VIEW:
        case ACTION_MAIN_VIEW:

        case ACTION_END:
            dispatch_action(act);
            break;
        default:
            error_printf("unknown action %d for %p type=%d\n", act, wdgt, wdgt->type);
            break;
    }
}

static void action_multi_state_button(widget_t* wdgt) {
    action_printf("action_multistate_button action state=%d %d %s\n", 
            wdgt->sub.multistate_button.state,
            wdgt->sub.multistate_button.res[wdgt->sub.multistate_button.state].dispatch_action,
            action_to_string( wdgt->sub.multistate_button.res[wdgt->sub.multistate_button.state].dispatch_action));
    widget_dispatch_action_explicit(wdgt, wdgt->sub.multistate_button.res[wdgt->sub.multistate_button.state].dispatch_action);
//    wdgt->sub.multistate_button.state = (wdgt->sub.multistate_button.state + 1) % wdgt->sub.multistate_button.state_count;
}

void widget_dispatch_action(widget_t* wdgt) {
    if (wdgt->type == WIDGET_SLIDER && (!wdgt->sub.slider.defined_interactive || !wdgt->sub.slider.interactive)) {
        return;
    }
    switch(wdgt->action) {
        default:
            widget_dispatch_action_explicit(wdgt, wdgt->action);
            break;
        case ACTION_MULTISTATE_BUTTON:
            action_multi_state_button(wdgt);
            break;
    }
}

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
        "multistate_button",

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

void dispatch_action(action_t act) {
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
            
        case ACTION_MULTISTATE_BUTTON:
            // NOTHING TO DO
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
            // TODO nothing?
            break;

        case ACTION_INCREMENT_VOLUME:
            player_volume_inc(get_player());
            break;
        case ACTION_DECREMENT_VOLUME:
            player_volume_dec(get_player());
            break;

        case ACTION_SEEK:
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


