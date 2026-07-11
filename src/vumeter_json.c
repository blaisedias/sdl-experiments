#include "json.h"

#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <SDL2/SDL.h>
#include "application.h"
#include "logging.h"
#include "util.h"
#define DEBUG_VUMETER_JSON
#include "vumeterdef.h"

#define bool  SDL_bool

static void printf_rect(const SDL_Rect* rect) {
    printf("x=%4d, y=%4d, w=%4d, h=%4d", rect->x, rect->y, rect->w, rect->x);
}

static void printf_point(const SDL_Point* point) {
    printf("x=%4d, y=%4d", point->x, point->y);
}


static void dump_vu(const vumeter_properties_t* vu) {
#ifdef DEBUG_VUMETER_JSON
    printf("kind:%s, vutype=%s, format=%s\n",
            vu->kind, vu->vutype, vu->format);
#endif
    printf("format version:%d\n", vu->format_version);
    printf("layout:\n    w=%d h=%d arrangement=%s\n",
            vu->layout.w,
            vu->layout.h,
            string_from_layout_arrangement(vu->layout.arrangement)
            );
    printf("    rects:\n");
    for(int ix = 0; ix < sizeof(vu->layout.rects)/sizeof(vu->layout.rects[0]); ++ix) {
        printf("        ");
        printf_rect(vu->layout.rects+ix);
        printf("\n");
    }
    printf("resources:\n    count=%d\n    names:\n", vu->resources.count);
    for(int ix =0; ix < vu->resources.count; ++ix) {
        printf("        %s\n", vu->resources.names[ix]);
    }
    printf("placements:\n    count=%d\n    names:\n", vu->placements.count);
    for(int ix =0; ix < vu->placements.count; ++ix) {
        printf("        %03d) rect=", ix);
        printf_rect(&vu->placements.elements[ix].rect);
        printf(", center=");
        printf_point(&vu->placements.elements[ix].center);
        printf(", flip=%d ",
                vu->placements.elements[ix].flip);
        printf(", angle=%06.2f",
                vu->placements.elements[ix].angle);
        printf(", texture_index=%d",
                vu->placements.elements[ix].texture_index);
        printf("\n");
    }
}
static json_value* get_object_value(json_value* jvalue, const char* jt) {
    if (jvalue == NULL) {
        error_printf("get_object_value: value==NULL\n");
        return NULL;
    }
    if (jvalue->type != json_object) {
        error_printf("get_object_value: != object\n");
        return NULL;
    }
    for (int x=0; x < jvalue->u.object.length; x++) {
        if (0 == strcmp_ex((jvalue->u.object.values + x)->name, jt) ) {
            return jvalue->u.object.values[x].value;
        }
    }
    return NULL;
}

static json_value* get_object_object_value(json_value* jvalue, const char* jt) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type == json_object) {
        return jvalue;
    }
    return NULL;
}

static json_value* get_object_array_value(json_value* jvalue, const char* jt) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type == json_array) {
        return jvalue;
    }
    return NULL;
}


static int get_object_int_value(json_value* jvalue, const char* jt, int default_value) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type == json_integer) {
        return jvalue->u.integer;
    }
    return default_value;
}


static const char* get_object_string_value(json_value* jvalue, const char* jt, const char* default_value) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type == json_string) {
            return jvalue->u.string.ptr;
    }
    return default_value;
}

static bool get_object_boolean_value(json_value* jvalue, const char* jt, bool default_value) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type == json_boolean) {
            return (bool)jvalue->u.boolean;
    }
    return default_value;
}

static double get_object_double_value(json_value* jvalue, const char* jt, double default_value) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type == json_double) {
            return jvalue->u.dbl;
    }
    return default_value;
}

static float get_object_float_value(json_value* jvalue, const char* jt, float default_value) {
    return (float) get_object_double_value(jvalue, jt, (double) default_value);
}

static void deserialise_rect(json_value* jvalue, SDL_Rect* rect) {
    rect->x = get_object_int_value(jvalue, "x", 0);
    rect->y = get_object_int_value(jvalue, "y", 0);
    rect->w = get_object_int_value(jvalue, "w", 0);
    rect->h = get_object_int_value(jvalue, "h", 0);
}

static void deserialise_point(json_value* jvalue, SDL_Point* pt) {
    pt->x = get_object_int_value(jvalue, "x", 0);
    pt->y = get_object_int_value(jvalue, "y", 0);
}

