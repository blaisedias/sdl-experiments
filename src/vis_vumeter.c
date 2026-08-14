/*
** Copyright 2010 Logitech. All Rights Reserved.
** Copyright 2025 Blaise Dias
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/


#include "types.h"
#include "visualizer.h"

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
static uint32_t  sampled_rate;

void _digitise(int* levels, long long* sample_accumulator, size_t num_samples) {
	sample_accumulator[0] /= num_samples;
	sample_accumulator[1] /= num_samples;

/*
static long long prev_acc[2] = { 0, 0};
	if ((prev_acc[0]/10) != (sample_accumulator[0]/10) || (prev_acc[1]/10) != (sample_accumulator[1]/10)) {
		prev_acc[0] = sample_accumulator[0];
		prev_acc[1] = sample_accumulator[1];
		vol_calib_printf("%08lld %08lld\n", prev_acc[0], prev_acc[1]);

	}
*/
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
		uint32_t rate = vis_get_rate();
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
		if (rate != sampled_rate) {
			sampled_rate = rate;
			log_printf("sample rate = %d\n");
		}
	}

	_digitise(levels, sample_accumulator, num_samples);

	return 1;
}

int _visualizer_vumeter_cp(int* levels) {
static int16_t buff[VUMETER_DEFAULT_SAMPLE_WINDOW*2];
static long long prev_summed[2] = { 0, 0};
static int same_count =0;
static bool vc_displayed = false;

	long long div256Sq_accumulator[2] = {0,0};
	long long summed_accumulator[2] = {0,0};
	size_t num_samples, ns1, ns2;

	num_samples = VUMETER_DEFAULT_SAMPLE_WINDOW;
	size_t tot_samples = num_samples*2;

	vis_check();

	if (vis_get_playing()) {
		vis_lock();
		int offs;

		offs = vis_get_buffer_idx() - tot_samples;
		while (offs < 0) offs += vis_get_buffer_len();

		ns1 = MIN(vis_get_buffer_len() - offs, tot_samples);
		ns2 = tot_samples - ns1;
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
			summed_accumulator[0] += abs(*ptr);
			sample = (*ptr++) >> 8;
			sample_sq = sample * sample;
			div256Sq_accumulator[0] += sample_sq;

			summed_accumulator[1] += abs(*ptr);
			sample = (*ptr++) >> 8;
			sample_sq = sample * sample;
			div256Sq_accumulator[1] += sample_sq;
		}
	}
	summed_accumulator[0] /= num_samples;
	summed_accumulator[1] /= num_samples;
/*
	if ((prev_summed[0]/10) != (summed_accumulator[0]/10) || (prev_summed[1]/10) != (summed_accumulator[1]/10)) {
		prev_summed[0] = summed_accumulator[0];
		prev_summed[1] = summed_accumulator[1];
		vol_calib_printf("S:%lld %lld\n", prev_summed[0], prev_summed[1]);
	}
*/
	if ((prev_summed[0]/10) != (summed_accumulator[0]/10) || (prev_summed[1]/10) != (summed_accumulator[1]/10)) {
		same_count = 0;
		vc_displayed = false;
		prev_summed[0] = summed_accumulator[0];
		prev_summed[1] = summed_accumulator[1];
	} else {
		++same_count;
		if (same_count >= 60 && !vc_displayed) {
			vc_displayed = true;
			vol_calib_printf("Summed:%lld %lld Div256Sq:%lld %lld\n",
					prev_summed[0], prev_summed[1],
					div256Sq_accumulator[0]/num_samples, div256Sq_accumulator[1]/num_samples);
		}
	}
	_digitise(levels, div256Sq_accumulator, num_samples);
	return 1;
}

static volatile bool once = true;

int visualizer_vumeter(int* levels) {
	if (once) {
		printf("sizeof(long long)=%ld, sizeof(s16_t)=%ld, sizeof(int64_t)=%ld\n",
				(long)sizeof(long long), (long)sizeof(s16_t), (long)sizeof(int64_t));
		once = false;
	}
	return _visualizer_vumeter_cp(levels);
}

