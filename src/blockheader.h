#ifndef BLOCKHEADER_H_INCLUDED
#define BLOCKHEADER_H_INCLUDED

#include <array>

#include <jsoncpp/json/value.h>

namespace NextMiner {
    class BlockHeader {
        public:
            BlockHeader(const std::array<uint8_t, 80>& headerBytes);
            BlockHeader(const Json::Value& headerJson);
            ~BlockHeader();

            std::array<uint8_t, 80> getBytes() const;

            uint32_t getTarget() const;

            void setNonce(const uint32_t nonce);
    };
}

#endif // BLOCKHEADER_H_INCLUDED
