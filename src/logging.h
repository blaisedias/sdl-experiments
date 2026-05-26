/*
** Copyright 2025 Blaise Dias. All Rights Reserved.
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/


#ifndef  __jl_logging_h_
#define __jl_logging_h_
#include "types.h"

extern void error_printf(char *format, ...);
extern void dummy_printf(char *format, ...);

extern void (*vol_printf)(char *format, ...);
extern void (*perf_printf)(char *format, ...);
extern void (*load_printf)(char *format, ...);
extern void (*scale_printf)(char *format, ...);
extern void (*input_printf)(char *format, ...);
extern void (*debug_printf)(char *format, ...);
extern void (*tcache_printf)(char *format, ...);
extern void (*tcache_eject_printf)(char *format, ...);
extern void (*profile_printf)(char *format, ...);
extern void (*profile_texture_printf)(char *format, ...);
extern void (*json_printf)(char *format, ...);
extern void (*action_printf)(char *format, ...);
extern void (*app_printf)(char *format, ...);

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
}vu_printf_typ;

void enable_printf(vu_printf_typ v);
void disable_printf(vu_printf_typ v);

bool set_printf_onoff(vu_printf_typ v, bool on);

#endif // __jl_loggging_h_
