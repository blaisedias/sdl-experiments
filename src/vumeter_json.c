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
    printf("x=%4d, y=%4d, w=%4d, h=%4d", rect->x, rect->y, rect->w, rect->h);
}

static void printf_point(const SDL_Point* point) {
    printf("x=%4d, y=%4d", point->x, point->y);
}

static void printf_placement(const vu_placement_t* placement) {
        printf_rect(&placement->rect);
        printf(", center=");
        printf_point(&placement->center);
        printf(", flip=%d ",
                placement->flip);
        printf(", angle=%06.2f",
                placement->angle);
        printf(", texture_index=%d",
                placement->texture_index);
}

void dump_vumeter_specs(const vu_meters_specs_t* vu_specs) {
    if (vu_specs == NULL) {
        return;
    }
//    printf("resource path :%s\n", vu_specs->resource_path);
#ifdef DEBUG_VUMETER_JSON
    printf("kind:%s, vutype=%s, format=%s\n",
            vu_specs->kind, vu_specs->vutype, vu_specs->format);
#endif
    printf("format version:%d\n", vu_specs->format_version);
    printf("layout:\n    w=%d h=%d arrangement=%d (%s)\n",
            vu_specs->layout.w,
            vu_specs->layout.h,
            vu_specs->layout.arrangement,
            string_from_channel_arrangement(vu_specs->layout.arrangement)
            );
    printf("  viewports:\n");
    for(int ix = 0; ix < sizeof(vu_specs->layout.viewports)/sizeof(vu_specs->layout.viewports[0]); ++ix) {
        printf("    ");
        printf_rect(vu_specs->layout.viewports+ix);
        printf("\n");
    }
    printf("resources:\n    count=%d\n    filenames:\n", vu_specs->resource_list.count);
    for(int ix =0; ix < vu_specs->resource_list.count; ++ix) {
        printf("      %s\n", vu_specs->resource_list.names[ix]);
    }
    printf("placements:\n    count=%d\n", vu_specs->placement_list.count);
    for(int ix =0; ix < vu_specs->placement_list.count; ++ix) {
        printf("    %03d) rect:  ", ix);
        printf_placement(&vu_specs->placement_list.elements[ix]);
        printf("\n");
    }
    printf("compositions:\n    count=%d\n", vu_specs->composition_list.count);
    for(int ix =0; ix < vu_specs->composition_list.count; ++ix) {
        vu_composition_t* composition =  &vu_specs->composition_list.compositions[ix];
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
    printf("vumeters:\n    count=%d\n", vu_specs->vumeter_list.count);
    for(int ix =0; ix < vu_specs->vumeter_list.count; ++ix) {
        vumeter_defn_t* vumeter = &vu_specs->vumeter_list.vumeters[ix];
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

void checked_dump_vumeter_specs(const vu_meters_specs_t* vu_specs) {
    for(int ix_vu =0; ix_vu < vu_specs->vumeter_list.count; ++ix_vu) {
        vumeter_defn_t* vumeter = &vu_specs->vumeter_list.vumeters[ix_vu];
        printf("%s:\n", vumeter->name);
        for (int ich=0; ich < vumeter->component_count; ++ich) {
            printf("  channel %d:\n", ich);
            vu_component_t* component = &vumeter->components[ich];
            for (int ix_cmpnnt_cmpstn=0; ix_cmpnnt_cmpstn < component->composition_count; ++ix_cmpnnt_cmpstn) {
                int ix_composition = component->ix_compositions[ix_cmpnnt_cmpstn];
                printf("    composition %d:\n", ix_cmpnnt_cmpstn);
                // verify composition index lies in the range of compositions list
                if (ix_composition < 0 || ix_composition >= vu_specs->composition_list.count) {
                    error_printf("invalid composition index %d\n", ix_composition);
                    return;
                }
                vu_composition_t* composition = &vu_specs->composition_list.compositions[ix_composition];
                for(int ix_cmpstn_plcmnt=0; ix_cmpstn_plcmnt < composition->placement_count; ++ix_cmpstn_plcmnt) {
                    int ix_placement = composition->ix_placements[ix_cmpstn_plcmnt];
                    if (ix_placement < 0 || ix_placement >= vu_specs->placement_list.count) {
                        error_printf("invalid placement index %d\n", ix_placement);
                        return;
                    }
                    vu_placement_t* placement = &vu_specs->placement_list.elements[ix_placement];
                    int ix_resource = placement->texture_index;
                    if (ix_resource < 0 || ix_resource > vu_specs->resource_list.count) {
                        error_printf("invalid resource index %d\n", ix_resource);
                        return;
                    }
                    printf("      ");
                    printf_placement(placement);
                    printf(", %s\n", vu_specs->resource_list.names[ix_resource]);
                }
            }
        }
    }
}

static bool verify_vumeter_specs_placement(const vu_meters_specs_t* vu_specs, bool verbose, const int placement_index) {
    if (placement_index < 0 || placement_index >= vu_specs->placement_list.count) {
        error_printf("invalid placement index value %d\n", placement_index);
        return false;
    }

    const vu_placement_t* placement = vu_specs->placement_list.elements + placement_index;
    int resource_index = placement->texture_index;
    if (resource_index < 0 || resource_index >= vu_specs->resource_list.count) {
        error_printf("invalid resource index value %d for placement index\n", resource_index, placement_index);
        return false;
    }
    if (verbose) {
        printf("      ");
        printf_placement(placement);
        printf(", %s\n", vu_specs->resource_list.names[resource_index]);
    }
    return true;
}

static bool verify_vumeter_specs_composition(const vu_meters_specs_t* vu_specs, bool verbose, const int composition_index) {
    if (composition_index < 0 || composition_index >= vu_specs->composition_list.count) {
        error_printf("invalid composition index value %d\n", composition_index);
        return false;
    }
    if (verbose) { printf("    composition %d:\n", composition_index); }
    vu_composition_t* composition = &vu_specs->composition_list.compositions[composition_index];
    for(int ix=0; ix < composition->placement_count; ++ ix) {
        if (!verify_vumeter_specs_placement(vu_specs, verbose, composition->ix_placements[ix])) {
            error_printf("invalid composition at %d\n", composition_index);
            return false;
        }
    } 
    return true;
}

static bool verify_vumeter_specs_component(const vu_meters_specs_t* vu_specs, bool verbose, vu_component_t* component) {
    for(int ix=0; ix < component->composition_count; ++ix) {
        if (!verify_vumeter_specs_composition(vu_specs, verbose, component->ix_compositions[ix])) {
            return false;
        }
    }
    return true;
}

static bool verify_vumeter_specs_vumeter(const vu_meters_specs_t* vu_specs, bool verbose, vumeter_defn_t* vumeter) {
    if (vumeter->component_count < 0 || vumeter->component_count > ARRAYLEN(vumeter->components)) {
        error_printf("invalid component count %d\n", vumeter->component_count);
        return false;
    }
    for (int ix=0; ix < vumeter->component_count; ++ix) {
        if (verbose) { printf("  channel %d:\n", ix); }
        if (!verify_vumeter_specs_component(vu_specs, verbose, vumeter->components + ix)) {
            return false;
        }
    }
    return true;
}

bool verify_vumeter_specs(const vu_meters_specs_t* vu_specs, bool verbose) {
    for(int vumeter_index =0; vumeter_index < vu_specs->vumeter_list.count; ++vumeter_index) {
        vumeter_defn_t* vumeter = &vu_specs->vumeter_list.vumeters[vumeter_index];
        if (verbose) { printf("%s: index=%d\n", vumeter->name, vumeter_index); }
        if (!verify_vumeter_specs_vumeter(vu_specs, verbose, vumeter)) {
            return false;
        }
    }
    return true;
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


static bool read_object_int_value(json_value* jvalue, const char* jt, int* destination, int default_value) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type != json_integer) {
        return false;
    }
    *destination = jvalue ? jvalue->u.integer : default_value;
    return true;
}

static const char* get_object_string_value(json_value* jvalue, const char* jt, const char* default_value) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type == json_string) {
            return jvalue->u.string.ptr;
    }
    return default_value;
}

