/*
** Copyright 2010 Logitech. All Rights Reserved.
** Copyright 2025 Blaise Dias
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/


#include "types.h"
#include "visualizer.h"
#include "audio_volume.h"
#include "platform.h"
#include "logging.h"
#include "conversion.h"

#define SAMPLE_COUNT   (1024 * 2)
#define TOT_SAMPLES     (SAMPLE_COUNT*2)

static inline int avg_ll(long long accumulator, uint32_t samples) {
	return (int)(accumulator/llong_from_uint32_t(samples));
}

static inline int sqrt_avg(float accumulator, uint32_t samples) {
	return (int)sqrt((float)accumulator/(float)samples);
}


static long long RMS_MAP[] = {
	   0,    2,    5,    7,   10,   21,   33,   45,   57,   82,
	 108,  133,  159,  200,  242,  284,  326,  387,  448,  509,
	 570,  652,  735,  817,  900, 1005, 1111, 1217, 1323, 1454,
	1585, 1716, 1847, 2005, 2163, 2321, 2480, 2666, 2853, 3040,
	3227, 3414, 3601, 3788, 3975, 4162, 4349, 4536, 4755, 5000,
};

static runtime_volume_t vol_runtimes[2];

static void legacy_digitise(void) {
	for(int indx =0; indx < 2; ++indx) {
		vol_runtimes[indx].vol = 0;
		for (int level = 48; level >=0; --level) {
			if (vol_runtimes[indx].div256Sq > RMS_MAP[level]) {
				vol_runtimes[indx].vol = level;
				vol_printf("%02d a:%08lld rms:%08lld ",
						level, vol_runtimes[indx].div256Sq, RMS_MAP[level]);
				break;
			}
		}
	}
}

// 8
#define MASK_OFF_LSB_3 ((~0)^0x7)
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

#if 0
static int _visualizer_vumeter_div256_squared(void) {
	long long sample_accumulator[2];
	int16_t *ptr;
	s16_t sample;
	s32_t sample_sq;
	size_t i, num_samples, samples_until_wrap;

	int offs;

	num_samples = SAMPLE_COUNT;

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

	vol_runtimes[0].div256Sq = sample_accumulator[0]/num_samples;
	vol_runtimes[1].div256Sq = sample_accumulator[1]/num_samples;
	legacy_digitise();

	return 1;
}
#endif

static inline bool tenpc_delta(long long a, long long b) {
	long long d = llabs(a - b);
	return d > a /10;
}

#if VOLUME_CALIB_LEVEL
extern FILE* fp_vol_calib;
#endif

#if 0
static int _visualizer_vumeter_cp(void) {
	static int16_t buff[TOT_SAMPLES];
#if  VOLUME_CALIB_LEVEL
	static int same_count =0;
	static bool vc_displayed = false;
	static long long prev_sq_summed[2] = { 0, 0};
#endif // VOLUME_CALIB_LEVEL

	long long div256Sq_accumulator[2] = {0,0};
	long long summed_accumulator[2] = {0,0};
	float f_sq_summed_accumulator[2] = {0.0, 0.0};
//	int32_t num_samples = SAMPLE_COUNT;
//	int32_t tot_samples = TOT_SAMPLES;

	vis_check();

	if (vis_get_playing()) {
		vis_lock();
		int32_t offs;
		int32_t vis_buffer_len = (int32_t)vis_get_buffer_len();
		int32_t  vis_buffer_idx = (int32_t)vis_get_buffer_idx();

		if ( TOT_SAMPLES > (uint32_t)vis_buffer_len ) {
			vis_unlock();
			error_printf("total samples %u  exceeds visualiser buffer length %u\n",
							TOT_SAMPLES , vis_buffer_len);
			exit(EXIT_FAILURE);
		}


		offs = vis_buffer_idx - TOT_SAMPLES;
		while (offs < 0) offs += vis_buffer_len;

		size_t ns1 = MIN((size_t)(vis_buffer_len - offs), TOT_SAMPLES);
		size_t ns2 = TOT_SAMPLES - ns1;
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
		for (int32_t i=0; i < SAMPLE_COUNT; i++) {
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
	vol_runtimes[0].div256Sq = avg_ll(div256Sq_accumulator[0], SAMPLE_COUNT);
	vol_runtimes[1].div256Sq = avg_ll(div256Sq_accumulator[1], SAMPLE_COUNT);
	vol_runtimes[0].summed = avg_ll(summed_accumulator[0], SAMPLE_COUNT);
	vol_runtimes[1].summed = avg_ll(summed_accumulator[1], SAMPLE_COUNT);
	vol_runtimes[0].sq_summed = sqrt_avg(f_sq_summed_accumulator[0], SAMPLE_COUNT);
	vol_runtimes[1].sq_summed = sqrt_avg(f_sq_summed_accumulator[1], SAMPLE_COUNT);

#if VOLUME_CALIB_LEVEL
//	if (prev_sq_summed[0] != sq_summed_accumulator[0] || prev_sq_summed[1] != sq_summed_accumulator[1]) {
	if (tenpc_delta(prev_sq_summed[0],vol_runtimes[0].sq_summed) || tenpc_delta(prev_sq_summed[1],vol_runtimes[1].sq_summed)) {
		same_count = 0;
		vc_displayed = false;
	} else {
		++same_count;
		if (same_count >= 120 && !vc_displayed) {
			vc_displayed = true;
			vol_calib_printf("Summed:%lld %lld Div256Sq:%lld %lld SqSummed:%lld %lld\n",
					summed_accumulator[0], summed_accumulator[1],
					div256Sq_accumulator[0]/SAMPLE_COUNT, div256Sq_accumulator[1]/SAMPLE_COUNT,
					(long long)vol_runtimes[0].sq_summed, (long long)vol_runtimes[1].sq_summed
					);
		}
	}
	prev_sq_summed[0] = vol_runtimes[0].sq_summed;
	prev_sq_summed[1] = vol_runtimes[1].sq_summed;
#endif // VOLUME_CALIB_LEVEL

	legacy_digitise();
	return 1;
}
#endif

static inline bool ignore_delta(int a, int b) {
	int d = abs(a - b);
	// 1%
	return d > a /100;
}

static int _visualizer_vumeter_cp2(void) {
	static int16_t buff[TOT_SAMPLES];
#if  VOLUME_CALIB_LEVEL
	static int same_count = 0;
	static bool vc_displayed = false;
	static int prev_sq_summed[120][2];
#endif // VOLUME_CALIB_LEVEL

	long long div256Sq_accumulator[2] = {0, 0};
	float f_sq_summed_accumulator[2] = {0.0, 0.0};
//	int32_t num_samples = SAMPLE_COUNT;
//	int32_t tot_samples = TOT_SAMPLES;

	vis_check();

	if (vis_get_playing()) {
		vis_lock();
		int32_t offs;
		int32_t vis_buffer_len = (int32_t)vis_get_buffer_len();
		int32_t  vis_buffer_idx = (int32_t)vis_get_buffer_idx();

		if ( TOT_SAMPLES > (uint32_t)vis_buffer_len ) {
			vis_unlock();
			error_printf("total samples %u  exceeds visualiser buffer length %u\n",
							TOT_SAMPLES , vis_buffer_len);
			exit(EXIT_FAILURE);
		}


		offs = vis_buffer_idx - TOT_SAMPLES;
		while (offs < 0) offs += vis_buffer_len;

		size_t ns1 = MIN((size_t)(vis_buffer_len - offs), TOT_SAMPLES);
		size_t ns2 = TOT_SAMPLES - ns1;
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
		for (int32_t i=0; i < SAMPLE_COUNT; i++) {
			sample_sq = (*ptr);
			sample_sq *= sample_sq;
			f_sq_summed_accumulator[0] += (float)sample_sq;
			sample = (*ptr) >> 8;
			sample_sq = sample * sample;
			div256Sq_accumulator[0] += sample_sq;
			ptr++;

			sample_sq = (*ptr);
			sample_sq *= sample_sq;
			f_sq_summed_accumulator[1] += (float)sample_sq;
			sample = (*ptr) >> 8;
			sample_sq = sample * sample;
			div256Sq_accumulator[1] += sample_sq;
			ptr++;
		}
	}
	vol_runtimes[0].div256Sq = avg_ll(div256Sq_accumulator[0], SAMPLE_COUNT);
	vol_runtimes[1].div256Sq = avg_ll(div256Sq_accumulator[1], SAMPLE_COUNT);
	vol_runtimes[0].sq_summed = sqrt_avg(f_sq_summed_accumulator[0], SAMPLE_COUNT);
	vol_runtimes[1].sq_summed = sqrt_avg(f_sq_summed_accumulator[1], SAMPLE_COUNT);

#if VOLUME_CALIB_LEVEL
//	if (prev_sq_summed[0] != sq_summed_accumulator[0] || prev_sq_summed[1] != sq_summed_accumulator[1]) {
	if (ignore_delta(prev_sq_summed[same_count][0],vol_runtimes[0].sq_summed) || ignore_delta(prev_sq_summed[same_count][1],vol_runtimes[1].sq_summed)) {
		same_count = 0;
		vc_displayed = false;
	} else {
		++same_count;
		if (same_count >= 120 && !vc_displayed) {
			long long sq_summed[2] = {0, 0};
			for(int x = 0; x < 120; x++) {
				sq_summed[0] += prev_sq_summed[x][0];
				sq_summed[1] += prev_sq_summed[x][1];
				if (fp_vol_calib && prev_sq_summed[x][0] && prev_sq_summed[x][1]) {
					fprintf(fp_vol_calib,"V:%d, %d\n", prev_sq_summed[x][0], prev_sq_summed[x][1]);
				}
			}
//			vc_displayed = true;
			vol_calib_printf("Div256Sq:%5d %5d SqSummed:%6d %6d\n",
					vol_runtimes[0].div256Sq, vol_runtimes[1].div256Sq,
					(int)(sq_summed[0]/120), (int)(sq_summed[1]/120)
					);
			same_count = 0;
		}
	}
	prev_sq_summed[same_count][0] = vol_runtimes[0].sq_summed;
	prev_sq_summed[same_count][1] = vol_runtimes[1].sq_summed;
#endif // VOLUME_CALIB_LEVEL

	legacy_digitise();
	return 1;
}



// ==== volume levels {
// @60 FPS 30 => 1/2 a second
static int peak_hold_counter_init_value = 30;
// fine tune decay behaviour - default is 0 so decay immediately
// @60 FPS 4 appears to be a reasonable value.
static int decay_hold_counter_init_value = 3;

void update_volume_levels(float decay_unit) {
	_visualizer_vumeter_cp2();

	for (int ix_chan=0; ix_chan < NUM_VU_CHANNELS; ++ix_chan) {
		if (vol_runtimes[ix_chan].vol >= vol_runtimes[ix_chan].peak_hold_vol) {
			vol_runtimes[ix_chan].peak_hold_counter = peak_hold_counter_init_value;
			vol_runtimes[ix_chan].peak_hold_vol = vol_runtimes[ix_chan].vol;
		}
		if (--vol_runtimes[ix_chan].peak_hold_counter < 0) {
			vol_runtimes[ix_chan].peak_hold_vol = 0;
			vol_runtimes[ix_chan].peak_hold_counter = 0;
		}
		if (vol_runtimes[ix_chan].vol >= vol_runtimes[ix_chan].decay_vol) {
			vol_runtimes[ix_chan].decay_vol = (float)vol_runtimes[ix_chan].vol;
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

runtime_volume_ptr get_runtime_volume() {
	return vol_runtimes;
}
// ==== volume levels }

