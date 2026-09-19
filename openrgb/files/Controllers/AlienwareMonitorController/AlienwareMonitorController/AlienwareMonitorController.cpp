/* SPDX-License-Identifier: GPL-2.0-or-later
 * Original drivers: Adam Honse (CalcProgrammer1), 2025;
 * Ferréol DUBOIS COLI (Fefe_du_973), 2025 (AW3423DWF authentication).
 */
#include <thread>
#include "AlienwareMonitorController.h"
#include "StringUtils.h"

using namespace AlienwareMonitor;

AlienwareMonitorController::AlienwareMonitorController(hid_device* dev_handle, const char* path, const Profile& model)
    : dev(dev_handle), location(path), profile(model) {}
AlienwareMonitorController::~AlienwareMonitorController() { hid_close(dev); }
std::string AlienwareMonitorController::GetLocation() { return "HID: " + location; }
std::string AlienwareMonitorController::GetName() { return profile.name; }
const Profile& AlienwareMonitorController::GetProfile() const { return profile; }

std::string AlienwareMonitorController::GetSerialString()
{
    std::lock_guard<std::mutex> lock(io_mutex);
    wchar_t serial[256] = {};
    if(hid_get_serial_number_string(dev, serial, 256) < 0) return "";
    return StringUtils::wchar_to_string(serial);
}

bool AlienwareMonitorController::WriteReport(const std::vector<unsigned char>& report)
{
    return !report.empty() && hid_write(dev, report.data(), report.size()) == static_cast<int>(report.size());
}

bool AlienwareMonitorController::Fail()
{
    key_index = -1;
    retry_after = std::chrono::steady_clock::now() + std::chrono::seconds(1);
    return false;
}

bool AlienwareMonitorController::Authenticate(unsigned int index)
{
    if(index >= OEMKeys().size()) return false;
    std::vector<unsigned char> packet(193, 0);
    packet[1] = 0x40; packet[2] = 0xE1; packet[3] = 1;
    if(!WriteReport(packet)) return false;
    /* HIDAPI retains the zero Report ID, including for unnumbered reports. */
    std::array<unsigned char, 193> input = {};
    const int count = hid_get_input_report(dev, input.data(), input.size());
    if(count < 17 || count > static_cast<int>(input.size()) || input[0] != 0) return false;
    std::array<unsigned char, 8> key = {};
    if(!GenerateKey(input.data() + 1, 16, OEMKeys()[index], key)) return false;
    packet[3] = 2;
    std::copy(key.begin(), key.end(), packet.begin() + 65);
    return WriteReport(packet);
}

bool AlienwareMonitorController::SelectKey()
{
    /* Match AWCC's bounded handshake + DDC probe. A successful transfer is
       transport evidence, not independent proof that the monitor accepted a key. */
    const auto probe = DDCReport(Transport::Realtek, {0x51,0x82,0x01,0xC8,0x74}, 0);
    for(unsigned int attempt = 0; attempt < OEMKeys().size(); ++attempt)
    {
        const unsigned int index = (profile.preferred_key + attempt) % OEMKeys().size();
        if(Authenticate(index) && WriteReport(probe))
        {
            key_index = static_cast<int>(index);
            return true;
        }
    }
    return Fail();
}

bool AlienwareMonitorController::Initialize()
{
    std::lock_guard<std::mutex> lock(io_mutex);
    if(initialized) return true;
    if(profile.transport == Transport::Realtek)
    {
        if(profile.authentication && !SelectKey()) return false;
    }
    else if(profile.transport == Transport::Legacy || profile.pid == 0x1013)
    {
        std::vector<unsigned char> packet(65, 0xFF);
        packet[0] = 0; packet[1] = 0x95; packet[2] = packet[3] = packet[4] = 0;
        if(!WriteReport(packet)) return Fail();
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if(profile.transport == Transport::Microchip)
        {
            /* Preserve the existing AW3225QF initialization sequence. */
            if(!WriteReport(DDCReport(Transport::Microchip, {0x51,0x85,0x01,0xFE,0x03,0x00,0x06,0x40}))) return Fail();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
            packet.assign(65, 0);
            packet[1] = 0x93; packet[2] = 0x37; packet[3] = 0x12;
            if(!WriteReport(packet)) return Fail();
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    else
    {
        /* Initialization query used by the existing SignalRGB Gen2 driver. */
        std::vector<unsigned char> packet(65, 0);
        const unsigned char query[] = {0,0x92,0x37,5,0,0x51,0x82,0xD0,0xF4,0x99};
        std::copy(std::begin(query), std::end(query), packet.begin());
        if(!WriteReport(packet)) return Fail();
    }
    initialized = true;
    next_write = std::chrono::steady_clock::now() + std::chrono::milliseconds(profile.delay_ms);
    return true;
}

bool AlienwareMonitorController::SendColor(unsigned char mask, unsigned char r, unsigned char g, unsigned char b)
{
    std::lock_guard<std::mutex> lock(io_mutex);
    if(!initialized || mask == 0 || (mask & ~profile.AllZones()) != 0) return false;
    if(std::chrono::steady_clock::now() < retry_after) return false;
    std::this_thread::sleep_until(next_write);
    if(profile.authentication)
    {
        if(key_index < 0 && !SelectKey()) return false;
        /* A fresh challenge is required before each color, not just once at open. */
        if(!Authenticate(static_cast<unsigned int>(key_index))) return Fail();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
    if(!WriteReport(ColorReport(profile.transport, mask, r, g, b))) return Fail();
    next_write = std::chrono::steady_clock::now() + std::chrono::milliseconds(profile.delay_ms);
    return true;
}
