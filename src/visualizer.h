#ifndef __jl_visualiser_h
#define __jl_visualiser_h
#include "types.h"

void vis_check(void);
void vis_lock(void);
void vis_unlock(void);

bool vis_get_playing(void);
u32_t vis_get_rate(void);

s16_t *vis_get_buffer(void);
u32_t vis_get_buffer_len(void);
u32_t vis_get_buffer_idx(void);
int visualizer_vumeter(int* levels);
#endif //__jl_visualiser_h