static bool read_object_boolean_value(json_value* jvalue, const char* jt, bool* destination, bool default_value) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type == json_boolean) {
        return false;
    }
    *destination = jvalue ? (bool)jvalue->u.boolean : default_value;
    return default_value;
}

static bool read_object_double_value(json_value* jvalue, const char* jt, double* destination, double default_value) {
    jvalue = get_object_value(jvalue, jt);
    if (jvalue && jvalue->type != json_double) {
        return false;
    }
    *destination = jvalue ? jvalue->u.dbl : default_value;
    return true;
}

static bool read_object_float_value(json_value* jvalue, const char* jt, float* destination, float default_value) {
    double dbl = (double) default_value;
    if (!read_object_double_value(jvalue, jt, &dbl, (double) default_value)) {
        return false;
    }
    *destination = (float)dbl;
    return true;
}

static bool deserialise_rect(json_value* jvalue, SDL_Rect* rect) {
    if (!read_object_int_value(jvalue, "x", &rect->x, 0)) { return false; }
    if (!read_object_int_value(jvalue, "y", &rect->y, 0)) { return false; }
    if (!read_object_int_value(jvalue, "w", &rect->w, 0)) { return false; }
    if (!read_object_int_value(jvalue, "h", &rect->h, 0)) { return false; }
    return true;
}

