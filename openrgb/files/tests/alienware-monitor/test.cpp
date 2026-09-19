/* SPDX-License-Identifier: GPL-2.0-or-later */
#include <cassert>
#include <fstream>
#include <iostream>
#include <set>
#include <nlohmann/json.hpp>
#include "AlienwareMonitorController.h"
#include "StringUtils.h"

using namespace AlienwareMonitor;

/* These are link-time HID stubs. The test binary cannot open a USB device. */
struct hid_device_
{
    std::vector<std::vector<unsigned char>> writes;
    int read_length = 193;
    bool short_write = false;
    unsigned int reads = 0;
    unsigned int accepted_key = 0;
    bool reject_probe = false;
    bool authenticated = false;
    std::array<unsigned char, 16> token = {{0xBC,0x57,0xDD,0xAA,0x09,0xB1,0x23,0x0D,0xBD,0xFB,0x77,0x61,0x80,0xA0,0x28,0xEC}};
};

extern "C" int HID_API_CALL hid_write(hid_device* dev, const unsigned char* data, size_t length)
{
    dev->writes.emplace_back(data, data + length);
    if(dev->short_write) return static_cast<int>(length) - 1;
    if(length == 193 && data[2] == 0xE1 && data[3] == 2)
    {
        std::array<unsigned char, 8> expected = {};
        assert(GenerateKey(dev->token.data(), 16, OEMKeys()[dev->accepted_key], expected));
        dev->authenticated = std::equal(expected.begin(), expected.end(), data + 65);
    }
    if(length == 193 && data[2] == 0xC6 && data[7] == 5 && (dev->reject_probe || !dev->authenticated)) return -1;
    return static_cast<int>(length);
}
extern "C" int HID_API_CALL hid_get_input_report(hid_device* dev, unsigned char* data, size_t length)
{
    ++dev->reads;
    assert(length == 193 && data[0] == 0);
    std::copy(dev->token.begin(), dev->token.end(), data + 1);
    return dev->read_length;
}
extern "C" int HID_API_CALL hid_get_serial_number_string(hid_device*, wchar_t*, size_t) { return -1; }
extern "C" void HID_API_CALL hid_close(hid_device*) {}
std::string StringUtils::wchar_to_string(const wchar_t*) { return "mock"; }

static std::vector<unsigned char> Hex(const std::string& value)
{
    std::vector<unsigned char> bytes;
    assert(value.size() % 2 == 0);
    for(size_t i = 0; i < value.size(); i += 2)
        bytes.push_back(static_cast<unsigned char>(std::stoul(value.substr(i, 2), nullptr, 16)));
    return bytes;
}

static void Fixtures(const char* filename)
{
    std::ifstream file(filename);
    assert(file.good());
    nlohmann::json data; file >> data;
    for(const auto& color : data["colors"])
    {
        const std::vector<unsigned char> rgb = color["rgb"];
        const auto report = ColorReport(Transport::Realtek, color["mask"], rgb[0], rgb[1], rgb[2]);
        if(report != Hex(color["hid_report_hex"]))
        {
            std::cerr << "Color fixture mismatch: " << color["source"] << " frame " << color["frame"] << '\n';
            std::abort();
        }
    }
    std::set<std::string> parities;
    for(const auto& auth : data["auth"])
    {
        const auto token = Hex(auth["token_hex"]);
        const auto response = Hex(auth["response_hex"]);
        std::array<unsigned char, 8> key = {};
        assert(GenerateKey(token.data(), token.size(), OEMKeys()[auth["key_index"].get<size_t>()], key));
        assert(std::equal(key.begin(), key.end(), response.begin()));
        parities.insert(auth["parity"]);
    }
    assert(parities.size() == 2);
    std::cout << data["colors"].size() << " captured color reports, " << data["auth"].size() << " captured challenge/response pairs: PASS\n";
}

static void ProfilesAndBounds()
{
    std::set<unsigned int> ids;
    unsigned int auth_count = 0;
    for(const Profile& profile : Profiles())
    {
        assert(ids.insert((profile.vid << 16) | profile.pid).second);
        unsigned char mask = 0;
        assert(!profile.zones.empty());
        for(const Zone& zone : profile.zones)
        {
            assert(zone.mask && !(zone.mask & (zone.mask - 1)) && !(mask & zone.mask));
            mask |= zone.mask;
        }
        assert(mask == profile.AllZones());
        if(profile.authentication) { ++auth_count; assert(profile.transport == Transport::Realtek); }
    }
    assert(Profiles().size() == 23 && auth_count == 4);
    assert(!FindProfile(0x187C, 0x1011) && !FindProfile(0x187C, 0x1012) && !FindProfile(0xFFFF, 0x101D));
    assert(FindProfile(0x187C, 0x101D)->AllZones() == 9);
    assert(FindProfile(0x187C, 0x100B)->AllZones() == 15);
    assert(DDCReport(Transport::Realtek, std::vector<unsigned char>(129)).empty());
    assert(DDCReport(Transport::Microchip, std::vector<unsigned char>(61)).empty());
    assert(DDCReport(Transport::Realtek, {}).empty());
    std::array<unsigned char, 16> token = {};
    std::array<unsigned char, 8> key = {};
    for(size_t n = 0; n < 16; ++n) assert(!GenerateKey(token.data(), n, OEMKeys()[0], key));
    assert(!GenerateKey(nullptr, 16, OEMKeys()[0], key));
    assert(!GenerateKey(token.data(), 16, {0x11}, key));
    for(const auto& encoded : OEMKeys()) assert(GenerateKey(token.data(), 16, encoded, key));
    const auto legacy = ColorReport(Transport::Legacy, 0x0F, 0x12, 0x34, 0x56);
    assert(legacy.size() == 65 && std::equal(legacy.begin(), legacy.begin() + 10, Hex("0092480500040f123456").begin()));
    assert(std::all_of(legacy.begin() + 10, legacy.end(), [](unsigned char v) { return v == 0xFF; }));
    const auto micro = ColorReport(Transport::Microchip, 8, 0x12, 0x34, 0x56);
    assert(micro.size() == 65 && std::equal(micro.begin(), micro.begin() + 14, Hex("0092370a005187d0040812345664").begin()));
    assert(micro[14] == 0x70); // 6E ^ 51 ^ 87 ^ D0 ^ 04 ^ 08 ^ 12 ^ 34 ^ 56 ^ 64
    assert(std::all_of(micro.begin() + 15, micro.end(), [](unsigned char v) { return v == 0xFF; }));
}

