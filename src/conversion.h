#ifndef __jl_conversion_h
#define __jl_conversion_h
#include <assert.h>
#include <stdint.h>
#include <limits.h>
#include <stddef.h>

// implement checked conversion, NOT safe conversion.
// assert if the destination limits would be exceeded.
// assert if negative values are converted to unsigned.
// Motivation: fast fail on unexpected invalid conversions.

// to int {
static inline int int_from_unsigned(unsigned v) {
    assert(v <= INT_MAX);
    return (int)v;
}

static inline int int_from_long(long v) {
    assert(v <= INT_MAX);
    assert(v >= INT_MIN);
    return (int)v;
}

static inline int int_from_uint32_t(uint32_t v) {
    assert(v <= INT_MAX);
    return (int)v;
}

static inline int int_from_size_t(size_t v) {
    assert(v <= INT_MAX);
    return (int)v;
}

static inline int int_from_ssize_t(ssize_t v) {
     assert(v <= INT_MAX);
     assert(v >= INT_MIN);
     return (int)v;
}
// } to int

// to unsigned {
static inline unsigned unsigned_from_long(long v) {
    assert(v <= UINT_MAX && v >=0);
    return (unsigned)v;
}

static inline unsigned unsigned_from_int(int v) {
    assert(v >=0);
    return (unsigned)v;
}

static inline unsigned unsigned_from_size_t(size_t v) {
    assert(v <= UINT_MAX);
    return (unsigned)v;
}
// } to unsigned 

// to size_t {
static inline size_t size_t_from_int(int v) {
    assert(v >= 0);
    return (size_t)v;
}

static inline size_t size_t_from_uint32_t(uint32_t v) {
    // nothing to assert
    return (size_t)v;
}

static inline size_t size_t_from_int32_t(int32_t v) {
    assert(v >= 0);
    return (size_t)v;
}

static inline size_t size_t_from_unsigned_long(unsigned long v) {
    // size_t is unsigned, check limit by settings all bits
    assert(v <= (size_t)(~0));
    return (size_t)v;
}
// } to size_t

// to long long {
static inline long long llong_from_size_t(size_t v) {
    assert(sizeof(long long)/sizeof(v) >= 2);
    return (long long)v;
}

static inline long long llong_from_uint32_t(uint32_t v) {
    assert(sizeof(long long)/sizeof(v) >= 2);
    return (long long)v;
}
// } to long long

#endif
