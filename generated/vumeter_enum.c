// Do not edit: this file is generated
#include <string.h>

#include "vumeter_enum.h"

static const char* channel_arrangement_strings [] = {
    "none",
    "horizontal",
    "vertical",
};


static const char* composition_render_op_strings [] = {
    "static",
    "single",
    "aggregate",
    "aggregate-off",
};


static const char* composition_volume_type_strings [] = {
    "sampled",
    "peak-hold+sampled",
    "decay",
    "peak-hold+decay",
    "none",
};


#define ARRAYLEN(a) sizeof((a))/sizeof((a)[0])


bool is_string_channel_arrangement(const char* s) {
    bool found = false;
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(channel_arrangement_strings) ; ++ix) {
            if (0 == strcmp(s, channel_arrangement_strings[ix])) {
                return true;
            }
        }
    }
    return found;
}

channel_arrangement_t channel_arrangement_from_string(const char* s, channel_arrangement_t defv) {
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(channel_arrangement_strings) ; ++ix) {
            if (0 == strcmp(s, channel_arrangement_strings[ix])) {
                return ix;
            }
        }
    }
    return defv;
}

const char* string_from_channel_arrangement(channel_arrangement_t v) {
    for (int ix = 0; ix < ARRAYLEN(channel_arrangement_strings) ; ++ix) {
        if (v == ix) {
            return channel_arrangement_strings[ix];
        }
    }
    return NULL;
}


bool is_string_composition_render_op(const char* s) {
    bool found = false;
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(composition_render_op_strings) ; ++ix) {
            if (0 == strcmp(s, composition_render_op_strings[ix])) {
                return true;
            }
        }
    }
    return found;
}

composition_render_op_t composition_render_op_from_string(const char* s, composition_render_op_t defv) {
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(composition_render_op_strings) ; ++ix) {
            if (0 == strcmp(s, composition_render_op_strings[ix])) {
                return ix;
            }
        }
    }
    return defv;
}

const char* string_from_composition_render_op(composition_render_op_t v) {
    for (int ix = 0; ix < ARRAYLEN(composition_render_op_strings) ; ++ix) {
        if (v == ix) {
            return composition_render_op_strings[ix];
        }
    }
    return NULL;
}


bool is_string_composition_volume_type(const char* s) {
    bool found = false;
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(composition_volume_type_strings) ; ++ix) {
            if (0 == strcmp(s, composition_volume_type_strings[ix])) {
                return true;
            }
        }
    }
    return found;
}

composition_volume_type_t composition_volume_type_from_string(const char* s, composition_volume_type_t defv) {
    if (NULL != s) {
        for (int ix = 0; ix < ARRAYLEN(composition_volume_type_strings) ; ++ix) {
            if (0 == strcmp(s, composition_volume_type_strings[ix])) {
                return ix;
            }
        }
    }
    return defv;
}

const char* string_from_composition_volume_type(composition_volume_type_t v) {
    for (int ix = 0; ix < ARRAYLEN(composition_volume_type_strings) ; ++ix) {
        if (v == ix) {
            return composition_volume_type_strings[ix];
        }
    }
    return NULL;
}

