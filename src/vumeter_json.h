/*
** Copyright 2026 Blaise Dias. All Rights Reserved.
**
** This file is licensed under BSD. Please see the LICENSE file for details.
*/

#ifndef __jl_vumeter_json_h_
#define __jl_vumeter_json_h_
#include "vumeterdef.h"

vu_meters_t* deserialise_vumeters_json_string(const char* json_string, size_t length);
vu_meters_t* deserialise_vumeters_json_file(const char* filepath);
bool verify_vumeter_specs(const vu_meters_specs_t*);
vu_meters_t* release_deserialised_vumeters(vu_meters_t*);

// debug
void dump_vumeter_specs(const vu_meters_specs_t*);
void checked_dump_vumeter_specs(const vu_meters_specs_t*);
#endif

