#ifndef __jl_nowplaying_h_
#define __jl_nowplaying_h_

#include "lyrion_player.h"
// These functions must be implemented by main application code
// make nowplaying view active
void select_main_view(void);
// make nowplaying view active
void select_np_view(void);
// Select the next nowplaying view
void next_np_view(void);
// Select the previous nowplaying view
void prev_np_view(void);
// get pointer to the current player
lyrion_player_ptr get_player(void);
// lock vumeters on all views
void lock_vu_meters(void);
// unlock vumeters on all views
void unlock_vu_meters(void);
// lock visualisers on all views
void lock_visualisers(void);
// unlock visualisers on all views
void unlock_visualisers(void);

void timedate_sprintf(char* buff, size_t bufflen, const char *format);
#endif
