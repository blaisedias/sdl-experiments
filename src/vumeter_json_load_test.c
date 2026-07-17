#include <stdlib.h>
#include "vumeter_json.h"

static vumeter_properties_t* vu_props_list = NULL;

void vu_props_list_add(vumeter_properties_t* vu) {
    if (vu != NULL) {
        vumeter_properties_t** pvu = &vu_props_list;
        while(*pvu != NULL) {
            pvu = &(*pvu)->next;
        }
        *pvu = vu;
    }
}

void vu_props_list_release(vumeter_properties_t** pvu) {
    if (*pvu) {
        vu_props_list_release(&(*pvu)->next);
        release_vumeter_memory(*pvu);
        *pvu = NULL;
    }
}

int main(int argc, char** argv) {
    bool dump = false;
    for(int ix=1; ix < argc; ++ix) {
        if (0 == strcmp(argv[ix], "dump")) {
            dump = true;
        }
    }

    for(int ix=1; ix < argc; ++ix) {
        if (0 == strcmp(argv[ix], "dump")) {
            continue;
        }
        vumeter_properties_t* vu = json_deserialise_vumeters_file(argv[ix]);
        vu_props_list_add(vu);
        printf("%s ", argv[ix]);
        if (NULL == vu) {
            puts("failed");
            exit(EXIT_FAILURE);
        }
        puts("OK");
    }
    for(vumeter_properties_t* vu = vu_props_list; vu ; vu =vu->next) {
        if (dump) {
            dump_vumeter(vu);
        }
    }
    vu_props_list_release(&vu_props_list);
    return 0;
}
