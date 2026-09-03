/*
** Copyright 2010 Logitech. All Rights Reserved.
** Copyright 2025 Blaise Dias
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/


#include "types.h"
#include "visualizer.h"
#include "audio_volume.h"

#define VUMETER_DEFAULT_SAMPLE_WINDOW 1024 * 2

static long long RMS_MAP[] = {
	   0,    2,    5,    7,   10,   21,   33,   45,   57,   82,
	 108,  133,  159,  200,  242,  284,  326,  387,  448,  509,
	 570,  652,  735,  817,  900, 1005, 1111, 1217, 1323, 1454,
	1585, 1716, 1847, 2005, 2163, 2321, 2480, 2666, 2853, 3040,
	3227, 3414, 3601, 3788, 3975, 4162, 4349, 4536, 4755, 5000,
};

extern void (*vol_printf)(char *format, ...);
extern void (*vol_calib_printf)(char *format, ...);
extern void (*log_printf)(char *format, ...);

void _digitise(int* levels, long long* sample_accumulator, size_t num_samples) {
	sample_accumulator[0] /= num_samples;
	sample_accumulator[1] /= num_samples;

	for(int indx =0; indx < 2; ++indx) {
		levels[indx] = 0;
		for (int level = 48; level >=0; --level) {
			if (sample_accumulator[indx] > RMS_MAP[level]) {
				levels[indx] = level;
				vol_printf("%02d a:%08lld rms:%08lld ", level, sample_accumulator[indx], RMS_MAP[level]);
				break;
			}
		}
	}
}

// 16
#define MASK_OFF_LSB_4 ((~0)^0xf)
// 32
#define MASK_OFF_LSB_5 ((~0)^0x1f)
// 64
#define MASK_OFF_LSB_6 ((~0)^0x3f)
// 128
#define MASK_OFF_LSB_7 ((~0)^0x7f)
// 256
#define MASK_OFF_LSB_8 ((~0)^0xff)

int _visualizer_vumeter_div256_squared(int* levels) {
	long long sample_accumulator[2];
	int16_t *ptr;
	s16_t sample;
	s32_t sample_sq;
	size_t i, num_samples, samples_until_wrap;

	int offs;

	num_samples = VUMETER_DEFAULT_SAMPLE_WINDOW;

	sample_accumulator[0] = 0;
	sample_accumulator[1] = 0;

	vis_check();

	if (vis_get_playing()) {

		vis_lock();

		offs = vis_get_buffer_idx() - (num_samples * 2);
		while (offs < 0) offs += vis_get_buffer_len();

		ptr = vis_get_buffer() + offs;
		samples_until_wrap = vis_get_buffer_len() - offs;

		for (i=0; i<num_samples; i++) {
			sample = (*ptr++) >> 8;
			sample_sq = sample * sample;
			sample_accumulator[0] += sample_sq;

			sample = (*ptr++) >> 8;
			sample_sq = sample * sample;
			sample_accumulator[1] += sample_sq;

			samples_until_wrap -= 2;
			if (samples_until_wrap <= 0) {
				ptr = vis_get_buffer();
				samples_until_wrap = vis_get_buffer_len();
			}
		}

		vis_unlock();
	}

	_digitise(levels, sample_accumulator, num_samples);

	return 1;
}


static inline bool tenpc_delta(long long a, long long b) {
	long long d = llabs(a - b);
	return d > a /10;
}

int _visualizer_vumeter_cp(int* levels) {
static int16_t buff[VUMETER_DEFAULT_SAMPLE_WINDOW*2];
static int same_count =0;
static bool vc_displayed = false;
static long long prev_sq_summed[2] = { 0, 0};

	long long div256Sq_accumulator[2] = {0,0};
	long long summed_accumulator[2] = {0,0};
	float f_sq_summed_accumulator[2] = {0.0, 0.0};
	long long sq_summed_accumulator[2] = {0.0, 0.0};
	size_t num_samples;

	num_samples = VUMETER_DEFAULT_SAMPLE_WINDOW;
	size_t tot_samples = num_samples*2;

	vis_check();

	if (vis_get_playing()) {
		vis_lock();
		int offs;

		offs = vis_get_buffer_idx() - tot_samples;
		while (offs < 0) offs += vis_get_buffer_len();

		size_t ns1 = MIN(vis_get_buffer_len() - offs, tot_samples);
		size_t ns2 = tot_samples - ns1;
		int16_t *ptr = vis_get_buffer() + offs;
		memcpy(buff, ptr, sizeof(*ptr)*ns1);
		if (ns2) {
			memcpy(buff + ns1,
				   vis_get_buffer(),
				   sizeof(*ptr)*ns2);
		}
		vis_unlock();

		s16_t sample;
		s32_t sample_sq;

		ptr = buff;
		for (int i=0; i<num_samples; i++) {
			float f = abs(*ptr);
			f *=f;
			f_sq_summed_accumulator[0] += f;
			summed_accumulator[0] += abs(*ptr);
			sample = (*ptr++) >> 8;
			sample_sq = sample * sample;
			div256Sq_accumulator[0] += sample_sq;

			f = abs(*ptr);
			f *=f;
			f_sq_summed_accumulator[1] += f;
			summed_accumulator[1] += abs(*ptr);
			sample = (*ptr++) >> 8;
			sample_sq = sample * sample;
			div256Sq_accumulator[1] += sample_sq;
		}
	}
	summed_accumulator[0] /= num_samples;
	summed_accumulator[1] /= num_samples;
	sq_summed_accumulator[0] = sqrt(f_sq_summed_accumulator[0]/num_samples);
	sq_summed_accumulator[1] = sqrt(f_sq_summed_accumulator[1]/num_samples);

//	if (prev_sq_summed[0] != sq_summed_accumulator[0] || prev_sq_summed[1] != sq_summed_accumulator[1]) {
	if (tenpc_delta(prev_sq_summed[0],sq_summed_accumulator[0]) || tenpc_delta(prev_sq_summed[1],sq_summed_accumulator[1])) {
		same_count = 0;
		vc_displayed = false;
	} else {
		++same_count;
		if (same_count >= 120 && !vc_displayed) {
			vc_displayed = true;
			vol_calib_printf("Summed:%lld %lld Div256Sq:%lld %lld SqSummed:%lld %lld\n",
					summed_accumulator[0], summed_accumulator[1],
					div256Sq_accumulator[0]/num_samples, div256Sq_accumulator[1]/num_samples,
					(long long)sq_summed_accumulator[0], (long long)sq_summed_accumulator[1]
					);
		}
	}
	prev_sq_summed[0] = sq_summed_accumulator[0]; 
	prev_sq_summed[1] = sq_summed_accumulator[1]; 
	_digitise(levels, div256Sq_accumulator, num_samples);
	return 1;
}

/*
static volatile bool once = true;

int visualizer_vumeter(int* levels) {
	if (once) {
		printf("sizeof(long long)=%ld, sizeof(s16_t)=%ld, sizeof(int64_t)=%ld\n",
				(long)sizeof(long long), (long)sizeof(s16_t), (long)sizeof(int64_t));
		once = false;
	}
	return _visualizer_vumeter_cp(levels);
}
*/

