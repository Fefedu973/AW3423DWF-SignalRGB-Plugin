"""Offline Alienware USBPcap authentication analyzer. Never opens USB devices.

Reads pcap or pcapng (LINKTYPE_USBPCAP=249) using only Python's standard library.
Use: python analyze_alienware_pcap.py capture.pcapng --output analysis.json
Optional --keys keys.json accepts [{"label":"...","hex":"f5 3f ..."}, ...].
An authentication match proves reproduction of the recorded response, not acceptance
by the monitor. VID/PID are reported only when a captured device descriptor supplies
them; filtered captures without descriptors remain explicitly unidentified.
"""
from __future__ import annotations

import argparse
import hashlib
import json
import struct
from functools import reduce
from collections import Counter
from pathlib import Path

USBPCAP = 249
OEM_KEYS = [('universal', 'f53fc139443a31790db18276'), ('dm-8', '7e7f18002b33fbfde364d0c004b0dac7b5'), ('awcc-current-2', '66cba370083ae41d34235e24cff60d57'), ('awcc-current-3', '333a3b140985ea277dcd6ae85f8570'), ('awcc-current-4', 'b9f1b4fb0ff536ef77abf70d72')]


def synthetic_key(oem: bytes) -> bytes:
    result = bytearray()
    pos = 0
    while pos < len(oem) and len(result) < 8:
        run = (oem[pos] & 1) + ((oem[pos] & 0x10) >> 4)
        for _ in range(run):
            if len(result) >= 8:
                break
            if pos + 1 >= len(oem):
                raise ValueError('Truncated OEM key')
            result.append(oem[pos + 1] ^ oem[pos])
            pos += 1
        pos += 1
    if len(result) != 8:
        raise ValueError(f'OEM key generated {len(result)} bytes instead of 8')
    return bytes(result)


def challenge_mask(challenge: bytes) -> bytes:
    if len(challenge) != 16:
        raise ValueError('Challenge must contain exactly 16 bytes, including zeros')
    odd = ((challenge[0] << 8) | challenge[15]).bit_count() % 2 != 0
    if odd:
        out = bytearray(challenge[:8])
        idx = challenge[14] & 7
        out[idx] ^= challenge[idx + 8]
    else:
        out = bytearray(challenge[8:16])
        idx = challenge[6] & 7
        out[idx] ^= challenge[idx]
    return bytes(out)


def generate_response(challenge: bytes, oem: bytes) -> bytes:
    return bytes(a ^ b for a, b in zip(synthetic_key(oem), challenge_mask(challenge)))


