#include "cpuworker.h"

#include "util.h"
#include "Lyra2RE/Lyra2RE.h"

NextMiner::CPUWorker::CPUWorker() {
    running = true;

    workNotify.reset(new Gate());
    completeNotify.reset(new Gate());

    workerThread.reset(new std::thread([&]{
        std::vector<uint8_t> outputBuffer;
        outputBuffer.resize(32);

        while(running) {
            workNotify->wait();
            workNotify->reset();

            // Maybe it's time to quit rather than work
            if(!running) {
                completeNotify->notify();
                break;
            }

            for(uint32_t i = startNonce; i < endNonce; i++) {
                *reinterpret_cast<uint32_t*>(&headerBytes[76]) = EndSwap(i);

                lyra2re2_hash(reinterpret_cast<const char*>(&headerBytes[0]),
                              reinterpret_cast<char*>(&outputBuffer[0]));

                const uint64_t result = *reinterpret_cast<uint64_t*>(&outputBuffer[24]);
                if(result < target) {
                    solutions.push_back(i);
                }
            }

            completeNotify->notify();
        }
    }));
}

NextMiner::CPUWorker::~CPUWorker() {
    running = false;
    workNotify->notify();
    workerThread->join();
}

std::vector<uint32_t> NextMiner::CPUWorker::scanHash(const std::vector<uint8_t>& headerBytes,
                                                     const uint32_t startNonce,
                                                     const uint32_t endNonce,
                                                     const uint64_t target) {
    // Notify thread of new work
    this->headerBytes = headerBytes;
    this->startNonce = startNonce;
    this->endNonce = endNonce;
    this->target = target;
    solutions.clear();
    workNotify->notify();

    // Wait for completion and return
    completeNotify->wait();
    completeNotify->reset();
    return solutions;
}
