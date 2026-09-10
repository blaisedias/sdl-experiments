#ifndef __jl_widgets_json_h_
#define __jl_widgets_json_h_

#include "json.h"
#include "widgets.h"

int deserialise_widgets_file(const char* file_path, view_context_t* ctx);
int deserialise_json(const char* json_string, const int len, view_context_t* ctx);

#endif  // __jl_widgets_json_h_