def capture_packets(path: Path):
    """Yield (frame_number, section, linktype, timestamp, packet_bytes)."""
    with path.open('rb') as stream:
        magic = stream.read(4)
        stream.seek(0)
        if magic != b'\x0a\x0d\x0d\x0a':
            formats = {
                b'\xd4\xc3\xb2\xa1': ('<', 1_000_000),
                b'\xa1\xb2\xc3\xd4': ('>', 1_000_000),
                b'\x4d\x3c\xb2\xa1': ('<', 1_000_000_000),
                b'\xa1\xb2\x3c\x4d': ('>', 1_000_000_000),
            }
            if magic not in formats:
                raise ValueError('Not a supported pcap or pcapng file')
            endian, scale = formats[magic]
            header = stream.read(24)
            if len(header) != 24:
                raise ValueError('Truncated pcap header')
            linktype = struct.unpack_from(endian + 'I', header, 20)[0] & 0xFFFF
            frame = 0
            while header := stream.read(16):
                if len(header) != 16:
                    raise ValueError('Truncated pcap packet header')
                seconds, fraction, caplen, _ = struct.unpack(endian + 'IIII', header)
                data = stream.read(caplen)
                if len(data) != caplen:
                    raise ValueError('Truncated pcap packet')
                frame += 1
                yield frame, 0, linktype, seconds + fraction / scale, data
            return

        endian, interfaces, frame, section = '<', [], 0, -1
        while header := stream.read(8):
            if len(header) != 8:
                raise ValueError('Truncated pcapng block header')
            if header[:4] == b'\x0a\x0d\x0d\x0a':
                bom = stream.read(4)
                if bom == b'\x4d\x3c\x2b\x1a':
                    endian = '<'
                elif bom == b'\x1a\x2b\x3c\x4d':
                    endian = '>'
                else:
                    raise ValueError('Invalid pcapng byte order magic')
                length = struct.unpack(endian + 'I', header[4:])[0]
                if length < 28 or length % 4:
                    raise ValueError('Invalid pcapng section length')
                rest = stream.read(length - 12)
                if len(rest) != length - 12 or struct.unpack(endian + 'I', rest[-4:])[0] != length:
                    raise ValueError('Truncated or mismatched pcapng section')
                interfaces, section = [], section + 1
                continue
            block_type, length = struct.unpack(endian + 'II', header)
            if length < 12 or length % 4 or length > 100_000_000:
                raise ValueError('Invalid pcapng block length')
            rest = stream.read(length - 8)
            if len(rest) != length - 8 or struct.unpack(endian + 'I', rest[-4:])[0] != length:
                raise ValueError('Truncated or mismatched pcapng block')
            body = rest[:-4]
            if block_type == 1:
                linktype, _, snaplen = struct.unpack_from(endian + 'HHI', body)
                resolution, offset = 1e-6, 0
                pos = 8
                while pos + 4 <= len(body):
                    code, n = struct.unpack_from(endian + 'HH', body, pos)
                    value = body[pos + 4:pos + 4 + n]
                    pos += 4 + ((n + 3) & ~3)
                    if code == 0:
                        break
                    if code == 9 and value:
                        resolution = (2 ** -(value[0] & 0x7F)) if value[0] & 0x80 else (10 ** -value[0])
                    elif code == 14 and n == 8:
                        offset = struct.unpack(endian + 'q', value)[0]
                interfaces.append((linktype, snaplen, resolution, offset))
            elif block_type in (2, 6):
                if len(body) < 20:
                    raise ValueError('Truncated pcapng packet block')
                if block_type == 6:
                    iface, high, low, caplen, _ = struct.unpack_from(endian + 'IIIII', body)
                else:
                    iface, _, high, low, caplen, _ = struct.unpack_from(endian + 'HHIIII', body)
                if iface >= len(interfaces) or 20 + caplen > len(body):
                    raise ValueError('Invalid pcapng interface or packet size')
                linktype, _, resolution, offset = interfaces[iface]
                frame += 1
                yield frame, section, linktype, ((high << 32) | low) * resolution + offset, body[20:20 + caplen]
            elif block_type == 3:
                if not interfaces or len(body) < 4:
                    raise ValueError('Invalid pcapng simple packet block')
                packet_len = struct.unpack_from(endian + 'I', body)[0]
                linktype, snaplen, _, _ = interfaces[0]
                caplen = min(packet_len, snaplen) if snaplen else packet_len
                if caplen > len(body) - 4:
                    raise ValueError('Truncated pcapng simple packet')
                frame += 1
                yield frame, section, linktype, None, body[4:4 + caplen]


