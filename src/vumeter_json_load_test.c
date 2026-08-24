#include <stdlib.h>
#include "vumeter.h"
#include "util.h"

static vumeter_instance_t vumeters[100];

static void printf_rect(const SDL_Rect* rect) {
    printf("x=%4d, y=%4d, w=%4d, h=%4d", rect->x, rect->y, rect->w, rect->h);
}

static void printf_point(const SDL_Point* point) {
    printf("x=%4d, y=%4d", point->x, point->y);
}

void vumeter_check_setup(SDL_Rect* bounds_in, int count) {
    static const char* component_prefix[] = {"F", "L", "R"};
    static const bool spc[] = {false, true};
    printf("bounds:\n");
    for (int ixb = 0; ixb < count; ++ixb) {
        printf("  %d) ", ixb);
        printf_rect(bounds_in + ixb);
        printf("\n");
    }
    for (int ix=0; ix < sizeof(vumeters)/sizeof(vumeters[0]); ++ix) {
        vumeter_instance_t* vumtr = &vumeters[ix];
        if (NULL != vumtr->vss) {
            printf("%02d) %s\n", ix, vumtr->defn->name);
            printf("  spec viewports:\n");
            printf("    ");
            for (int iv = 0; iv < ARRAYLEN(vumtr->vss->spec->layout.viewports); ++iv) {
                printf("%s: ", component_prefix[iv]); 
                printf_rect(vumtr->vss->spec->layout.viewports + iv);
                printf(", ");
            }
            printf("\n");
            printf("  vu viewports:\n");
            for (int ixb = 0; ixb < count; ++ixb) {
                SDL_Rect* bounds = &bounds_in[ixb];
                for(int ispc=0; ispc < ARRAYLEN(spc); ++ispc) {
                    vumtr->vss->state->equal_horizontal_spacing = spc[ispc];
                    vumeter_setup(vumtr, bounds);
                    printf("    ");
                    printf(" offset:");
                    printf_point(&vumtr->offset);
                    printf(",  ");
                    for (int iv = 0; iv < ARRAYLEN(vumtr->viewports); ++iv) {
                        printf("%s: ", component_prefix[iv]); 
                        printf_rect(vumtr->viewports + iv);
                        printf(", ");
                    }
                    printf(" scale_factor=%f\n", vumtr->scale_factor);
                }
            }
        }
    }
}

int main(int argc, char** argv) {
    bool dump = false;
    bool checked_dump = false;
    for(int ix=1; ix < argc; ++ix) {
        if (0 == strcmp(argv[ix], "dump")) {
            dump = true;
        }
        if (0 == strcmp(argv[ix], "checkdump")) {
            checked_dump = true;
        }
    }

    for(int ix=1; ix < argc; ++ix) {
        if (0 == strcmp(argv[ix], "dump")) {
            continue;
        }
        if (0 == strcmp(argv[ix], "checkdump")) {
            continue;
        }
            printf("%s ", argv[ix]);
        if (!vumeter_load_from_json_file(argv[ix])) {
            puts("failed");
            exit(EXIT_FAILURE);
        }
        puts("OK");
    }

        if (dump) {
            vumeter_dump_all_specs();
        }
        if (checked_dump) {
            vumeter_checked_dump_all_specs();
        }

    int vumeter_count = vumeter_populate_instance_array(vumeters, ARRAYLEN(vumeters));
    printf("Have %d VU meters\n", vumeter_count);

    SDL_Rect bounds[] = {
        {.x=0, .y=0, .w=1920, .h=600},
        {.x=100, .y=0, .w=1920, .h=600},
    };
    vumeter_check_setup(bounds, ARRAYLEN(bounds));

    vumeter_release_all();
    return 0;
}