static bool deserialise_point(json_value* jvalue, SDL_Point* pt) {
    if (!read_object_int_value(jvalue, "x", &pt->x, 0)) { return false; }
    if (!read_object_int_value(jvalue, "y", &pt->y, 0)) { return false; }
    return true;
}

static int deserialise_layout(json_value* jvalue, vu_meters_specs_t* vu_specs) {
    if (NULL == jvalue) {
        error_printf("deserialise_layout: got null object for layout\n");
        return -1;
    }
    vu_specs->layout.w = vu_specs->layout.h = 0;
    read_object_int_value(jvalue, "w", &vu_specs->layout.w, 0);
    if ( 0 >= vu_specs->layout.w ) {
        error_printf("deserialise_layout: invalid value for layout width %d\n", vu_specs->layout.w);
        return -1;
    }
    read_object_int_value(jvalue, "h", &vu_specs->layout.h, 0);
    if ( 0 >= vu_specs->layout.h) {
        error_printf("deserialise_layout: invalid value for layout height %d\n", vu_specs->layout.h);
        return -1;
    }
    const char* str = get_object_string_value(jvalue, "channel_arrangement", NULL);
    if (str && !is_string_channel_arrangement(str)) {
        error_printf("deserialise_layout: invalid value for layout arrangement %s\n", str);
    }
    vu_specs->layout.arrangement = channel_arrangement_from_string(str, NO_ARRANGEMENT);
    json_value* viewport_value = get_object_array_value(jvalue, "rectangles");
    if (viewport_value) {
        int n_viewport = sizeof(vu_specs->layout.viewports)/sizeof(vu_specs->layout.viewports[0]);
        if (viewport_value->u.array.length != n_viewport) {
            error_printf("deserialise_layout: number of layout viewports != %d\n", n_viewport);
        }
        for(int ix=0; ix < MIN(viewport_value->u.array.length, n_viewport); ++ix) {
            if (!deserialise_rect(viewport_value->u.array.values[ix], vu_specs->layout.viewports + ix)) {
                error_printf("deserialise_layout: failed to read viewport %d\n", ix);
            }
        }
    } else {
        return -1;
    }
    return 0;
}

