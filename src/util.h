#ifndef UTIL_H_INCLUDED
#define UTIL_H_INCLUDED

#include <vector>
#include <string>
#include <array>
#include <algorithm>

#include "sha256.h"

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
        ss << std::setw(2) << v;
    }

    return ss.str();
}

template <class T>
T EndSwap(T& objp) {
    unsigned char* memp = reinterpret_cast<unsigned char*>(&objp);
    T returning;
    std::reverse_copy(memp, memp + sizeof(T), &returning);

    return returning;
}

std::vector<uint8_t> ReverseBytes(const std::vector<uint8_t>& data) {
    return std::vector<uint8_t>(data.rbegin(), data.rend());
}

std::vector<uint8_t> DoubleSHA256(const std::vector<uint8_t>& data) {
    CSHA256 sha;
    std::vector<uint8_t> returning;

    sha.Write(&data[0], data.size());
    sha.Finalize(&returning[0]);

    sha.Reset();

    sha.Write(&returning[0], 32);
    sha.Finalize(&returning[0]);

    return returning;
}

#endif // UTIL_H_INCLUDED
