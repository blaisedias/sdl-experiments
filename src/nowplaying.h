#ifndef __jl_nowplaying_h_
#define __jl_nowplaying_h_

#include "lyrion_player.h"
// These functions must be implemented by main application code
// Select the next nowplaying view
void next_np_view();
// Select the previous nowplaying view
void prev_np_view();
// get pointer to the current player
lyrion_player_ptr get_player();
// lock vumeters on all views
void lock_vu_meters();
// unlock vumeters on all views
void unlock_vu_meters();
// lock visualisers on all views
void lock_visualisers();
// unlock visualisers on all views
void unlock_visualisers();
#endif
