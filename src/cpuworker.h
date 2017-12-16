#ifndef CPUWORKER_H_INCLUDED
#define CPUWORKER_H_INCLUDED

#include <vector>
#include <memory>
#include <thread>

#include "gate.h"

namespace NextMiner {
    class CPUWorker {
        public:
            CPUWorker();
            ~CPUWorker();

            std::vector<uint32_t> scanHash(const std::vector<uint8_t>& headerBytes,
                                           const uint32_t startNonce,
                                           const uint32_t endNonce,
                                           const uint64_t target);

        private:
            std::unique_ptr<std::thread> workerThread;
            bool running;

            uint32_t startNonce;
            uint32_t endNonce;
            uint64_t target;

            std::vector<uint32_t> solutions;
            std::vector<uint8_t> headerBytes;
            std::unique_ptr<NextMiner::Gate> workNotify;
            std::unique_ptr<NextMiner::Gate> completeNotify;

    };
}

#endif // CPUWORKER_H_INCLUDED
