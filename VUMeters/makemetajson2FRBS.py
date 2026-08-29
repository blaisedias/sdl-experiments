#!/usr/bin/env python
'''
test program for jivelite compose2 VU Meters
'''

import json
import os
import subprocess
import shutil
import sys
from argparse import ArgumentParser

MAGICK = '/usr/bin/magick'

arrangement_map = {
        "none": "NO_ARRANGEMENT",
        "horizontal": "HORIZONTAL_ARRANGEMENT",
        "vertical": "VERTICAL_ARRANGEMENT",
}


def handle_result(result):
    '''
    examine subprocess output and terminate if command failed
    '''
    if result.returncode != 0:
        print(result.stderr)
        raise RuntimeError()


def image_dim(filepath):
    '''
    return the width and height of an image
    '''
    result = subprocess.run([
        "identify",
        "-format",
        '{"w":%[fx:w], "h":%[fx:h]}',
        filepath
    ], shell=False, text=True, capture_output=True, check=True)
    handle_result(result)
    return json.loads(result.stdout)


def link_error(error_message):
    '''
    raise runtime error with messages
    '''
    raise RuntimeError(error_message)


volume_handling_c_map = {
        'sampled': "SAMPLED",
        'peak-hold+sampled': "PEAK_HOLD_AND_SAMPLED",
        'decay': "DECAY",
        'peak-hold+decay': "PEAK_HOLD_AND_DECAY",
}

