/*
** Copyright 2025 Blaise Dias. All Rights Reserved.
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/


#ifndef  __jl_logging_h_
#define __jl_logging_h_
#include "types.h"

void error_printf(const char *format, ...);
void dummy_printf(const char *format, ...);

extern void (*log_printf)(const char *format, ...);
extern void (*vol_printf)(const char *format, ...);
extern void (*perf_printf)(const char *format, ...);
extern void (*load_printf)(const char *format, ...);
extern void (*scale_printf)(const char *format, ...);
extern void (*input_printf)(const char *format, ...);
extern void (*debug_printf)(const char *format, ...);
extern void (*tcache_printf)(const char *format, ...);
extern void (*tcache_eject_printf)(const char *format, ...);
extern void (*profile_printf)(const char *format, ...);
extern void (*profile_texture_printf)(const char *format, ...);
extern void (*json_printf)(const char *format, ...);
extern void (*action_printf)(const char *format, ...);
extern void (*app_printf)(const char *format, ...);
extern void (*vol_calib_printf)(const char *format, ...);

typedef enum {
    DEBUG_PRINTF,
    VOL_PRINTF,
    PERF_PRINTF,
    LOAD_PRINTF,
    SCALE_PRINTF,
    INPUT_PRINTF,
    TEXTURE_CACHE_PRINTF,
    PROFILE_PERF_PRINTF,
    PROFILE_TEXTURE_PERF_PRINTF,
    JSON_PRINTF,
    ACTION_PRINTF,
    TEXTURE_CACHE_EJECT_PRINTF,
    APP_PRINTF,
    VOL_CALIB_PRINTF,
}vu_printf_typ;

void enable_printf(vu_printf_typ v);
void disable_printf(vu_printf_typ v);

bool set_printf_onoff(vu_printf_typ v, bool on);

#endif // __jl_loggging_h_
