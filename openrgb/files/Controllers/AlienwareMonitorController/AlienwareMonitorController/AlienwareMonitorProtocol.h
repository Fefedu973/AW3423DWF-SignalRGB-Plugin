/* SPDX-License-Identifier: GPL-2.0-or-later
 * Authentication derived from Ferréol DUBOIS COLI's AW3423DWF driver.
 * Microchip framing derived from Adam Honse's Alienware monitor driver.
 */
#pragma once

#include <algorithm>
#include <array>
#include <cstddef>
#include <vector>

namespace AlienwareMonitor
{
enum class Transport { Legacy, Microchip, Realtek };

struct Zone
{
    const char* name;
    unsigned char mask;
};

struct Profile
{
    unsigned short vid;
    unsigned short pid;
    const char* name;
    Transport transport;
    bool authentication;
    unsigned int preferred_key;
    unsigned int delay_ms;
    std::vector<Zone> zones;

    unsigned char AllZones() const
    {
        unsigned char mask = 0;
        for(const Zone& zone : zones) mask |= zone.mask;
        return mask;
    }
};

const std::vector<Profile>& Profiles();
const Profile* FindProfile(unsigned short vid, unsigned short pid);

inline unsigned char Checksum(const unsigned char* bytes, size_t length)
{
    unsigned char sum = 0x6E;
    for(size_t i = 0; i < length; ++i) sum ^= bytes[i];
    return sum;
}

/* HIDAPI includes the zero report ID. The USB payload is 64/192 bytes. */
inline std::vector<unsigned char> DDCReport(Transport transport, const std::vector<unsigned char>& body, unsigned char speed = 2)
{
    if(body.empty() || body.size() > (transport == Transport::Realtek ? 128u : 60u)) return {};
    std::vector<unsigned char> report(transport == Transport::Realtek ? 193 : 65,
                                      transport == Transport::Realtek ? 0 : 0xFF);
    report[0] = 0;
    size_t offset;
    if(transport == Transport::Realtek)
    {
        report[1] = 0x40; report[2] = 0xC6;
        report[7] = static_cast<unsigned char>(body.size());
        report[9] = 0x6E; report[11] = 0x80 | speed;
        offset = 65;
    }
    else
    {
        report[1] = 0x92; report[2] = 0x37;
        report[3] = static_cast<unsigned char>(body.size()); report[4] = 0;
        offset = 5;
    }
    std::copy(body.begin(), body.end(), report.begin() + offset);
    return report;
}

inline std::vector<unsigned char> ColorReport(Transport transport, unsigned char mask, unsigned char r, unsigned char g, unsigned char b)
{
    if(transport == Transport::Legacy)
    {
        std::vector<unsigned char> report(65, 0xFF);
        report[0] = 0; report[1] = 0x92; report[2] = 0x48;
        report[3] = 5; report[4] = 0; report[5] = 4;
        report[6] = mask; report[7] = r; report[8] = g; report[9] = b;
        return report;
    }
    std::vector<unsigned char> body = {0x51, 0x87, 0xD0, 0x04, mask, r, g, b, 0x64, 0};
    body.back() = Checksum(body.data(), body.size() - 1);
    return DDCReport(transport, body);
}

inline const std::vector<std::vector<unsigned char>>& OEMKeys()
{
    /* AWCC FxDisplayCommon 6.14.24.0; index 1 also matches historical AW2724DM captures. */
    static const std::vector<std::vector<unsigned char>> keys = {
        {0xF5,0x3F,0xC1,0x39,0x44,0x3A,0x31,0x79,0x0D,0xB1,0x82,0x76},
        {0x7E,0x7F,0x18,0x00,0x2B,0x33,0xFB,0xFD,0xE3,0x64,0xD0,0xC0,0x04,0xB0,0xDA,0xC7,0xB5},
        {0x66,0xCB,0xA3,0x70,0x08,0x3A,0xE4,0x1D,0x34,0x23,0x5E,0x24,0xCF,0xF6,0x0D,0x57},
        {0x33,0x3A,0x3B,0x14,0x09,0x85,0xEA,0x27,0x7D,0xCD,0x6A,0xE8,0x5F,0x85,0x70},
        {0xB9,0xF1,0xB4,0xFB,0x0F,0xF5,0x36,0xEF,0x77,0xAB,0xF7,0x0D,0x72}
    };
    return keys;
}

inline bool GenerateKey(const unsigned char* token, size_t token_length, const std::vector<unsigned char>& encoded, std::array<unsigned char, 8>& key)
{
    if(!token || token_length != 16) return false;
    size_t count = 0;
    for(size_t pos = 0; pos < encoded.size() && count < key.size(); ++pos)
    {
        const unsigned int run = (encoded[pos] & 1) + ((encoded[pos] & 0x10) >> 4);
        for(unsigned int n = 0; n < run && count < key.size(); ++n, ++pos)
        {
            if(pos + 1 >= encoded.size()) return false;
            key[count++] = encoded[pos] ^ encoded[pos + 1];
        }
    }
    if(count != key.size()) return false;
    unsigned int bits = (token[0] << 8) | token[15];
    unsigned int parity = 0;
    while(bits) { parity ^= bits & 1; bits >>= 1; }
    const unsigned int half = parity ? 0 : 8;
    for(size_t i = 0; i < key.size(); ++i) key[i] ^= token[half + i];
    const unsigned int index = token[parity ? 14 : 6] & 7;
    key[index] ^= token[(parity ? 8 : 0) + index];
    return true;
}
}
