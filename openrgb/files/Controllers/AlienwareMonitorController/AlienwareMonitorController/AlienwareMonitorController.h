/* SPDX-License-Identifier: GPL-2.0-or-later
 * Original drivers: Adam Honse (CalcProgrammer1), 2025;
 * Ferréol DUBOIS COLI (Fefe_du_973), 2025.
 */
#pragma once
#include <hidapi.h>
#include <chrono>
#include <mutex>
#include <string>
#include "AlienwareMonitorProtocol.h"

class AlienwareMonitorController
{
public:
    AlienwareMonitorController(hid_device* dev_handle, const char* path, const AlienwareMonitor::Profile& model);
    ~AlienwareMonitorController();
    std::string GetLocation();
    std::string GetName();
    std::string GetSerialString();
    const AlienwareMonitor::Profile& GetProfile() const;
    bool Initialize();
    bool SendColor(unsigned char mask, unsigned char r, unsigned char g, unsigned char b);
private:
    hid_device* dev;
    std::string location;
    const AlienwareMonitor::Profile& profile;
    std::mutex io_mutex;
    int key_index = -1;
    bool initialized = false;
    std::chrono::steady_clock::time_point next_write;
    std::chrono::steady_clock::time_point retry_after;
    bool WriteReport(const std::vector<unsigned char>& report);
    bool Authenticate(unsigned int index);
    bool SelectKey();
    bool Fail();
};
