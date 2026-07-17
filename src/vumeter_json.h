/*
** Copyright 2026 Blaise Dias. All Rights Reserved.
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/

#ifndef __jl_vumeter_json_h_
#define __jl_vumeter_json_h_
#include "vumeterdef.h"

void release_vumeter_memory(vumeter_properties_t* vu);
vumeter_properties_t* deserialise_vumeter_json_string(const char* json_string, size_t length);
vumeter_properties_t* json_deserialise_vumeters_file(const char* filepath);

// debug
void dump_vumeter(const vumeter_properties_t* vu);
#endif