def analyze(path: Path, keys: list[tuple[str, bytes]]) -> dict:
    devices, pending_auth, pending_get, pending_descriptor, pending_set = {}, {}, {}, {}, {}
    chains, commands, colors, ddcs, warnings = [], [], [], [], []
    link_counts = Counter()
    frames = 0
    for frame, section, linktype, timestamp, raw in capture_packets(path):
        frames = frame
        link_counts[str(linktype)] += 1
        if linktype != USBPCAP:
            continue
        if len(raw) < 27:
            warnings.append(f'Frame {frame}: truncated USBPcap header')
            continue
        header_len, irp, status, function, info, bus, address, endpoint, transfer, data_len = struct.unpack_from('<HQIHBHHBBI', raw)
        if header_len < 27 or header_len > len(raw):
            warnings.append(f'Frame {frame}: invalid USBPcap header length {header_len}')
            continue
        ident = f'{section}:{bus}:{address}'
        dev = devices.setdefault(ident, {'section': section, 'bus': bus, 'address': address, 'vid': None, 'pid': None, 'first_frame': frame})
        dev['last_frame'] = frame
        payload = raw[header_len:]
        if len(payload) < data_len:
            warnings.append(f'Frame {frame}: captured USB data shorter than declared length')
            continue
        payload = payload[:data_len]
        completed = bool(info & 1)
        direction_in = bool(endpoint & 0x80)
        stage = raw[27] if transfer == 2 and header_len >= 28 else None
        request_key = (ident, irp)
        setup = None
        if transfer == 2 and stage == 0 and not completed and len(payload) >= 8:
            request_type, request, value, index, length = struct.unpack_from('<BBHHH', payload)
            setup = {'request_type': request_type, 'request': request, 'value': value, 'index': index, 'length': length}
            payload = payload[8:]
            if request == 6 and value >> 8 == 1 and request_type & 0x80:
                pending_descriptor[request_key] = frame
            if request == 1 and request_type == 0xA1:
                pending_get[request_key] = frame

        if completed and direction_in and status == 0:
            descriptor_request = pending_descriptor.pop(request_key, None)
            if descriptor_request is not None and len(payload) >= 18 and payload[0:2] == b'\x12\x01':
                vid, pid = struct.unpack_from('<HH', payload, 8)
                dev.update(vid=f'0x{vid:04X}', pid=f'0x{pid:04X}', descriptor_frame=frame)
            get_frame = pending_get.pop(request_key, None)
            if get_frame is not None and ident in pending_auth and len(payload) >= 16:
                pending_auth[ident].update(challenge_frame=frame, get_request_frame=get_frame, challenge=payload[:16].hex(), challenge_payload_length=len(payload))

        if completed and not direction_in and request_key in pending_set:
            pending_set.pop(request_key).update(ack_frame=frame, ack_status=f'0x{status:08X}')

        # USB captures normally omit report ID zero; accept it only at a precise prefix.
        report_id_zero = payload.startswith(b'\x00\x40')
        report = payload[1:] if report_id_zero else payload
        outbound = not direction_in and not completed
        if not outbound or status != 0 or len(report) < 3 or report[0] != 0x40:
            continue
        base = {'frame': frame, 'timestamp': timestamp, 'device': ident, 'transport': 'control' if transfer == 2 else f'transfer-{transfer}', 'payload_length': len(payload), 'leading_report_id_zero': report_id_zero}
        if setup:
            base['setup'] = setup
        command = {**base, 'prefix': report[:12].hex(), 'data_64_95': report[64:96].hex()}
        commands.append(command)
        if setup and setup['request'] == 9:
            pending_set[request_key] = command
        if report[:3] == b'\x40\xe1\x01':
            if ident in pending_auth:
                chains.append({**pending_auth.pop(ident), 'result': 'no_recorded_response'})
            pending_auth[ident] = {**base, 'challenge_request_frame': frame}
        elif report[:3] == b'\x40\xe1\x02':
            chain = pending_auth.pop(ident, {'device': ident})
            chain.update(response_frame=frame, response_payload_length=len(payload))
            if len(report) < 72:
                chain['result'] = 'response_payload_too_short'
            else:
                observed = report[64:72]
                chain['observed_response'] = observed.hex()
                if 'challenge' not in chain:
                    chain['result'] = 'challenge_not_captured'
                else:
                    challenge = bytes.fromhex(chain['challenge'])
                    expected = [(label, generate_response(challenge, oem)) for label, oem in keys]
                    chain['matching_oem_keys'] = [label for label, response in expected if observed == response]
                    chain['universal_expected'] = dict(expected).get('universal', b'').hex()
                    chain['inferred_synthetic_key'] = bytes(a ^ b for a, b in zip(observed, challenge_mask(challenge))).hex()
                    chain['parity'] = 'odd' if ((challenge[0] << 8) | challenge[15]).bit_count() % 2 else 'even'
                    chain['result'] = 'matched' if chain['matching_oem_keys'] else 'no_known_key_matches'
            chains.append(chain)
        elif report[:2] == b'\x40\xc6' and len(report) >= 74:
            ddc_length = int.from_bytes(report[6:8], 'little')
            if not 4 <= ddc_length <= len(report) - 64:
                warnings.append(f'Frame {frame}: invalid DDC payload length {ddc_length}')
                continue
            ddc = report[64:64 + ddc_length]
            checksum = reduce(int.__xor__, ddc[:-1], 0x6e)
            ddc_record = {**base, 'ddc_length': ddc_length, 'ddc_length_matches_header': ddc_length == (ddc[1] & 0x7F) + 3, 'ddc_opcode': ddc[2], 'ddc_payload': ddc.hex(), 'checksum': ddc[-1], 'checksum_offset': 64 + ddc_length - 1, 'full_ddc_checksum_expected': checksum, 'full_ddc_checksum_matches': checksum == ddc[-1]}
            if ddc[2] == 0xd0 and ddc_length >= 10:
                ddc_record.update(mode_byte_67=report[67], zone_byte_68=report[68], rgb_bytes_69_71=list(report[69:72]), brightness_byte_72=report[72], effect_parameters=list(report[73:64 + ddc_length - 1]))
                colors.append(ddc_record)
            ddcs.append(ddc_record)

    chains.extend({**p, 'result': 'no_recorded_response'} for p in pending_auth.values())
    known = {key: dev for key, dev in devices.items() if dev['vid'] is not None}
    for records in (chains, ddcs, commands):
        for item in records:
            dev = devices[item['device']]
            item['vid'] = dev['vid']
            item['pid'] = dev['pid']
    result_counts = Counter(c['result'] for c in chains)
    return {
        'source': str(path.resolve()), 'sha256': sha256_file(path),
        'frames': frames, 'linktypes': dict(link_counts), 'devices_with_descriptors': known,
        'authentication_devices': {k: devices[k] for k in {c['device'] for c in chains}},
        'summary': {'chains': len(chains), 'results': dict(result_counts), 'universal_matches': sum('universal' in c.get('matching_oem_keys', []) for c in chains), 'ddc_commands': len(ddcs), 'color_commands': len(colors), 'full_ddc_checksum_matches': sum(c['full_ddc_checksum_matches'] for c in ddcs), 'ddc_length_matches': sum(c['ddc_length_matches_header'] for c in ddcs), 'set_report_acknowledged_success': sum(c.get('ack_status') == '0x00000000' for c in commands), 'mode_counts': dict(Counter(f"0x{c['mode_byte_67']:02X}" for c in colors))},
        'oem_synthetic_keys': {label: synthetic_key(key).hex() for label, key in keys},
        'authentication_chains': chains, 'ddc_commands': ddcs, 'color_commands': colors, 'commands': commands,
        'warnings': warnings,
        'limits': ['Matching recorded bytes does not establish hardware acceptance.', 'VID/PID are unknown without a captured device descriptor.', 'Only USBPcap linktype 249 is decoded; other linktypes are counted.'],
    }


def sha256_file(path: Path) -> str:
    with path.open('rb') as source:
        return hashlib.file_digest(source, 'sha256').hexdigest()


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('capture', type=Path)
    parser.add_argument('--output', type=Path)
    parser.add_argument('--keys', type=Path)
    args = parser.parse_args()
    keys = [(label, bytes.fromhex(value)) for label, value in OEM_KEYS]
    if args.keys:
        keys.extend((row['label'], bytes.fromhex(row['hex'])) for row in json.loads(args.keys.read_text(encoding='utf-8-sig')))
    result = analyze(args.capture, keys)
    if args.output:
        args.output.parent.mkdir(parents=True, exist_ok=True)
        args.output.write_text(json.dumps(result, ensure_ascii=False, indent=2), encoding='utf-8')
    print(json.dumps({'source': result['source'], 'frames': result['frames'], 'summary': result['summary'], 'devices_with_descriptors': result['devices_with_descriptors'], 'authentication_devices': result['authentication_devices'], 'warnings': result['warnings'][:10]}, ensure_ascii=True, indent=2))


if __name__ == '__main__':
    main()