static int deserialise_resource_list(json_value* jresources, vu_meters_specs_t* vu_specs) {
    if (NULL == jresources) {
        error_printf("deserialise_resource_list: got null object for resources\n");
        return -1;
    }
    vu_specs->resource_list.count = jresources->u.array.length;
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
    vu_specs->resource_list.names = (char **)buffer;
    char *p = (char*)(&vu_specs->resource_list.names[vu_specs->resource_list.count]);
    for(int ix=0; ix < jresources->u.array.length; ++ix) {
        json_value* jelem = jresources->u.array.values[ix];
        switch (jelem->type) {
            case json_null:
                vu_specs->resource_list.names[ix] = NULL;
                break;
            case json_string:
                strcpy(p, jelem->u.string.ptr);
                vu_specs->resource_list.names[ix] = p;
                p += 1 + jelem->u.string.length;
                break;
            default:
                error_printf("resources: not a string\n");
                return -1;
        }
    }
//    vu_specs->resource_list.textures = calloc(vu_specs->resource_list.count, sizeof(vu_specs->resource_list.textures[0]));
/* FIXME moved to vumeter_state_t
    if (NULL == CALLOC(vu_specs->resource_list.count, vu_specs->resource_list.textures)) {
        error_printf("OOM: calloc textures %d\n", vu_specs->resource_list.count);
        return -1;
    }
*/
    return 0;
}

