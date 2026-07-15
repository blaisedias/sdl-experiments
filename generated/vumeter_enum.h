// Do not edit: this file is generated
#ifndef __vumeter_enum__
#include "types.h"

typedef enum {
    NO_ARRANGEMENT,
    HORIZONTAL_ARRANGEMENT,
    VERTICAL_ARRANGEMENT,
} layout_arrangement_t;


typedef enum {
    STATIC,
    SINGLE,
    AGGREGATE,
    AGGREGATEOFF,
} component_render_op_t;


typedef enum {
    SAMPLED,
    PEAK_HOLD_AND_SAMPLED,
    DECAY,
    PEAK_HOLD_AND_DECAY,
} component_volume_type_t;


bool is_string_layout_arrangement(const char* s);
layout_arrangement_t layout_arrangement_from_string(const char* s, layout_arrangement_t defv);
const char* string_from_layout_arrangement(layout_arrangement_t v);


bool is_string_component_render_op(const char* s);
component_render_op_t component_render_op_from_string(const char* s, component_render_op_t defv);
const char* string_from_component_render_op(component_render_op_t v);


bool is_string_component_volume_type(const char* s);
component_volume_type_t component_volume_type_from_string(const char* s, component_volume_type_t defv);
const char* string_from_component_volume_type(component_volume_type_t v);

#endif // __vumeter_enum__
