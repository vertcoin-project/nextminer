#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <vector>
#include <string>
#include <array>
#include <algorithm>
#include <iostream>

#include "sha256.h"
#include "bignum.hpp"

std::vector<uint8_t> HexToBytes(const std::string& hex) {
    std::vector<uint8_t> bytes;

    for(unsigned int i = 0; i < hex.length(); i += 2) {
        const std::string byteString = hex.substr(i, 2);
        const uint8_t byte = static_cast<uint8_t>(
                          strtoul(byteString.c_str(), nullptr, 16));
        bytes.push_back(byte);
    }

    return bytes;
}

std::string BytesToHex(const std::vector<uint8_t>& bytes) {
    std::stringstream ss;

    ss << std::hex << std::setfill('0');
    for(const auto v : bytes) {
        ss << std::setw(2) << static_cast<int>(v);
    }

    return ss.str();
}

template <typename T>
T EndSwap(T u){
    static_assert(CHAR_BIT == 8, "CHAR_BIT != 8");

    union {
        T u;
        unsigned char u8[sizeof(T)];
    } source, dest;

    source.u = u;

    for(size_t k = 0; k < sizeof(T); k++) {
        dest.u8[k] = source.u8[sizeof(T) - k - 1];
    }

    return dest.u;
}

std::vector<uint8_t> ReverseBytes(const std::vector<uint8_t>& data) {
    return std::vector<uint8_t>(data.rbegin(), data.rend());
}

std::vector<uint8_t> DoubleSHA256(const std::vector<uint8_t>& data) {
    CSHA256 sha;
    std::vector<uint8_t> returning;
    returning.resize(32);

    sha.Write(&data[0], data.size());
    sha.Finalize(&returning[0]);

    sha.Reset();

    sha.Write(&returning[0], 32);
    sha.Finalize(&returning[0]);

    return returning;
}

std::vector<uint32_t> DiffToTarget(const double& diff) {
    uint64_t m;
    int k;
    double ourDiff = diff;

    for(k = 6; k > 0 && ourDiff > 1.0; k--) {
        ourDiff /= 4294967296.0;
    }
    m = static_cast<uint64_t>(4294901760.0 / ourDiff);

    std::vector<uint32_t> target;
    target.resize(8);

    if(m == 0 && k == 6) {
        std::fill(target.begin(), target.end(), 0xffffffff);
    } else {
        std::fill(target.begin(), target.end(), 0x00000000);
        target[k] = static_cast<uint32_t>(m);
        target[k + 1] = static_cast<uint32_t>(m >> 32);
    }

    return target;
}

uint32_t TargetToCompact(const std::vector<uint32_t>& target) {
    std::vector<uint8_t> byteArray;
    for(unsigned int i = 0; i < target.size() * 4; i++) {
        byteArray.push_back(*(reinterpret_cast<const uint8_t*>(target.data()) + i));
    }

    return CBigNum(byteArray).GetCompact();
}

uint32_t DiffToCompact(const double& diff) {
    return TargetToCompact(DiffToTarget(diff));
}

uint256 CompactToTarget(const uint32_t compact) {
    return CBigNum().SetCompact(compact).getuint256();
}

#endif // UTIL_H_INCLUDED
