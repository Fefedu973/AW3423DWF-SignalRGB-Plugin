"""Export independently captured packets, retaining source hashes and frame numbers.
Usage: python extract_fixtures.py PATH_TO_LOCAL_ARCHIVES_ANALYSIS_DIRECTORY
This script is optional: fixtures.json is already included for offline test runs.
"""
import importlib.util
import json
import pathlib
import struct
import sys

archive = pathlib.Path(sys.argv[1])
spec = importlib.util.spec_from_file_location('capture_parser', archive / 'analyze_alienware_pcap.py')
parser = importlib.util.module_from_spec(spec)
spec.loader.exec_module(parser)
result = {'description': 'USB captures; HIDAPI zero Report ID added to wire payloads.', 'colors': [], 'auth': []}
sources = ['AW3423DWF-monitor-analysis.json', 'AW2724DM-analysis.json', 'aw3426dw-analysis.json', 'signalrgb-colors-analysis.json']
for filename in sources:
    analysis = json.loads((archive / filename).read_text(encoding='utf-8'))
    wanted = {c['frame']: c for c in analysis['color_commands'] if c.get('mode_byte_67') == 4}
    packets = {}
    for frame, _, link, _, raw in parser.capture_packets(pathlib.Path(analysis['source'])):
        if frame not in wanted:
            continue
        header = struct.unpack_from('<H', raw)[0]
        body = raw[header:]
        if wanted[frame]['transport'] == 'control':
            body = body[8:]
        if not wanted[frame]['leading_report_id_zero']:
            body = b'\0' + body
        packets[frame] = body.hex()
    seen = set()
    for frame, c in wanted.items():
        identity = (c['zone_byte_68'], *c['rgb_bytes_69_71'])
        if identity in seen:
            continue
        seen.add(identity)
        result['colors'].append({'source': pathlib.Path(analysis['source']).name, 'sha256': analysis['sha256'],
             'frame': frame, 'pid': c['pid'], 'mask': identity[0], 'rgb': list(identity[1:]), 'hid_report_hex': packets[frame]})
    for c in analysis['authentication_chains']:
        keys = c.get('matching_oem_keys', [])
        key_index = 0 if 'universal' in keys else 1 if 'dm-8' in keys else None
        if key_index is not None:
            result['auth'].append({'source': pathlib.Path(analysis['source']).name, 'sha256': analysis['sha256'],
                  'frame': c['response_frame'], 'token_hex': c['challenge'], 'response_hex': c['observed_response'],
                  'key_index': key_index, 'parity': c['parity']})
out = pathlib.Path(__file__).with_name('fixtures.json')
out.write_text(json.dumps(result, indent=2) + '\n', encoding='utf-8', newline='\n')
print(f'Exported {len(result["colors"])} unique color packets and {len(result["auth"])} authentication pairs')