static int deserialise_layout(json_value* jvalue, vumeter_properties_t* vu) {
    if (NULL == jvalue) {
        error_printf("deserialise_layout: got null object for layout\n");
        return -1;
    }
    vu->layout.w = get_object_int_value(jvalue, "w", 0);
    if ( 0 >= vu->layout.w ) {
        error_printf("deserialise_vumeter: invalid value of layout width %d\n", vu->layout.w);
        return -1;
    }
    vu->layout.h = get_object_int_value(jvalue, "h", 0);
    if ( 0 >= vu->layout.h) {
        error_printf("deserialise_vumeter: invalid value of layout height %d\n", vu->layout.h);
        return -1;
    }
    const char* str = get_object_string_value(jvalue, "arrangement", NULL);
    if (str && !is_string_layout_arrangement(str)) {
        error_printf("invalid value for layout arrangement %s\n", str);
    }
    vu->layout.arrangement = layout_arrangement_from_string(str, NO_ARRANGEMENT);
    json_value* rects_value = get_object_array_value(jvalue, "rects");
    if (rects_value) {
        int n_rects = sizeof(vu->layout.rects)/sizeof(vu->layout.rects[0]);
        if (rects_value->u.array.length != n_rects) {
            error_printf("number of layout rectangles != %d\n", n_rects);
        }
        for(int ix=0; ix < MIN(rects_value->u.array.length, n_rects); ++ix) {
            deserialise_rect(rects_value->u.array.values[ix], vu->layout.rects + ix);
        }
    } else {
        return -1;
    }
    return 0;
}

static int deserialise_resources(json_value* jresources, vumeter_properties_t* vu) {
    if (NULL == jresources) {
        error_printf("deserialise_resources: got null object for resources\n");
        return -1;
    }
    vu->resources.count = jresources->u.array.length;
    size_t bufflen = jresources->u.array.length;
    for(int ix=0; ix < jresources->u.array.length; ++ix) {
        json_value* jelem = jresources->u.array.values[ix];
        switch (jelem->type) {
            case json_null:
                break;
            case json_string:
                bufflen += jelem->u.string.length;
                break;
            default:
                error_printf("resources: not a string\n");
                return -1;
        }
    }
    bufflen += sizeof(char*) *  jresources->u.array.length;
    char *buffer  = calloc(bufflen, sizeof(char));
//    vu->resources.names = calloc(bufflen, sizeof(char));
    vu->resources.names = (char **)buffer;
    if (!vu->resources.names) {
        error_printf("OOM: resources strings %ld\n", bufflen);
        return -1;
    }
    char *p = (char*)(&vu->resources.names[vu->resources.count]);
    for(int ix=0; ix < jresources->u.array.length; ++ix) {
        json_value* jelem = jresources->u.array.values[ix];
        switch (jelem->type) {
            case json_null:
                vu->resources.names[ix] = NULL;
                break;
            case json_string:
                strcpy(p, jelem->u.string.ptr);
                vu->resources.names[ix] = p;
                p += 1 + jelem->u.string.length;
                break;
            default:
                error_printf("resources: not a string\n");
                return -1;
        }
    }
    vu->resources.textures = calloc(sizeof(vu->resources.textures[0]), vu->resources.count);
    return 0;
}

static int deserialise_placements(json_value* jplacements, vumeter_properties_t* vu) {
    if (NULL == jplacements) {
        error_printf("deserialise_placements: got null object for placements\n");
        return -1;
    }
    vu->placements.count = jplacements->u.array.length;
    vu->placements.elements = calloc(sizeof( vu->placements.elements[0]), vu->placements.count);
    for(int ix=0; ix < vu->placements.count; ++ix) {
        vu_placement_t* placement = &vu->placements.elements[ix];
        json_value* jelem = jplacements->u.array.values[ix];

        deserialise_rect(jelem, &placement->rect);
        json_value* jcenter = get_object_object_value(jelem, "center");
        if (jcenter) {
            deserialise_point(jcenter, &placement->center);
        }
        placement->flip = get_object_int_value(jelem, "flip", 0);
        placement->angle = get_object_float_value(jelem, "angle", 0);
        placement->texture_index = get_object_int_value(jelem, "resource", 0);
    }
    return 0;
}

static int deserialise_levels(json_value* jlevels, vumeter_properties_t* vu) {
    if (NULL == jlevels) {
        error_printf("deserialise_levels: got null object for levels\n");
        return -1;
    }
    
    return 0;
}

