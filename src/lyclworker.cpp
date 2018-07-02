#include "lyclworker.h"

#include "util.h"

#include "lycl/AppLyra2REv2.hpp"
#include "lycl/Blake256.hpp"

NextMiner::LyCLWorker::LyCLWorker(lycl::device& device) {
    running = true;

    workNotify.reset(new Gate());
    completeNotify.reset(new Gate());

    workerThread.reset(new std::thread([&]{
        std::vector<uint8_t> outputBuffer;
        outputBuffer.resize(32);

        lycl::AppLyra2REv2 deviceCtx;
        if(!deviceCtx.onInit(device)) {
            deviceCtx.onDestroy();
            throw std::runtime_error("Failed to initialise device!");
        }

        std::vector<uint32_t> m_potentialNonces;
        std::vector<uint32_t> m_nonces;

        uint32_t* pdata = (uint32_t*)&headerBytes[0];
        uint32_t* ptarget = workInfo.target;
        const uint32_t Htarg = ptarget[7];
        
        while(running) {
            workNotify->wait();
            workNotify->reset();

            // Maybe it's time to quit rather than work
            if(!running) {
                completeNotify->notify();
                break;
            }

            lycl::KernelData kernelData;
            memset(&kernelData, 0, sizeof(kernelData));
            
            uint32_t h[8] =
            {
                0x6A09E667, 0xBB67AE85,
                0x3C6EF372, 0xA54FF53A,
                0x510E527F, 0x9B05688C,
                0x1F83D9AB, 0x5BE0CD19
            };
            
            kernelData.in16 = pdata[16];
            kernelData.in17 = pdata[17];
            kernelData.in18 = pdata[18];

            blake256_compress(h, pdata);

            kernelData.uH0 = h[0];
            kernelData.uH1 = h[1];
            kernelData.uH2 = h[2];
            kernelData.uH3 = h[3];
            kernelData.uH4 = h[4];
            kernelData.uH5 = h[5];
            kernelData.uH6 = h[6];
            kernelData.uH7 = h[7];
            kernelData.htArg = Htarg;
            //Log::print(Log::LT_Notice, "Device:%d block:%u,%u,%u,%u,%u,%u,%u,%u", thr_id, h[0], h[1], h[2], h[3], h[4], h[5], h[6], h[7]);
            //-------------------------------------
            // upload data
            deviceCtx.setKernelData(kernelData);

            /*for(uint32_t i = startNonce; i < endNonce; i++) {
                *reinterpret_cast<uint32_t*>(&headerBytes[76]) = EndSwap(i);

                lyra2re2_hash(reinterpret_cast<const char*>(&headerBytes[0]),
                              reinterpret_cast<char*>(&outputBuffer[0]));

                const uint64_t result = *reinterpret_cast<uint64_t*>(&outputBuffer[24]);
                if(result < target) {
                    solutions.push_back(i);
                }
            }*/

            completeNotify->notify();
        }
    }));
}

NextMiner::LyCLWorker::~LyCLWorker() {
    running = false;
    workNotify->notify();
    workerThread->join();
}

std::vector<uint32_t> NextMiner::LyCLWorker::scanHash(const std::vector<uint8_t>& headerBytes,
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
