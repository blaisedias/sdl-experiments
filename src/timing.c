#define _XOPEN_SOURCE 600
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <time.h>
#include "timing.h"
#include "conversion.h"

int64_t get_micro_seconds() {
    int64_t millis;
    struct timespec  ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) == -1) {
        perror("clock_gettime");
        exit(EXIT_FAILURE);
    }
    millis = (ts.tv_sec*1000000);
    millis += (ts.tv_nsec/1000);
    return millis;
}

int64_t get_milli_seconds() {
    return get_micro_seconds()/1000;
}

//TODO: change formal parameter to long or unsigned long
void sleep_milli_seconds(long millis) {
    if (millis < 0) {
        return;
    }
    struct timespec ts = {.tv_sec =millis/1000, .tv_nsec = 1000000*(millis%1000)};
    nanosleep(&ts, NULL);
}

//TODO: change formal parameter to long or unsigned long
void sleep_micro_seconds(long micros) {
    if (micros < 0) {
        return;
    }
    struct timespec ts = {.tv_sec =micros/1000000, .tv_nsec = 1000*(micros%1000000)};
    nanosleep(&ts, NULL);
}