static void TransportFailuresAndIsolation()
{
    Profile noauth = *FindProfile(0x187C, 0x101D); noauth.delay_ms = 0;
    hid_device_ a;
    AlienwareMonitorController monitor(&a, "mock-a", noauth);
    assert(monitor.Initialize() && a.writes.empty());
    assert(monitor.SendColor(9, 1, 2, 3) && a.reads == 0 && a.writes.size() == 1);
    assert(!monitor.SendColor(2, 1, 2, 3));
    a.short_write = true;
    assert(!monitor.SendColor(9, 0, 0, 0));
    size_t failed_count = a.writes.size();
    assert(!monitor.SendColor(9, 0, 0, 0) && a.writes.size() == failed_count);

    Profile auth = *FindProfile(0x187C, 0x100E); auth.delay_ms = 0;
    hid_device_ b; b.read_length = 16;
    AlienwareMonitorController truncated(&b, "mock-b", auth);
    assert(!truncated.Initialize() && b.reads == 5);
    assert(std::all_of(b.writes.begin(), b.writes.end(), [](const auto& p) { return p[2] == 0xE1 && p[3] == 1; }));

    hid_device_ c; c.accepted_key = 1;
    AlienwareMonitorController fallback(&c, "mock-c", auth);
    assert(fallback.Initialize() && c.reads == 2); // rejected key 0, accepted key 1
    assert(fallback.SendColor(0x0B, 0, 255, 0) && c.reads == 3);
    assert(c.writes.back()[2] == 0xC6 && c.writes.back()[7] == 10);
    hid_device_ d;
    AlienwareMonitorController independent(&d, "mock-d", auth);
    assert(independent.Initialize() && d.reads == 1); // no shared key index
    c.read_length = -1;
    assert(!fallback.SendColor(1, 255, 0, 0));
    assert(c.writes.back()[2] == 0xE1 && c.writes.back()[3] == 1); // no color after failed read
    hid_device_ e; e.reject_probe = true;
    AlienwareMonitorController reject(&e, "mock-e", auth);
    assert(!reject.Initialize() && e.reads == 5);
    hid_device_ short_prefetch; short_prefetch.short_write = true;
    AlienwareMonitorController short_auth(&short_prefetch, "mock-short", auth);
    assert(!short_auth.Initialize() && short_prefetch.reads == 0 && short_prefetch.writes.size() == 5);

    Profile micro = *FindProfile(0x187C, 0x1013); micro.delay_ms = 0;
    hid_device_ f;
    AlienwareMonitorController existing(&f, "mock-f", micro);
    assert(existing.Initialize() && f.writes.size() == 3);
    assert(f.writes[0][1] == 0x95 && f.writes[1][3] == 8 && f.writes[2][1] == 0x93);
    for(const auto& write : f.writes) assert(write.size() == 65 && write[0] == 0);
    assert(existing.SendColor(0x0B, 0x12, 0x34, 0x56) && f.reads == 0);
    Profile legacy = *FindProfile(0x0424, 0x274A); legacy.delay_ms = 0;
    hid_device_ g;
    AlienwareMonitorController classic(&g, "mock-g", legacy);
    assert(classic.Initialize() && g.writes.size() == 1 && g.writes[0][1] == 0x95);
    assert(classic.SendColor(15, 0, 0, 255) && g.writes.back()[2] == 0x48 && g.reads == 0);
    Profile modern_micro = *FindProfile(0x187C, 0x100B); modern_micro.delay_ms = 0;
    hid_device_ h;
    AlienwareMonitorController modern(&h, "mock-h", modern_micro);
    assert(modern.Initialize() && h.writes.size() == 1 && h.writes[0][8] == 0xF4);
}

int main(int argc, char** argv)
{
    assert(argc == 2);
    Fixtures(argv[1]);
    ProfilesAndBounds();
    TransportFailuresAndIsolation();
    std::cout << "Profiles, report lengths, malformed inputs, failed/partial transfers, bounded authentication, per-device key isolation: PASS\n";
}
