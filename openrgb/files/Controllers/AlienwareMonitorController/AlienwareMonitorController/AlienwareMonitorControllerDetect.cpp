/* SPDX-License-Identifier: GPL-2.0-or-later
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
REGISTER_HID_DETECTOR_PU("Alienware AW2518H", DetectAlienwareMonitorControllers, 0x0424, 0x274C, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW2521H", DetectAlienwareMonitorControllers, 0x187C, 0x100A, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW2521HF", DetectAlienwareMonitorControllers, 0x187C, 0x1006, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW2521HFL", DetectAlienwareMonitorControllers, 0x187C, 0x1007, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW2524H", DetectAlienwareMonitorControllers, 0x187C, 0x100F, 0xFFDA, 0x00DA);
REGISTER_HID_DETECTOR_PU("Alienware AW2720HF", DetectAlienwareMonitorControllers, 0x187C, 0x1005, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW2721D", DetectAlienwareMonitorControllers, 0x187C, 0x1009, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW2723DF", DetectAlienwareMonitorControllers, 0x187C, 0x100C, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW2724DM", DetectAlienwareMonitorControllers, 0x187C, 0x1010, 0xFFDA, 0x00DA);
REGISTER_HID_DETECTOR_PU("Alienware AW2725DF", DetectAlienwareMonitorControllers, 0x187C, 0x1014, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW2725Q", DetectAlienwareMonitorControllers, 0x187C, 0x1019, 0xFFDA, 0x00DA);
REGISTER_HID_DETECTOR_PU("Alienware AW3225QF", DetectAlienwareMonitorControllers, 0x187C, 0x1013, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW3226Q", DetectAlienwareMonitorControllers, 0x187C, 0x1020, 0xFFDA, 0x00DA);
REGISTER_HID_DETECTOR_PU("Alienware AW3418DW", DetectAlienwareMonitorControllers, 0x0424, 0x274A, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW3418HW", DetectAlienwareMonitorControllers, 0x0424, 0x274B, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW3420DW", DetectAlienwareMonitorControllers, 0x0424, 0x2745, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW3423DW", DetectAlienwareMonitorControllers, 0x187C, 0x100B, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW3423DWF", DetectAlienwareMonitorControllers, 0x187C, 0x100E, 0xFFDA, 0x00DA);
REGISTER_HID_DETECTOR_PU("Alienware AW3425DW", DetectAlienwareMonitorControllers, 0x187C, 0x101A, 0xFFDA, 0x00DA);
REGISTER_HID_DETECTOR_PU("Alienware AW3426DW", DetectAlienwareMonitorControllers, 0x187C, 0x101D, 0xFFDA, 0x00DA);
REGISTER_HID_DETECTOR_PU("Alienware AW3821DW", DetectAlienwareMonitorControllers, 0x187C, 0x1008, 0xFF00, 0x0001);
REGISTER_HID_DETECTOR_PU("Alienware AW3926QW", DetectAlienwareMonitorControllers, 0x187C, 0x1021, 0xFFDA, 0x00DA);
REGISTER_HID_DETECTOR_PU("Alienware AW5520QF", DetectAlienwareMonitorControllers, 0x0424, 0x2741, 0xFF00, 0x0001);
