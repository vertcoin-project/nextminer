#ifndef GETWORK_H_INCLUDED
#define GETWORK_H_INCLUDED

#include <memory>
#include <vector>
#include <functional>

#include "blockheader.h"

namespace NextMiner {
    class GetWork {
        public:
            GetWork() {};
            virtual ~GetWork() {};

            class Work {
                public:
                    Work() {};
                    virtual ~Work() {};

                    virtual std::vector<uint8_t> getBytes() = 0;

                    virtual void setNonce(const uint32_t nonce) = 0;

                    virtual uint32_t getTarget() = 0;
            };

            virtual void registerWorker(std::function<void(const Work&)> cb) = 0;

            virtual std::unique_ptr<Work> getWork() = 0;

            virtual std::tuple<bool, std::string> submitWork(const Work& work) = 0;

            virtual void suggestTarget(const uint32_t target) = 0;
    };
}

#endif // GETWORK_H_INCLUDED