static int __json_deserialise_vumeter(json_value* jvalue, vumeter_properties_t* vu) {
    const char* str;
    str = get_object_string_value(jvalue, "kind", NULL);
    if (strcmp_ex("vumeter", str)) {
        error_printf("deserialise_vumeter: unsupported kind %s\n", str); 
    }
#ifdef DEBUG_VUMETER_JSON
    vu->kind = str;
#endif

    str = get_object_string_value(jvalue, "vutype", NULL);
    if (strcmp_ex("compose2", str)) {
        error_printf("deserialise_vumeter: unknown vutype %s\n", str); 
        return -1;
    }
#ifdef DEBUG_VUMETER_JSON
    vu->vutype = str;
#endif

    str = get_object_string_value(jvalue, "format", NULL);
    if (strcmp_ex("indexed", str)) {
        error_printf("deserialise_vumeter: unsupported format %s\n", str); 
        return -1;
    }
#ifdef DEBUG_VUMETER_JSON
    vu->format = str;
#endif

    vu->format_version = get_object_int_value(jvalue, "format_version", -1);
    // FIXME: 
    if (vu->format_version != 0) {
        error_printf("deserialise_vumeter: format version %d\n", vu->format_version);
        return -2;
    }

    vu->volume_levels = get_object_int_value(jvalue, "volume_levels", 0);
    if (vu->volume_levels <= 0) {
        error_printf("deserialise_vumeter: invalid volume levels %d\n", vu->volume_levels);
        return -2;
    }

    if (deserialise_layout(get_object_object_value(jvalue, "layout"), vu)) {
        return -1;
    }

    if (deserialise_resources(get_object_array_value(jvalue, "resources"), vu)) {
        return -1;
    }

    if (deserialise_placements(get_object_array_value(jvalue, "placement"), vu)) {
        return -1;
    }

    if (deserialise_levels(get_object_array_value(jvalue, "levels"), vu)) {
        return -1;
    }


    { // dummy
        SDL_Point pt;
        deserialise_point(jvalue, &pt);
        get_object_float_value(jvalue, "", 0);
        get_object_double_value(jvalue, "", 0);
        get_object_boolean_value(jvalue, "", 0);
    }
    dump_vu(vu);

    FREE(vu->resources.names);
    FREE(vu->resources.textures);
    FREE(vu->placements.elements);

    return 0;
}

int json_deserialise_vumeter(const char* json_string, size_t length) {
    vumeter_properties_t vu_instance;
    vumeter_properties_t* vu= &vu_instance;

    memset(&vu_instance, 0, sizeof(vu_instance));
    json_value* jvalue = json_parse(json_string, length);
    if (NULL == jvalue) {
        error_printf("deserialise_vumeter: failed to parse string\n");
        return -1;
    }
    int rv = __json_deserialise_vumeter(jvalue, vu);
    json_value_free(jvalue);
    return rv;

}

int json_deserialise_vumeters_file(const char* filepath) {
    FILE *fp;
    struct stat filestatus;
    char* json_string;

    if ( stat(filepath, &filestatus) != 0) {
        error_printf("deserialise_vumeters_file: file %s not found\n", filepath);
        return EXIT_FAILURE;
    }

    json_string =  calloc(filestatus.st_size, 1);
    if (json_string == NULL) {
        error_printf("deserialise_vumeters_file: OOM %d %s \n", filestatus.st_size, filepath);
        return EXIT_FAILURE;
    }

    fp = fopen(filepath, "rt");
    if (fp == NULL) {
        free(json_string);
        error_printf("deserialise_vumeters_file: failed to open file %s \n", filepath);
        return EXIT_FAILURE;
    }

    if (1 != fread(json_string, filestatus.st_size, 1, fp)) {
        fclose(fp);
        free(json_string);
        error_printf("deserialise_vumeters_file: failed to read file data %s \n", filepath);
        return EXIT_FAILURE;
    }

    fclose(fp);
    int rv = json_deserialise_vumeter(json_string, filestatus.st_size);
    free(json_string);

    if (rv != 0) {
        error_printf("deserialise_vumeters_file: failed to parse json file %s\n", filepath);
    }
    return rv;
}


int main(int argc, char** argv) {
    for(int ix=1; ix < argc; ++ix) {
        json_deserialise_vumeters_file(argv[ix]);
    }
    return 0;
}
