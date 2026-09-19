/* SPDX-License-Identifier: GPL-2.0-or-later */
#include "AlienwareMonitorProtocol.h"

namespace AlienwareMonitor
{
/* Models/zones/auth: installed AWCC FxDisplayCommon 6.14.24.0 and FxMetaData.
   Legacy 0424:274A/B/C: SignalRGB Alienware_Monitor_Gen1_Controller.js.
   Models without RGB zones or known transport are deliberately not registered. */
const std::vector<Profile>& Profiles()
{
    static const std::vector<Profile> profiles = {
        {0x0424, 0x274C, "Alienware AW2518H", Transport::Legacy, false, 0, 50, {{"Back Logo", 0x01}}},
        {0x187C, 0x100A, "Alienware AW2521H", Transport::Microchip, false, 0, 50, {{"Logo", 0x01}, {"Stand", 0x02}, {"Power Button", 0x08}}},
        {0x187C, 0x1006, "Alienware AW2521HF", Transport::Microchip, false, 0, 50, {{"Back", 0x01}}},
        {0x187C, 0x1007, "Alienware AW2521HFL", Transport::Microchip, false, 0, 50, {{"Back", 0x01}}},
        {0x187C, 0x100F, "Alienware AW2524H", Transport::Realtek, false, 0, 200, {{"Logo", 0x01}, {"Number", 0x02}, {"Power Button", 0x08}}},
        {0x187C, 0x1005, "Alienware AW2720HF", Transport::Microchip, false, 0, 50, {{"Back", 0x01}}},
        {0x187C, 0x1009, "Alienware AW2721D", Transport::Microchip, false, 0, 50, {{"Logo", 0x01}, {"Stand", 0x02}, {"Downlight", 0x04}, {"Power Button", 0x08}}},
        {0x187C, 0x100C, "Alienware AW2723DF", Transport::Microchip, false, 0, 50, {{"Logo", 0x01}, {"Number", 0x02}, {"Power Button", 0x08}}},
        {0x187C, 0x1010, "Alienware AW2724DM", Transport::Realtek, true, 1, 200, {{"Logo", 0x01}, {"Number", 0x02}, {"Power Button", 0x08}}},
        {0x187C, 0x1014, "Alienware AW2725DF", Transport::Microchip, false, 0, 50, {{"Logo", 0x01}, {"Number", 0x02}, {"Power Button", 0x08}}},
        {0x187C, 0x1019, "Alienware AW2725Q", Transport::Realtek, true, 0, 200, {{"Logo", 0x01}, {"Power Button", 0x08}}},
        {0x187C, 0x1013, "Alienware AW3225QF", Transport::Microchip, false, 0, 50, {{"Logo", 0x01}, {"Number", 0x02}, {"Power Button", 0x08}}},
        {0x187C, 0x1020, "Alienware AW3226Q", Transport::Realtek, false, 0, 200, {{"Logo", 0x01}, {"Power Button", 0x08}}},
        {0x0424, 0x274A, "Alienware AW3418DW", Transport::Legacy, false, 0, 50, {{"Upper Left Backside", 0x01}, {"Stand Backside", 0x02}, {"Middle Bottom", 0x04}, {"Switch", 0x08}}},
        {0x0424, 0x274B, "Alienware AW3418HW", Transport::Legacy, false, 0, 50, {{"Upper Left Backside", 0x01}, {"Stand Backside", 0x02}, {"Middle Bottom", 0x04}, {"Switch", 0x08}}},
        {0x0424, 0x2745, "Alienware AW3420DW", Transport::Microchip, false, 0, 50, {{"Logo", 0x01}, {"Stand", 0x02}, {"Downlight", 0x04}, {"Power Button", 0x08}}},
        {0x187C, 0x100B, "Alienware AW3423DW", Transport::Microchip, false, 0, 50, {{"Logo", 0x01}, {"Stand", 0x02}, {"Downlight", 0x04}, {"Power Button", 0x08}}},
        {0x187C, 0x100E, "Alienware AW3423DWF", Transport::Realtek, true, 0, 200, {{"Logo", 0x01}, {"Number", 0x02}, {"Power Button", 0x08}}},
        {0x187C, 0x101A, "Alienware AW3425DW", Transport::Realtek, true, 0, 200, {{"Logo", 0x01}, {"Power Button", 0x08}}},
        {0x187C, 0x101D, "Alienware AW3426DW", Transport::Realtek, false, 0, 100, {{"Logo", 0x01}, {"Power Button", 0x08}}},
        {0x187C, 0x1008, "Alienware AW3821DW", Transport::Microchip, false, 0, 50, {{"Logo", 0x01}, {"Stand", 0x02}, {"Downlight", 0x04}, {"Power Button", 0x08}}},
        {0x187C, 0x1021, "Alienware AW3926QW", Transport::Realtek, false, 0, 200, {{"Logo", 0x01}, {"Power Button", 0x08}}},
        {0x0424, 0x2741, "Alienware AW5520QF", Transport::Microchip, false, 0, 50, {{"Back", 0x01}}},
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
