#!/usr/bin/env python
'''
test program for jivelite compose2 VU Meters
'''

import copy
import json
import sys
from argparse import ArgumentParser


def link_error(error_message):
    '''
    raise runtime error with messages
    '''
    raise RuntimeError(error_message)


def main():
    '''
    main business logic
    '''
    parser = ArgumentParser()
    parser.add_argument('--src', dest='src', required=True)
    parser.add_argument('--out', dest='out', required=True)
    options = parser.parse_args()

    with open(f'{options.src}/meta.json', encoding='utf-8') as fp:
        vu = json.load(fp)

    print("Checking links", file=sys.stderr)

    # linkage checks for resources
    for placement_name, v in vu['placement'].items():
        try:
            tmp = vu['resources'][v['resource']]
#            tmp = tmp + vu['resources'][v['resource']]['h']
#            if 'w' not in v:
#                v['w'] = vu['resources'][v['resource']]['w']
#            if 'h' not in v:
#                v['h'] = vu['resources'][v['resource']]['h']
        except KeyError:
            link_error(
                f'ERROR: invalid resource linkage {v["resource"]} '
                f'in placement:{placement_name}')

#    for k, v in vu['resources'].items():
#        if k.startswith("#"):
#            continue
#        if isinstance(v, dict):
#            v.pop('w')
#            v.pop('h')

    # linkage checks compositions  for placements
    for composition_name, composition in vu['compositions'].items():
        if composition_name.startswith('#'):
            continue
        placements = composition['placements']
        level = 0
        for placement in placements:
            if placement is not None:
                try:
                    tmp = vu['placement'][placement]['x']
                    tmp = tmp + vu['placement'][placement]['y']
                except KeyError:
                    link_error(
                        'ERROR: invalid placement linkage '
                        f'{placement} in composition[{composition_name}]'
                        f'for level={level}'
                    )
            level += 1

    if 'fascias' in vu:
        for fascia_name, fascia in vu['fascias'].items():
            if fascia_name.startswith('#'):
                continue
            index = 0
            for placement in fascia['placements']:
                try:
                    tmp = vu['placement'][placement]['x']
                    tmp = tmp + vu['placement'][placement]['y']
                except KeyError:
                    link_error(
                        'ERROR: invalid placement linkage '
                        f'{placement} in fascia[{fascia_name}]'
                        f' for index={index}'
                        )
                index += 1

    # vu meter link checks
    for vumeter_name, vumeter in vu['vumeters'].items():
        for k, v in vumeter.items():
            for composition_name in v:
                comps = vu['compositions']
                if k in ['fascia']:
                    comps = vu['fascias']
                if composition_name not in comps:
                    link_error(
                            'ERROR: ivalid composition linkage '
                            f'{composition_name} in composition list "{k}"'
                            f' of vumeter:{vumeter_name}'
                            )

    # checks done, convert to indexed format
    vu_indexed = copy.deepcopy(vu)
    # set the format
    vu_indexed['format'] = 'indexed'
    # remove comments and 'files' members
    keys_to_delete = [k for k in vu_indexed.keys()
                      if k.startswith('#') or k == 'files']
    for k in keys_to_delete:
        del vu_indexed[k]
    del keys_to_delete

    symbol_to_indices = {}
    counts = {}
    symbol_to_indices['layout'] = {'fascia': 0, 'left': 1, 'right': 2}
