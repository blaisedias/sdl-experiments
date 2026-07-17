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


void dump_vumeter(const vumeter_properties_t* vu) {
    if (vu == NULL) {
        return;
    }
    printf("resource path :%s\n", vu->resource_path);
#ifdef DEBUG_VUMETER_JSON
    printf("kind:%s, vutype=%s, format=%s\n",
            vu->kind, vu->vutype, vu->format);
#endif
    printf("format version:%d\n", vu->format_version);
    printf("layout:\n    w=%d h=%d arrangement=%d (%s)\n",
            vu->layout.w,
            vu->layout.h,
            vu->layout.arrangement,
            string_from_channel_arrangement(vu->layout.arrangement)
            );
    printf("  rects:\n");
    for(int ix = 0; ix < sizeof(vu->layout.rects)/sizeof(vu->layout.rects[0]); ++ix) {
        printf("    ");
        printf_rect(vu->layout.rects+ix);
        printf("\n");
    }
    printf("resources:\n    count=%d\n    filenames:\n", vu->resource_list.count);
    for(int ix =0; ix < vu->resource_list.count; ++ix) {
        printf("      %s\n", vu->resource_list.names[ix]);
    }
    printf("placements:\n    count=%d\n", vu->placement_list.count);
    for(int ix =0; ix < vu->placement_list.count; ++ix) {
        printf("    %03d) rect:  ", ix);
        printf_rect(&vu->placement_list.elements[ix].rect);
        printf(", center=");
        printf_point(&vu->placement_list.elements[ix].center);
        printf(", flip=%d ",
                vu->placement_list.elements[ix].flip);
        printf(", angle=%06.2f",
                vu->placement_list.elements[ix].angle);
        printf(", texture_index=%d",
                vu->placement_list.elements[ix].texture_index);
        printf("\n");
    }
    printf("compositions:\n    count=%d\n", vu->composition_list.count);
    for(int ix =0; ix < vu->composition_list.count; ++ix) {
        vu_composition_t* composition =  &vu->composition_list.compositions[ix];
        printf("    %d) composition: render_op=%d (%s) volume_type=%d (%s)\n",
                ix,
                composition->render_op, string_from_composition_render_op(composition->render_op),
                composition->volume_type, string_from_composition_volume_type(composition->volume_type)
                );
        printf("         placements: ");
        for(int ixp=0; ixp < composition->placement_count; ixp++) {
            printf("%d, ", composition->ix_placements[ixp]);
        }
        puts("");
    }
    printf("vumeters:\n    count=%d\n", vu->vumeter_list.count);
    for(int ix =0; ix < vu->vumeter_list.count; ++ix) {
        vumeter_t* vumeter = &vu->vumeter_list.vumeters[ix];
        printf("    %s:\n", vumeter->name);
        for (int ich=0; ich < vumeter->component_count; ++ich) {
            vu_component_t* component = &vumeter->components[ich];
            printf("        ");
            for (int icomp=0; icomp < component->composition_count; ++icomp) {
                printf("%d, ", component->ix_compositions[icomp]);
            }
            puts("");
        }
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
    const char* str = get_object_string_value(jvalue, "channel_arrangement", NULL);
    if (str && !is_string_channel_arrangement(str)) {
        error_printf("invalid value for layout arrangement %s\n", str);
    }
    vu->layout.arrangement = channel_arrangement_from_string(str, NO_ARRANGEMENT);
    json_value* rects_value = get_object_array_value(jvalue, "rectangles");
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

static int deserialiser_resource_list(json_value* jresources, vumeter_properties_t* vu) {
    if (NULL == jresources) {
        error_printf("deserialiser_resource_list: got null object for resources\n");
        return -1;
    }
    vu->resource_list.count = jresources->u.array.length;
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
    char *buffer;
    if (NULL == CALLOC(bufflen, buffer)) {
        error_printf("OOM: resources strings %ld\n", bufflen);
        return -1;
    }
    vu->resource_list.names = (char **)buffer;
    char *p = (char*)(&vu->resource_list.names[vu->resource_list.count]);
    for(int ix=0; ix < jresources->u.array.length; ++ix) {
        json_value* jelem = jresources->u.array.values[ix];
        switch (jelem->type) {
            case json_null:
                vu->resource_list.names[ix] = NULL;
                break;
            case json_string:
                strcpy(p, jelem->u.string.ptr);
                vu->resource_list.names[ix] = p;
                p += 1 + jelem->u.string.length;
                break;
            default:
                error_printf("resources: not a string\n");
                return -1;
        }
    }
//    vu->resource_list.textures = calloc(vu->resource_list.count, sizeof(vu->resource_list.textures[0]));
    if (NULL == CALLOC(vu->resource_list.count, vu->resource_list.textures)) {
        error_printf("OOM: calloc textures %d\n", vu->resource_list.count);
        return -1;
    }
    return 0;
}

static int deserialise_placement_list(json_value* jplacements, vumeter_properties_t* vu) {
    if (NULL == jplacements) {
        error_printf("deserialise_placement_list: got null object for placements\n");
        return -1;
    }
    vu->placement_list.count = jplacements->u.array.length;
//    vu->placement_list.elements = calloc(vu->placement_list.count, sizeof( vu->placement_list.elements[0]));
    if(NULL == CALLOC(vu->placement_list.count, vu->placement_list.elements)) {
        error_printf("OOM placements %d\n", vu->placement_list.count);
        return -1;
    }
    for(int ix=0; ix < vu->placement_list.count; ++ix) {
        vu_placement_t* placement = &vu->placement_list.elements[ix];
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

static int deserialise_composition(json_value* jcomposition, vu_composition_t* composition) {
    const char* str = get_object_string_value(jcomposition, "render_op", "static");
    if (!is_string_composition_render_op(str)) {
        error_printf("invalid value for render op %s\n", str);
        return -1;
    }
    composition->render_op = composition_render_op_from_string(str, STATIC);

    str = get_object_string_value(jcomposition, "volume_type", "sampled");
    if (!is_string_composition_volume_type(str)) {
        error_printf("invalid value for volume type %s\n", str);
        return -1;
    }
    composition->volume_type = composition_volume_type_from_string(str, STATIC);

    json_value* jcp = get_object_array_value(jcomposition, "placements");
    if (jcp == NULL) {
        error_printf("got null object for compositions placements\n");
        return -1;
    }
    composition->placement_count = jcp->u.array.length;
//    composition->ix_placements = calloc(composition->placement_count, sizeof(composition->ix_placements[0]));
    if(NULL == CALLOC(composition->placement_count,composition->ix_placements)) {
        error_printf("OOM composition placements %d\n", composition->placement_count);
        return -1;
    }
    for(int ix=0; ix < composition->placement_count; ++ix) {
        if (jcp->u.array.values[ix]->type != json_integer) {
            error_printf("composition placement index is not an integer\n");
            return -1;
        }
        composition->ix_placements[ix] = jcp->u.array.values[ix]->u.integer;
    }
    return 0;
}

static int deserialise_composition_list(json_value* jcompositions, vumeter_properties_t* vu) {
    if (NULL == jcompositions) {
        error_printf("deserialise_composition_list: got null object for compositions\n");
        return -1;
    }
    vu->composition_list.count = jcompositions->u.array.length;
//    vu->composition_list.compositions = calloc(vu->composition_list.count, sizeof( vu->composition_list.compositions[0]));
    if (NULL == CALLOC(vu->composition_list.count, vu->composition_list.compositions)) {
        error_printf("OOM composition list compositions %d\n", vu->composition_list.count);
        return -1;
    }
    
    for(int ix=0; ix < vu->composition_list.count; ++ix) {
        vu_composition_t* composition = &vu->composition_list.compositions[ix];
        if (0 != deserialise_composition(jcompositions->u.array.values[ix], composition)) {
            return -1;
        }
    }

    return 0;
}

static int deserialise_component(json_value* jcomponent,  vu_component_t* component) {
    if (NULL == jcomponent) {
        error_printf("deserialise_component: got null object for component\n");
        return -1;
    }
    if (jcomponent->type != json_array) {
        error_printf("deserialise_component: component is not an array\n");
        return -1;
    }

//    component->ix_compositions = calloc(jcomponent->u.array.length, sizeof(component->ix_compositions[0]));
    if (NULL == CALLOC(jcomponent->u.array.length, component->ix_compositions)) {
        error_printf("deserialise_component: OOM %d\n", jcomponent->u.array.length);
        return -1;
    }

    component->composition_count = jcomponent->u.array.length;
    for(int ix = 0; ix < jcomponent->u.array.length; ++ix) {
        json_value* jcomp = jcomponent->u.array.values[ix];
        if (jcomp->type != json_integer) {
            error_printf("deserialise_component: composition value is not an integer\n");
            return -1;
        }
        component->ix_compositions[ix] = jcomp->u.integer;
    }
    return 0;
}

static int deserialise_vumeter(json_value* jvumeter, vumeter_t* vumeter) {
    if (NULL == jvumeter) {
        error_printf("deserialise_vumeter: got null object for vumeter\n");
        return -1;
    }
    if (jvumeter->type != json_object) {
        error_printf("deserialise_vumeter: vumeter is not an object\n");
        return -1;
    }
    if (jvumeter->u.object.values->value->type != json_array) {
        error_printf("deserialise_vumeter: vumeter object is not an array\n");
        return -1;
    }
    vumeter->name = strdup(jvumeter->u.object.values->name);
    json_value* jcomponents = jvumeter->u.object.values->value;
    if (jcomponents->u.array.length != 1+NUM_VU_CHANNELS) {
        error_printf("deserialise_vumeter: vumeter object array length is incorrect: %d, expected %d\n",
                jcomponents->u.array.length, 1+NUM_VU_CHANNELS);
        return -1;
    }
    vumeter->component_count = jcomponents->u.array.length;
    for(int ix = 0; ix < jcomponents->u.array.length; ++ix) {
        if (0 != deserialise_component(jcomponents->u.array.values[ix], &vumeter->components[ix])) {
            return -1;
        }
    }
    return 0;
}

static int deserialise_vumeter_list(json_value* jvumeters, vumeter_properties_t* vu) {
    if (NULL == jvumeters) {
        error_printf("deserialise_vumeter_list: got null object for vumeters\n");
        return -1;
    }
    vu->vumeter_list.count = jvumeters->u.array.length;
//    vu->vumeter_list.vumeters = calloc(vu->vumeter_list.count, sizeof( vu->vumeter_list.vumeters[0]));
    if (NULL == CALLOC(vu->vumeter_list.count, vu->vumeter_list.vumeters)) {
        error_printf("OOM vumeter list vumeters %d\n", vu->vumeter_list.count);
        return -1;
    }
    
    for(int ix=0; ix < vu->vumeter_list.count; ++ix) {
        vumeter_t* vumeter = &vu->vumeter_list.vumeters[ix];
        if (0 != deserialise_vumeter(jvumeters->u.array.values[ix], vumeter)) {
            return -1;
        }
    }

    return 0;
}

static int _json_deserialise(json_value* jvalue, vumeter_properties_t* vu) {
    const char* str;
    str = get_object_string_value(jvalue, "kind", NULL);
    if (strcmp_ex("vumeter", str)) {
        error_printf("deserialise_vumeter: unsupported kind %s\n", str); 
    }
#ifdef DEBUG_VUMETER_JSON
    vu->kind = strdup(str);
#endif

    str = get_object_string_value(jvalue, "vutype", NULL);
    if (strcmp_ex("compose2", str)) {
        error_printf("deserialise_vumeter: unknown vutype %s\n", str); 
        return -1;
    }
#ifdef DEBUG_VUMETER_JSON
    vu->vutype = strdup(str);
#endif

    str = get_object_string_value(jvalue, "format", NULL);
    if (strcmp_ex("indexed", str)) {
        error_printf("deserialise_vumeter: unsupported format %s\n", str); 
        return -1;
    }
#ifdef DEBUG_VUMETER_JSON
    vu->format = strdup(str);
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

    if (deserialiser_resource_list(get_object_array_value(jvalue, "resources"), vu)) {
        return -1;
    }

    if (deserialise_placement_list(get_object_array_value(jvalue, "placement"), vu)) {
        return -1;
    }

    if (deserialise_composition_list(get_object_array_value(jvalue, "compositions"), vu)) {
        return -1;
    }

    if (deserialise_vumeter_list(get_object_array_value(jvalue, "vumeters"), vu)) {
        return -1;
    }

    { // dummy
        SDL_Point pt;
        deserialise_point(jvalue, &pt);
        get_object_float_value(jvalue, "", 0);
        get_object_double_value(jvalue, "", 0);
        get_object_boolean_value(jvalue, "", 0);
    }
    return 0;
}

void release_vumeter_memory(vumeter_properties_t* vu) {
    if (NULL != vu) {
#ifdef DEBUG_VUMETER_JSON
        FREE(vu->kind);
        FREE(vu->vutype);
        FREE(vu->format);
#endif
        FREE(vu->resource_list.names);
        FREE(vu->resource_list.textures);
        FREE(vu->placement_list.elements);
        for (int ix=0; ix < vu->composition_list.count; ++ix) {
            FREE(vu->composition_list.compositions[ix].ix_placements);
        }
        FREE(vu->composition_list.compositions);
        for (int ix=0; ix < vu->vumeter_list.count; ++ix) {
            FREE(vu->vumeter_list.vumeters[ix].name);
            for (int ich=0; ich < vu->vumeter_list.vumeters[ix].component_count; ++ich) {
                FREE(vu->vumeter_list.vumeters[ix].components[ich].ix_compositions);
            }
        }
        FREE(vu->vumeter_list.vumeters);
        FREE(vu->resource_path);
        free(vu);
    }
}

vumeter_properties_t* deserialise_vumeter_json_string(const char* json_string, size_t length) {
    vumeter_properties_t* vu = NULL;
    if (NULL != CALLOC(1, vu)) {
        json_value* jvalue = json_parse(json_string, length);
        if (NULL == jvalue) {
            error_printf("deserialise_vumeter: failed to parse string\n");
            release_vumeter_memory(vu);
            return NULL;
        }
        int rv = _json_deserialise(jvalue, vu);
        json_value_free(jvalue);
        if (0 != rv) {
            release_vumeter_memory(vu);
            vu = NULL;
        }
    }
    return vu;
}

vumeter_properties_t* json_deserialise_vumeters_file(const char* filepath) {
    FILE *fp;
    struct stat filestatus;
    char* json_string;

    if ( stat(filepath, &filestatus) != 0) {
        error_printf("deserialise_vumeters_file: file %s not found\n", filepath);
        return NULL;
    }

    if (CALLOC(filestatus.st_size, json_string)  == NULL) {
        error_printf("deserialise_vumeters_file: OOM %d %s \n", filestatus.st_size, filepath);
        return NULL;
    }

    fp = fopen(filepath, "rt");
    if (fp == NULL) {
        free(json_string);
        error_printf("deserialise_vumeters_file: failed to open file %s \n", filepath);
        return NULL;
    }

    if (1 != fread(json_string, filestatus.st_size, 1, fp)) {
        fclose(fp);
        free(json_string);
        error_printf("deserialise_vumeters_file: failed to read file data %s \n", filepath);
        return NULL;
    }

    fclose(fp);
    vumeter_properties_t* vu = deserialise_vumeter_json_string(json_string, filestatus.st_size);
    free(json_string);

    if (NULL == vu) {
        error_printf("deserialise_vumeters_file: failed to parse json file %s\n", filepath);
    }
    vu->resource_path = strdup(filepath);
    // remove filename from resource path
    {
        for (char *p = vu->resource_path + strlen(vu->resource_path) - 1;
                p > vu->resource_path;
                --p)
        {
            if (*p == '/') {
                *p = '\0';
                break;
            }
        }
    }
    return vu;
}