static int deserialise_placement_list(json_value* jplacements, vu_meters_specs_t* vu_specs) {
    if (NULL == jplacements) {
        error_printf("deserialise_placement_list: got null object for placements\n");
        return -1;
    }
    vu_specs->placement_list.count = jplacements->u.array.length;
//    vu_specs->placement_list.elements = calloc(vu_specs->placement_list.count, sizeof( vu_specs->placement_list.elements[0]));
    if(NULL == CALLOC(vu_specs->placement_list.count, vu_specs->placement_list.elements)) {
        error_printf("OOM placements %d\n", vu_specs->placement_list.count);
        return -1;
    }
    for(int ix=0; ix < vu_specs->placement_list.count; ++ix) {
        vu_placement_t* placement = &vu_specs->placement_list.elements[ix];
        json_value* jelem = jplacements->u.array.values[ix];

        if (!deserialise_rect(jelem, &placement->rect)) {
            error_printf("deserialise_placement_list: failed to deserialise placement rectangle %d\n", ix);
            return -1;
        }
        json_value* jcenter = get_object_object_value(jelem, "center");
        if (jcenter) {
            if (!deserialise_point(jcenter, &placement->center)) {
                error_printf("deserialise_placement_list: failed to deserialise center point\n");
            }
        }
        int int_flip;
        if (!read_object_int_value(jelem, "flip", &int_flip, 0)) {
            error_printf("deserialise_placement_list: failed to deserialise placement flip %d\n", ix);
            return -1;
        }
        placement->flip = int_flip;
        if (!read_object_float_value(jelem, "angle", &placement->angle, 0)) {
            error_printf("deserialise_placement_list: failed to deserialise placement angle %d\n", ix);
            return -1;
        }
        if (!read_object_int_value(jelem, "resource", &placement->texture_index, 0)) {
            error_printf("deserialise_placement_list: failed to deserialise placement resource %d\n", ix);
            return -1;
        }
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

static int deserialise_composition_list(json_value* jcompositions, vu_meters_specs_t* vu_specs) {
    if (NULL == jcompositions) {
        error_printf("deserialise_composition_list: got null object for compositions\n");
        return -1;
    }
    vu_specs->composition_list.count = jcompositions->u.array.length;
//    vu_specs->composition_list.compositions = calloc(vu_specs->composition_list.count, sizeof( vu_specs->composition_list.compositions[0]));
    if (NULL == CALLOC(vu_specs->composition_list.count, vu_specs->composition_list.compositions)) {
        error_printf("OOM composition list compositions %d\n", vu_specs->composition_list.count);
        return -1;
    }
    
    for(int ix=0; ix < vu_specs->composition_list.count; ++ix) {
        vu_composition_t* composition = &vu_specs->composition_list.compositions[ix];
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

static int deserialise_vumeter(json_value* jvumeter, vumeter_defn_t* vumeter) {
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

static int deserialise_vumeter_list(json_value* jvumeters, vu_meters_specs_t* vu_specs) {
    if (NULL == jvumeters) {
        error_printf("deserialise_vumeter_list: got null object for vumeters\n");
        return -1;
    }
    vu_specs->vumeter_list.count = jvumeters->u.array.length;
//    vu_specs->vumeter_list.vumeters = calloc(vu_specs->vumeter_list.count, sizeof( vu_specs->vumeter_list.vumeters[0]));
    if (NULL == CALLOC(vu_specs->vumeter_list.count, vu_specs->vumeter_list.vumeters)) {
        error_printf("OOM vumeter list vumeters %d\n", vu_specs->vumeter_list.count);
        return -1;
    }
    
    for(int ix=0; ix < vu_specs->vumeter_list.count; ++ix) {
        vumeter_defn_t* vumeter = &vu_specs->vumeter_list.vumeters[ix];
        if (0 != deserialise_vumeter(jvumeters->u.array.values[ix], vumeter)) {
            return -1;
        }
    }

    return 0;
}

static int _json_deserialise(json_value* jvalue, vu_meters_specs_t* vu_specs) {
    const char* str;
    str = get_object_string_value(jvalue, "kind", NULL);
    if (strcmp_ex("vumeter", str)) {
        error_printf("deserialise_vumeter: unsupported kind %s\n", str); 
    }
#ifdef DEBUG_VUMETER_JSON
    vu_specs->kind = strdup(str);
#endif

    str = get_object_string_value(jvalue, "vutype", NULL);
    if (strcmp_ex("compose2", str)) {
        error_printf("deserialise_vumeter: unknown vutype %s\n", str); 
        return -1;
    }
#ifdef DEBUG_VUMETER_JSON
    vu_specs->vutype = strdup(str);
#endif

    str = get_object_string_value(jvalue, "format", NULL);
    if (strcmp_ex("indexed", str)) {
        error_printf("deserialise_vumeter: unsupported format %s\n", str); 
        return -1;
    }
#ifdef DEBUG_VUMETER_JSON
    vu_specs->format = strdup(str);
#endif

    if(!read_object_int_value(jvalue, "format_version", &vu_specs->format_version, -1)) {
        error_printf("deserialise_vumeter: failed to deserialise format version\n");
        return -2;
    }
    // FIXME: 
    if (vu_specs->format_version != 0) {
        error_printf("deserialise_vumeter: format version %d\n", vu_specs->format_version);
        return -2;
    }

    if (!read_object_int_value(jvalue, "volume_levels", &vu_specs->volume_levels, 0)) {
        error_printf("deserialise_vumeter: failed to deserialise volume levels\n");
        return -2;
    }
    if (vu_specs->volume_levels <= 0) {
        error_printf("deserialise_vumeter: invalid volume levels %d\n", vu_specs->volume_levels);
        return -2;
    }

    if (deserialise_layout(get_object_object_value(jvalue, "layout"), vu_specs)) {
        return -1;
    }

    if (deserialise_resource_list(get_object_array_value(jvalue, "resources"), vu_specs)) {
        return -1;
    }

    if (deserialise_placement_list(get_object_array_value(jvalue, "placement"), vu_specs)) {
        return -1;
    }

    if (deserialise_composition_list(get_object_array_value(jvalue, "compositions"), vu_specs)) {
        return -1;
    }

    if (deserialise_vumeter_list(get_object_array_value(jvalue, "vumeters"), vu_specs)) {
        return -1;
    }

    { // dummy
        bool bdummy;
        read_object_boolean_value(jvalue, "", &bdummy, 0);
    }
    return 0;
}

static void free_specs_mem(const vu_meters_specs_t* vu_specs) {
    if (NULL != vu_specs) {
#ifdef DEBUG_VUMETER_JSON
        FREE(vu_specs->kind);
        FREE(vu_specs->vutype);
        FREE(vu_specs->format);
#endif
        FREE(vu_specs->resource_list.names);
/* FIXME moved to vumeter_state_t
        FREE(vu_specs->resource_list.textures);
*/        
        FREE(vu_specs->placement_list.elements);
        for (int ix=0; ix < vu_specs->composition_list.count; ++ix) {
            FREE(vu_specs->composition_list.compositions[ix].ix_placements);
        }
        FREE(vu_specs->composition_list.compositions);
        for (int ix=0; ix < vu_specs->vumeter_list.count; ++ix) {
            FREE(vu_specs->vumeter_list.vumeters[ix].name);
            for (int ich=0; ich < vu_specs->vumeter_list.vumeters[ix].component_count; ++ich) {
                FREE(vu_specs->vumeter_list.vumeters[ix].components[ich].ix_compositions);
            }
        }
        FREE(vu_specs->vumeter_list.vumeters);
    }
}

static void free_state_mem(vu_meters_state_t* vu_state) {
    if (NULL != vu_state) {
        FREE(vu_state->textures_list.textures);
        FREE(vu_state->vu_meter_disabled.elements);
    }
}

vu_meters_t* release_deserialised_vumeters(vu_meters_t* vu) {
    if (NULL != vu) {
        FREE(vu->resource_path);
        free_state_mem(vu->state);
        FREE(vu->state);
        free_specs_mem(vu->spec);
        FREE(vu->spec);
        free(vu);
    }
    return NULL;
}

static bool _deserialise_vumeter_json_string(vu_meters_t** pvu, const char* json_string, size_t length, const char* identifier) {
    vu_meters_t* vu = CALLOC(1, vu);
    *pvu = vu;
    // allocate the vu_meters object
    if (NULL == vu) {
        error_printf("OOM: vu_meters_t\n");
        return false;
    }

    // allocate the vu_meters spec object
    if ( NULL == CALLOC(1, vu->spec)) {
        error_printf("OOM: vu_meters_specs_t\n");
        return false;
    }

    json_value* jvalue = json_parse(json_string, length);
    if (NULL == jvalue) {
        error_printf("deserialise_vumeter: failed to parse string\n");
        return false;
    }

    int rv = _json_deserialise(jvalue, (vu_meters_specs_t*)vu->spec);
    json_value_free(jvalue);
    if (0 != rv) {
        return false;
    }

    if (!verify_vumeter_specs(vu->spec, false)) {
        error_printf("deserialise_vumeters_file: verification failed for %s\n", identifier);
        return false;
    }

    // vumeter spec was loaded and verified,
    // allocate the vu_meters state object
    CALLOC(1, vu->state);
    if (NULL == vu->state) {
        error_printf("OOM: vu_meters_state_t\n");
        return false;
    }

    // each spec resources maps to a texture
    // allocate the vu_meters state texture array
    vu->state->textures_list.count = vu->spec->resource_list.count;
    CALLOC(vu->state->textures_list.count, vu->state->textures_list.textures);
    if (NULL == vu->state->textures_list.textures) {
        error_printf("OOM: textures_id_t %d\n", vu->state->textures_list.count);
        return false;
    }

    vu->state->vu_meter_disabled.count = vu->spec->vumeter_list.count;
    if (NULL == CALLOC(vu->state->vu_meter_disabled.count, vu->state->vu_meter_disabled.elements)) {
        error_printf("OOM: disabled_flags %d\n", vu->state->vu_meter_disabled.count);
        return false;
    }
    return true;
}

vu_meters_t* deserialise_vumeters_json_string(const char* json_string, size_t length, const char* identifier) {
    vu_meters_t* vu = NULL;
    if (!_deserialise_vumeter_json_string(&vu, json_string, length, identifier)) {
        vu = release_deserialised_vumeters(vu);
    }
    return vu;
}

vu_meters_t* deserialise_vumeters_json_file(const char* filepath) {
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

    vu_meters_t* vu = deserialise_vumeters_json_string(json_string, filestatus.st_size, filepath);
    free(json_string);

    if (NULL == vu) {
        error_printf("deserialise_vumeters_file: failed to deserialise json file %s\n", filepath);
        return NULL;
    } 

    vu->resource_path = strdup(filepath);
    if (NULL == vu->resource_path) {
        error_printf("OOM: ressource_path %s\n", filepath);
        vu = release_deserialised_vumeters(vu);
        return NULL;
    }

    // remove filename from resource path
    {
        for (char *p = (char *)(vu->resource_path) + strlen(vu->resource_path) - 1;
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
