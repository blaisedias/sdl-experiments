// Do not edit: this file is generated
#include <string.h>

#include "vumeter_enum.h"

static const char* layout_arrangement_strings [] = {
    "none",
    "horizontal",
    "vertical",
};


static const char* component_render_op_strings [] = {
    "static",
    "single",
    "aggregate",
    "aggregate-off",
};


static const char* component_volume_type_strings [] = {
    "sampled",
    "peak-hold+sampled",
    "decay",
    "peak-hold+decay",
};


#define ARRAYLEN(a) sizeof((a))/sizeof((a)[0])


bool is_string_layout_arrangement(const char* s) {
    bool found = false;
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(layout_arrangement_strings) ; ++ix) {
            if (0 == strcmp(s, layout_arrangement_strings[ix])) {
                return true;
            }
        }
    }
    return found;
}

layout_arrangement_t layout_arrangement_from_string(const char* s, layout_arrangement_t defv) {
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(layout_arrangement_strings) ; ++ix) {
            if (0 == strcmp(s, layout_arrangement_strings[ix])) {
                return ix;
            }
        }
    }
    return defv;
}

const char* string_from_layout_arrangement(layout_arrangement_t v) {
    for (int ix = 0; ix < ARRAYLEN(layout_arrangement_strings) ; ++ix) {
        if (v == ix) {
            return layout_arrangement_strings[ix];
        }
    }
    return NULL;
}


bool is_string_component_render_op(const char* s) {
    bool found = false;
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(component_render_op_strings) ; ++ix) {
            if (0 == strcmp(s, component_render_op_strings[ix])) {
                return true;
            }
        }
    }
    return found;
}

component_render_op_t component_render_op_from_string(const char* s, component_render_op_t defv) {
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(component_render_op_strings) ; ++ix) {
            if (0 == strcmp(s, component_render_op_strings[ix])) {
                return ix;
            }
        }
    }
    return defv;
}

const char* string_from_component_render_op(component_render_op_t v) {
    for (int ix = 0; ix < ARRAYLEN(component_render_op_strings) ; ++ix) {
        if (v == ix) {
            return component_render_op_strings[ix];
        }
    }
    return NULL;
}


bool is_string_component_volume_type(const char* s) {
    bool found = false;
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(component_volume_type_strings) ; ++ix) {
            if (0 == strcmp(s, component_volume_type_strings[ix])) {
                return true;
            }
        }
    }
    return found;
}

component_volume_type_t component_volume_type_from_string(const char* s, component_volume_type_t defv) {
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(component_volume_type_strings) ; ++ix) {
            if (0 == strcmp(s, component_volume_type_strings[ix])) {
                return ix;
            }
        }
    }
    return defv;
}

const char* string_from_component_volume_type(component_volume_type_t v) {
    for (int ix = 0; ix < ARRAYLEN(component_volume_type_strings) ; ++ix) {
        if (v == ix) {
            return component_volume_type_strings[ix];
        }
    }
    return NULL;
}

