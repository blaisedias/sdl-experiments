#ifndef __jl_timing_h_
#define __jl_timing_h_
#include <stdint.h>

int64_t get_micro_seconds(void);
int64_t get_milli_seconds(void);
void sleep_milli_seconds(long millis);
void sleep_micro_seconds(long micros);
#endif // __jl_timing_h_