def main():
    '''
    main business logic
    '''
    parser = ArgumentParser()
    parser.add_argument('--src', dest='src', required=True)
    parser.add_argument('--out', dest='out', required=True)
    parser.add_argument('--name', dest='name')
    parser.add_argument('--Cpublicname', dest='cpublicname',
                        default='VuProperties')
    options = parser.parse_args()

    with open(f'{options.src}/template.json', encoding='utf-8') as fp:
        vu = json.load(fp)

    try:
        with open(f'{options.src}/rotaries.json', encoding='utf-8') as fp:
            vu['rotaries'].update(json.load(fp)['rotaries'])
        print(f'updated rotaries from {options.src}/rotaries.json')
    except FileNotFoundError:
        pass

    try:
        with open(f'{options.src}/placements.json', encoding='utf-8') as fp:
            vu['placement'].update(json.load(fp)['placement'])
        print(f'updated placements from {options.src}/placements.json')
    except FileNotFoundError:
        pass

    if 'fascia' not in vu['layout']:
        vu['layout']['fascia'] = {
                'x': 0, 'y': 0,
                'w': vu['layout']['w'],
                'h': vu['layout']['h']
                }

    if 'rotaries' not in vu:
        vu['rotaries'] = {}

    # expand sequences in resources
    if '$SEQUENCE' in vu['resources']:
        d = vu['resources']
        for seq in d['$SEQUENCE']:
            kp = seq['key_prefix']
            start = seq['start']
            stop = seq['end'] + 1
            v = seq['value']
            for index in range(start, stop):
                fname = f'{v}'.replace('{index}', f'{index}')
                fname0 = f'{v}'.replace('{index}', f'{index:02}')
                if os.path.isfile(f'{options.src}/{fname}'):
                    d[f'{kp}{index:02}'] = {
                            'imagefile': fname
                    }
                elif os.path.isfile(f'{options.src}/{fname0}'):
                    d[f'{kp}{index:02}'] = {
                            'imagefile': fname0
                    }
                else:
                    raise RuntimeError('Could not find image file'
                                       f'{fname} or {fname0}')
        del d['$SEQUENCE']

    # fixup resources with dimensions
    for k, v in vu['resources'].items():
        if k.startswith("#"):
            continue
        if isinstance(v, dict):
            v.update(image_dim(f'{options.src}/{v["imagefile"]}'))
        else:
            d = {'imagefile': v}
            d.update(image_dim(f'{options.src}/{v}'))
            vu['resources'][k] = d

    image_files = []
    for k, v in vu['resources'].items():
        if k.startswith("#"):
            continue
        image_files.append(v['imagefile'])
    image_files = sorted(set(image_files))
    vu['files'] = image_files

    # expand sequences in placements
    if '$SEQUENCE' in vu['placement']:
        d = vu['placement']
        for seq in d['$SEQUENCE']:
            n = seq['prefix']
            if 'xstep' in seq:
                xstep = seq['xstep']
            else:
                xstep = 0
            if 'ystep' in seq:
                ystep = seq['ystep']
            else:
                ystep = 0
            start = seq['start']
            stop = seq['end'] + 1
            x = seq['x']
            y = seq['y']
            if xstep != 0:
                x = x + ((start - 1) * xstep)
            if ystep != 0:
                y = y + ((start - 1) * ystep)
            for plcmnt in range(start, stop):
                k = f'{n}{plcmnt:02}'
                d[k] = {
                    "x": x,
                    "y": y,
                    "resource": seq['resource']
                }
                # resource link check
                tmp = vu['resources'][d[k]['resource']]['w']
                tmp = tmp + vu['resources'][d[k]['resource']]['h']
                x = x + xstep
                y = y + ystep
        del d['$SEQUENCE']

    if '$ROTATION_SEQUENCE' in vu['placement']:
        d = vu['placement']
        for seq in d['$ROTATION_SEQUENCE']:
            n = seq['prefix']
            start = seq['start']
            stop = seq['end'] + 1
            angle = seq['start_angle']
            angle_step = seq['angle_step']
            rotary = vu['rotaries'][seq['rotary']]
            axle = rotary['axle']
            bearing = rotary['bearing']

            x = seq['x'] + axle['x'] - bearing['x']
            y = seq['y'] + axle['y'] - bearing['y']
            for plcmnt in range(start, stop):
                k = f'{n}{plcmnt:02}'
                d[k] = {
                    'x': x,
                    'y': y,
                    'w': vu['resources'][rotary['resource']]['w'],
                    'h': vu['resources'][rotary['resource']]['h'],
                    'resource': rotary['resource'],
                    'angle': angle,
                    'center': {'x': bearing['x'], 'y': bearing['y']}
                }
                angle += angle_step
        del start, stop, angle, angle_step, axle, bearing, rotary
        del d['$ROTATION_SEQUENCE']

    # fixup resource links
    for k, v in vu['placement'].items():
        if 'resource' not in v:
            v['resource'] = k

    # expand sequences in fascia
    if 'fascia' in vu:
        for fascia_name, fascia in vu['fascia'].items():
            if fascia_name.startswith('#'):
                continue
            placements = fascia['placements']
            if isinstance(placements, dict):
                sequence = placements.get("$SEQUENCE", None)
                if sequence is not None:
                    for seq in sequence:
                        start = seq['start']
                        stop = seq['end'] + 1
                        val = seq['value']
                        for index in range(start, stop):
                            placements[f'{index}'] = f'{val}'.replace(
                                    '{index}', f'{index:02}')
                    del placements['$SEQUENCE']

    # expand sequences in compositions
    for composition_name, composition in vu['compositions'].items():
        if composition_name.startswith('#'):
            continue
        placements = composition['placements']
        if isinstance(placements, dict):
            sequence = placements.get("$SEQUENCE", None)
            if sequence is not None:
                for seq in sequence:
                    start = seq['start']
                    stop = seq['end'] + 1
                    val = seq['value']
                    for index in range(start, stop):
                        placements[f'{index}'] = f'{val}'.replace(
                                '{index}', f'{index:02}')
                del placements['$SEQUENCE']

    if 'fascia' in vu:
        for fascia_name, fascia in vu['fascia'].items():
            if fascia_name.startswith('#'):
                continue
            if isinstance(fascia["placements"], dict):
                placements_list = []
                for _, placement in fascia["placements"].items():
                    placements_list.append(placement)
                fascia["placements"] = placements_list

    for composition_name, composition in vu['compositions'].items():
        if composition_name.startswith('#'):
            continue
        placements = composition['placements']
        if isinstance(placements, dict):
            placements_list = []
            if composition['render_op'] not in ['static']:
                for i in range(vu['volume_levels']):
                    placements_list.append(placements.get(f'{i}', None))
            else:
                for _, placement in placements.items():
                    placements_list.append(placement)
            composition['placements'] = placements_list

    print(f'Writing {options.out}/unchecked-meta.json',
          file=sys.stderr)
    with open(f'{options.out}/unchecked-meta.json',
              'w', encoding='utf-8') as fp:
        json.dump(vu, fp, indent=2)

    print("Checking links", file=sys.stderr)
    # linkage checks for resources
    for placement_name, v in vu['placement'].items():
        try:
            tmp = vu['resources'][v['resource']]['w']
            tmp = tmp + vu['resources'][v['resource']]['h']
            if 'w' not in v:
                v['w'] = vu['resources'][v['resource']]['w']
            if 'h' not in v:
                v['h'] = vu['resources'][v['resource']]['h']
        except KeyError:
            link_error(
                f'ERROR: invalid resource linkage {v["resource"]} '
                f'in {placement_name}')

    for k, v in vu['resources'].items():
        if k.startswith("#"):
            continue
        if isinstance(v, dict):
            v.pop('w')
            v.pop('h')

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

    print(f'Writing {options.out}/meta.json',
          file=sys.stderr)
    with open(f'{options.out}/meta.json', 'w', encoding='utf-8') as fp:
        json.dump(vu, fp, indent=2)


if __name__ == "__main__":
    main()
