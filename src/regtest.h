#ifndef REGTEST_H_INCLUDED
#define REGTEST_H_INCLUDED

#include <random>

#include "getwork.h"
#include "log.h"

namespace NextMiner {
    class RegTest : public GetWork {
        public:
            RegTest(const uint32_t target);

            virtual ~RegTest();

            class RegTestJob : public Work {
                friend class RegTest;

                public:
                    RegTestJob(const uint32_t target);
                    RegTestJob();
                    ~RegTestJob();

                    virtual void operator=(const Work& other);

                    virtual std::vector<uint8_t> getBytes() const;

                    virtual void setNonce(const uint32_t nonce);

                    virtual uint32_t getTarget() const;

                    virtual void newExtranonce2();

                    virtual std::unique_ptr<Work> clone() const;

                protected:
                    uint32_t nonce;
                    uint32_t target;
                    std::string randomHex;

                    std::default_random_engine generator;
            };

            virtual void registerWorker(std::function<void(const bool)> cb);

            virtual std::unique_ptr<Work> getWork();

            virtual std::tuple<bool, std::string> submitWork(const Work& work);

            virtual void suggestDifficulty(const double difficulty);

        private:
            std::vector<std::function<void(const bool)>> callbacks;
            std::mutex callbacksLock;

            std::unique_ptr<RegTestJob> currentJob;
            std::mutex jobLock;

            uint32_t target;
    };
}

#endif
