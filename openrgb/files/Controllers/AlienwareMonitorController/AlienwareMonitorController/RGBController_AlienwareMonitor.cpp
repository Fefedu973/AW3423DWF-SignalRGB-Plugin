/*---------------------------------------------------------*\
| RGBController_AlienwareMonitor.cpp                        |
|                                                           |
|   RGBController for Alienware monitors                    |
|                                                           |
|   Adam Honse (CalcProgrammer1)                08 May 2025 |
|                                                           |
|   This file is part of the OpenRGB project                |
|   SPDX-License-Identifier: GPL-2.0-or-later               |
\*---------------------------------------------------------*/

#include "RGBController_AlienwareMonitor.h"
#include "LogManager.h"

/**------------------------------------------------------------------*\
    @name Alienware Monitor
    @category Accessory
    @type USB
    @save :x:
    @direct :white_check_mark:
    @effects :x:
    @detectors DetectAlienwareMonitorControllers
    @comment
\*-------------------------------------------------------------------*/

RGBController_AlienwareMonitor::RGBController_AlienwareMonitor(AlienwareMonitorController* controller_ptr)
{
    controller              = controller_ptr;

    name                    = controller->GetName();
    description             = "Alienware Monitor";
    vendor                  = "Alienware";
    type                    = DEVICE_TYPE_MONITOR;
    location                = controller->GetLocation();
    serial                  = controller->GetSerialString();

    mode Direct;
    Direct.name             = "Direct";
    Direct.flags            = MODE_FLAG_HAS_PER_LED_COLOR;
    Direct.color_mode       = MODE_COLORS_PER_LED;
    modes.push_back(Direct);

    active_mode             = 0;

    SetupZones();
}

RGBController_AlienwareMonitor::~RGBController_AlienwareMonitor()
{
    Shutdown();

    delete controller;
}

void RGBController_AlienwareMonitor::SetupZones()
{
    for(const AlienwareMonitor::Zone& entry : controller->GetProfile().zones)
    {
        zone new_zone;
        new_zone.name       = entry.name;
        new_zone.type       = ZONE_TYPE_SINGLE;
        new_zone.leds_min   = 1;
        new_zone.leds_max   = 1;
        new_zone.leds_count = 1;
        zones.push_back(new_zone);
        led new_led;
        new_led.name       = entry.name;
        new_led.value      = entry.mask;
        leds.push_back(new_led);
    }
    SetupColors();
}

void RGBController_AlienwareMonitor::DeviceUpdateLEDs()
{
    if(leds.empty()) return;
    bool equal = true;
    for(unsigned int i = 1; i < leds.size(); ++i) equal &= colors[i] == colors[0];
    if(equal)
    {
        unsigned char red = RGBGetRValue(colors[0]);
        unsigned char grn = RGBGetGValue(colors[0]);
        unsigned char blu = RGBGetBValue(colors[0]);
        if(!controller->SendColor(controller->GetProfile().AllZones(), red, grn, blu))
            LOG_DEBUG("[%s] Color transfer skipped or failed", name.c_str());
    }
    else
    {
        for(unsigned int led_idx = 0; led_idx < leds.size(); led_idx++)
        {
            DeviceUpdateSingleLED(led_idx);
        }
    }
}

void RGBController_AlienwareMonitor::DeviceUpdateZoneLEDs(int zone)
{
    DeviceUpdateSingleLED(zone);
}

void RGBController_AlienwareMonitor::DeviceUpdateSingleLED(int led)
{
    if(led < 0 || static_cast<size_t>(led) >= leds.size()) return;
    unsigned char red = RGBGetRValue(colors[led]);
    unsigned char grn = RGBGetGValue(colors[led]);
    unsigned char blu = RGBGetBValue(colors[led]);
    if(!controller->SendColor(leds[led].value, red, grn, blu))
        LOG_DEBUG("[%s] Color transfer skipped or failed", name.c_str());
}

void RGBController_AlienwareMonitor::DeviceUpdateMode()
{

}