#    layout_symbols_to_indices = symbol_to_indices['layout']
    src = vu['layout']
    vu_indexed['layout'] = {
        k: v for k, v in src.items() if k not in [
            'fascia', 'left', 'right'
            ] and not k.startswith('#')
    }
    dst = vu_indexed['layout']
    dst['rectangles'] = []
    dst = vu_indexed['layout']['rectangles']
    for k in ['fascia', 'left', 'right']:
        dst.append(src[k])

    if 'rotaries' in vu:
        rotaries_symbols_to_indices = symbol_to_indices['rotaries'] = {}
        src = vu['rotaries']
        vu_indexed['rotaries'] = []
        dst = vu_indexed['rotaries']
        ix = 0
        for k, v in src.items():
            if k.startswith('#'):
                continue
            rotaries_symbols_to_indices[k] = ix
            ix += 1
            dst.append(copy.deepcopy(v))

    src = vu['resources']
    resource_symbols_to_indices = symbol_to_indices['resources'] = {}
    # to support empty placements, add an empty resource, resource index = 0
    vu_indexed['resources'] = [None]
    dst = vu_indexed['resources']
    ix = 1
    for k, v in src.items():
        if k.startswith('#'):
            continue
        try:
            resource_symbols_to_indices[k] = dst.index(v['imagefile'])
        except ValueError:
            resource_symbols_to_indices[k] = ix
            ix += 1
            dst.append(v['imagefile'])
    counts['resources'] = ix

    src = vu['placement']
    placement_symbols_to_indices = symbol_to_indices['placements'] = {}
    # support sparse composition placement arraysin C lang,
    # add empty placement, pointing to empty resource, placement index = 0
    vu_indexed['placement'] = [{"x": 0, "y": 0, "resource": 0, "w": 0, "h": 0}]
    placement_symbols_to_indices['NONE'] = 0
    dst = vu_indexed['placement']
    ix = 1
    for k, v in src.items():
        if k.startswith('#'):
            continue
        placement_symbols_to_indices[k] = ix
        ix += 1
        ival = copy.deepcopy(v)
        ival['resource'] = resource_symbols_to_indices[v['resource']]
        dst.append(ival)
    counts['placements'] = ix

    src = vu['compositions']
    compositions_symbols_to_indices = {}
    compositions_symbols_to_indices = symbol_to_indices['compositions'] = {}
    vu_indexed['compositions'] = [{
        'render_op': "static",
        'volume_type': "none",
        'placements': [0]
        }]
    compositions_symbols_to_indices['NONE'] = 0
    dst = vu_indexed['compositions']
    ix = 1
    for k, v in src.items():
        if k.startswith('#'):
            continue
        compositions_symbols_to_indices[k] = ix
        ix += 1
        ival = copy.deepcopy(v)
        if ival['render_op'] == 'static':
            # overwrite volume_type, the value is ignored,
            # set it to none for clarity
            if ival.get("volume_type") not in ["none", None]:
                print("render_op is static, overriding volume_type to 'none'"
                      f' from {ival.get("volume_type", "")}',
                      file=sys.stderr)
            ival['volume_type'] = "none"
        ival['placements'] = []
        for placement in v['placements']:
            if placement is None:
                ival['placements'].append(0)
            else:
                ival['placements'].append(
                        placement_symbols_to_indices[placement])
        dst.append(ival)
    counts['compositions'] = ix

    src = vu.get('fascias', {})
#    fascias_symbols_to_indices = symbol_to_indices['fascias'] = {}
#    vu_indexed['fascias'] = [{
#        'render_op': "static",
#        'placements': [0]
#        }]
#    fascias_symbols_to_indices['NONE'] = 0
    # fascias are compositions    
    dst = vu_indexed['compositions']
    ix = counts['compositions'] = ix
    for k, v in src.items():
        if k.startswith('#'):
            continue
#        fascias_symbols_to_indices[k] = ix
        compositions_symbols_to_indices[k] = ix
        ix += 1
        # ival = copy.deepcopy(v)
        ival = {}
        ival['render_op'] = "static"
        ival['volume_type'] = "none"
        ival['placements'] = []
        for placement in v['placements']:
            ival['placements'].append(
                    placement_symbols_to_indices[placement])
        dst.append(ival)
        counts['compositions'] += 1

    try:
        del vu_indexed['fascias']
    except KeyError:
        pass

    src = vu['vumeters']
    vumeters_symbols_to_indices = symbol_to_indices['vumeters'] = {}
    vu_indexed['vumeters'] = []
    dst = vu_indexed['vumeters']
    ix = 0
    for k, v in src.items():
        if k.startswith('#'):
            continue
        vumeters_symbols_to_indices[k] = ix
        ix += 1
        ival = []
        for ck in ['fascia', 'left', 'right']:
            cval = []
            if ck not in v:
                cval.append(compositions_symbols_to_indices['NONE'])
            else:
                for composition in v[ck]:
                    cval.append(compositions_symbols_to_indices[composition])
            ival.append(cval)
        dst.append({k: ival})
    counts['vumeters'] = ix

    indices_to_symbols = {}
    for s2i_k, s2i in symbol_to_indices.items():
        indices_to_symbols[s2i_k] = {}
        dst = indices_to_symbols[s2i_k]
        has_0 = False
        for sym, symval in s2i.items():
            if symval == 0:
                has_0 = True
        if not has_0:
            dst['0'] = None
        for sym, symval in s2i.items():
            dst[f'{symval}'] = sym
    vu_indexed['counts'] = counts

    print(f'Writing {options.out}/indexed-meta.json')
    with open(f'{options.out}/indexed-meta.json',
              'w', encoding='utf-8') as fp:
        json.dump(vu_indexed, fp, indent=2)

    vu_crossref = {}
    vu_crossref['indices_to_symbols'] = indices_to_symbols
    vu_crossref['symbol_to_indices'] = symbol_to_indices
    print(f'Writing {options.out}/crossref-indexed-meta.json')
    with open(f'{options.out}/crossref-indexed-meta.json',
              'w', encoding='utf-8') as fp:
        json.dump(vu_crossref, fp, indent=2)


if __name__ == "__main__":
    main()
