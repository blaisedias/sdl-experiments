// Do not edit: this file is generated
#ifndef __vumeter_enum__
#include "types.h"

typedef enum {
    NO_ARRANGEMENT,
    HORIZONTAL_ARRANGEMENT,
    VERTICAL_ARRANGEMENT,
} channel_arrangement_t;


typedef enum {
    STATIC,
    SINGLE,
    AGGREGATE,
    AGGREGATEOFF,
} composition_render_op_t;


typedef enum {
    SAMPLED,
    PEAK_HOLD_AND_SAMPLED,
    DECAY,
    PEAK_HOLD_AND_DECAY,
    NONE,
} composition_volume_type_t;


typedef enum {
    FLIP_NONE,
    FLIP_HORIZONTAL,
    FLIP_VERTICAL,
    FLIP_HORIZONTAL_VERTICAL,
} flip_t;


bool is_string_channel_arrangement(const char* s);
channel_arrangement_t channel_arrangement_from_string(const char* s, channel_arrangement_t defv);
const char* string_from_channel_arrangement(channel_arrangement_t v);


bool is_string_composition_render_op(const char* s);
composition_render_op_t composition_render_op_from_string(const char* s, composition_render_op_t defv);
const char* string_from_composition_render_op(composition_render_op_t v);


bool is_string_composition_volume_type(const char* s);
composition_volume_type_t composition_volume_type_from_string(const char* s, composition_volume_type_t defv);
const char* string_from_composition_volume_type(composition_volume_type_t v);


bool is_string_flip(const char* s);
flip_t flip_from_string(const char* s, flip_t defv);
const char* string_from_flip(flip_t v);

#endif // __vumeter_enum__
