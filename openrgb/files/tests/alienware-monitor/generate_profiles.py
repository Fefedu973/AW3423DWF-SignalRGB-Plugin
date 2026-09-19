"""Regenerate the reviewed profile table from the evidence matrix (not a hardware probe)."""
import json
import pathlib
import sys

root = pathlib.Path(__file__).resolve().parents[2]
directory = root / 'Controllers/AlienwareMonitorController/AlienwareMonitorController'
models = json.loads(pathlib.Path(sys.argv[1]).read_text(encoding='utf-8'))['models']
families = {'legacy_92_48': 'Legacy', 'microchip_hid_i2c_ddc': 'Microchip', 'realtek_hid_ddc': 'Realtek'}
rows = []
registrations = []
for m in models:
    if not m.get('zones') or m.get('protocol_family') not in families or m.get('auth') is None:
        continue
    vid, pid = m['vid'], m['pid']
    transport = families[m['protocol_family']]
    zones = ', '.join('{"%s", 0x%02X}' % (z['name'], z['mask']) for z in m['zones'])
    delay = 100 if pid == '0x101D' else (200 if transport == 'Realtek' else 50)
    rows.append('        {%s, %s, "Alienware %s", Transport::%s, %s, %d, %d, {%s}},' %
                (vid, pid, m['model'], transport, str(m['auth']).lower(), int(pid == '0x1010'), delay, zones))
    page, usage = ('0xFFDA', '0x00DA') if transport == 'Realtek' else ('0xFF00', '0x0001')
    registrations.append('REGISTER_HID_DETECTOR_PU("Alienware %s", DetectAlienwareMonitorControllers, %s, %s, %s, %s);' %
                         (m['model'], vid, pid, page, usage))

profile_text = '''/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "AlienwareMonitorProtocol.h"

namespace AlienwareMonitor
{
/* Models/zones/auth: installed AWCC FxDisplayCommon 6.14.24.0 and FxMetaData.
   Legacy 0424:274A/B/C: SignalRGB Alienware_Monitor_Gen1_Controller.js.
   Models without RGB zones or known transport are deliberately not registered. */
const std::vector<Profile>& Profiles()
{
    static const std::vector<Profile> profiles = {
ROWS
    };
    return profiles;
}

const Profile* FindProfile(unsigned short vid, unsigned short pid)
{
    for(const Profile& profile : Profiles())
        if(profile.vid == vid && profile.pid == pid) return &profile;
    return nullptr;
}
}
'''.replace('ROWS', '\n'.join(rows))
detector_text = '''/* SPDX-License-Identifier: GPL-2.0-or-later
 * Original detector: Adam Honse (CalcProgrammer1), 2025.
 */
#include "AlienwareMonitorController.h"
#include "DetectionManager.h"
#include "RGBController_AlienwareMonitor.h"
#include "LogManager.h"

DetectedControllers DetectAlienwareMonitorControllers(hid_device_info* info, const std::string& /*name*/)
{
    DetectedControllers detected;
    const AlienwareMonitor::Profile* profile = AlienwareMonitor::FindProfile(info->vendor_id, info->product_id);
    if(!profile) return detected;
    hid_device* dev = hid_open_path(info->path);
    if(!dev) return detected;
    AlienwareMonitorController* controller = new AlienwareMonitorController(dev, info->path, *profile);
    if(!controller->Initialize())
    {
        LOG_WARNING("[%s] Monitor initialization or authentication transport failed", profile->name);
        delete controller;
        return detected;
    }
    detected.push_back(new RGBController_AlienwareMonitor(controller));
    return detected;
}

/* The usage filter selects the monitor lighting collection, not other hub HID functions. */
REGISTRATIONS
'''.replace('REGISTRATIONS', '\n'.join(registrations))
(directory / 'AlienwareMonitorProfiles.cpp').write_text(profile_text, encoding='utf-8', newline='\n')
(directory / 'AlienwareMonitorControllerDetect.cpp').write_text(detector_text, encoding='utf-8', newline='\n')
print('Generated %d distinct monitor profiles' % len(rows))
