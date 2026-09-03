#ifndef __jl_audio_volume_h_
#define __jl_audio_volume_h_

// For now the number of channels is fix at 2
// a future change will remove this hard-coding
#define     NUM_VU_CHANNELS     2

typedef struct {
    int     vol;
    int     peak_hold_vol;
    int     peak_hold_counter;
    int     decay_hold_counter;
    float   decay_vol;
    float   decay_unit;
}runtime_volume_t, *runtime_volume_ptr;

void update_volume_levels(runtime_volume_ptr vol_runtimes, float decay_unit);
int vumeter_set_peak_hold(int v);
int vumeter_set_decay_hold(int v);

#endif
