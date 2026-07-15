#!/usr/bin/env python3
r'''
'''
import copy
import json
import sys
import os.path
from argparse import ArgumentParser


def main():
    '''
    main business logic
    '''
    parser = ArgumentParser()
    parser.add_argument('--src', dest='src', required=True)
    parser.add_argument('--destdir', dest='destdir', required=True)
    parser.add_argument('--name', dest='name', required=True)
    options = parser.parse_args()

    with open(f'{options.src}', encoding='utf-8') as fp:
        data = json.load(fp)

    with open(f'{options.destdir}/{options.name}.h',
              "w", encoding='utf-8') as fp:
        print('// Do not edit: this file is generated', file=fp)
        print(f'#ifndef __{options.name}__', file=fp)
        print('#include "types.h"', file=fp)
        for entry in data['enums']:
            enum_name = entry['name']
            print('\ntypedef enum {', file=fp)
            for e in entry['enum']:
                print(f'    {e["symbol"]},', file=fp)
            print(f'}} {enum_name}_t;\n', file=fp)

        for entry in data['enums']:
            enum_name = entry['name']
            print(f"""
bool is_string_{enum_name}(const char* s);
{enum_name}_t {enum_name}_from_string(const char* s, {enum_name}_t defv);
const char* string_from_{enum_name}({enum_name}_t v);
""",
                  file=fp)

        print(f'#endif // __{options.name}__', file=fp)

    fpath = os.path.join(options.destdir, options.name)
    fname = options.name
    with open(f'{fpath}.c',
              "w", encoding='utf-8') as fp:
        print('// Do not edit: this file is generated', file=fp)
        print('#include <string.h>\n', file=fp)
        print(f'#include "{fname}.h"', file=fp)
        for entry in data['enums']:
            enum_name = entry['name']
            print(f'\nstatic const char* {enum_name}_strings [] = {{', file=fp)
            for e in entry['enum']:
                print(f'    "{e["string"]}",', file=fp)
            print('};\n', file=fp)

        print('\n#define ARRAYLEN(a) sizeof((a))/sizeof((a)[0])\n',
              file=fp)

        for entry in data['enums']:
            enum_name = entry['name']
            print(f"""
bool is_string_{enum_name}(const char* s) {{
    bool found = false;
    if (NULL != s) {{
        for (int ix = 0; ix < ARRAYLEN({enum_name}_strings) ; ++ix) {{
            if (0 == strcmp(s, {enum_name}_strings[ix])) {{
                return true;
            }}
        }}
    }}
    return found;
}}

{enum_name}_t {enum_name}_from_string(const char* s, {enum_name}_t defv) {{
    if (NULL != s) {{
        for (int ix = 0; ix < ARRAYLEN({enum_name}_strings) ; ++ix) {{
            if (0 == strcmp(s, {enum_name}_strings[ix])) {{
                return ix;
            }}
        }}
    }}
    return defv;
}}

const char* string_from_{enum_name}({enum_name}_t v) {{
    for (int ix = 0; ix < ARRAYLEN({enum_name}_strings) ; ++ix) {{
        if (v == ix) {{
            return {enum_name}_strings[ix];
        }}
    }}
    return NULL;
}}
""",
                  file=fp)


if __name__ == "__main__":
    main()