// ==== volume levels {
// @60 FPS 30 => 1/2 a second
static int peak_hold_counter_init_value = 30;
// fine tune decay behaviour - default is 0 so decay immediately
// @60 FPS 4 appears to be a reasonable value.
static int decay_hold_counter_init_value = 3;

void update_volume_levels(runtime_volume_ptr vol_runtimes, float decay_unit) {
    int vols[2];
    // FIXME: read directly into runtime volumes
    _visualizer_vumeter_cp(vols);

    vol_runtimes[0].vol = vols[0];
    vol_runtimes[1].vol = vols[1];

    for (int ix_chan=0; ix_chan < NUM_VU_CHANNELS; ++ix_chan) {
        vol_runtimes[ix_chan].vol = vols[ix_chan];
        if (vol_runtimes[ix_chan].vol >= vol_runtimes[ix_chan].peak_hold_vol) {
//            vol_runtimes[ix_chan].eak_hold_counter = peak_hold_counter_start;
            vol_runtimes[ix_chan].peak_hold_counter = peak_hold_counter_init_value;
            vol_runtimes[ix_chan].peak_hold_vol = vol_runtimes[ix_chan].vol;
        }
        if (--vol_runtimes[ix_chan].peak_hold_counter < 0) {
            vol_runtimes[ix_chan].peak_hold_vol = 0;
            vol_runtimes[ix_chan].peak_hold_counter = 0;
        }
        if (vol_runtimes[ix_chan].vol >= vol_runtimes[ix_chan].decay_vol) {
            vol_runtimes[ix_chan].decay_vol = vol_runtimes[ix_chan].vol;
            vol_runtimes[ix_chan].decay_hold_counter = decay_hold_counter_init_value;
        } else {
            if (--vol_runtimes[ix_chan].decay_hold_counter < 0) {
                vol_runtimes[ix_chan].decay_vol -= decay_unit;
                vol_runtimes[ix_chan].decay_hold_counter = 0;
            }
        }
    }
}

int vumeter_set_peak_hold(int v) {
    int retv = peak_hold_counter_init_value;
    peak_hold_counter_init_value = v;
    return retv;
}

int vumeter_set_decay_hold(int v) {
    int retv = decay_hold_counter_init_value;
    decay_hold_counter_init_value = v;
    return retv;
}

// ==== volume levels }

