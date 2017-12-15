#ifdef __linux
#include <pthread.h>
#endif // __linux

#include "Lyra2RE/Lyra2RE.h"

#include "util.h"
#include "stratumclient.h"

int main() {
    std::unique_ptr<NextMiner::Log> log(new NextMiner::Log);
    std::unique_ptr<NextMiner::GetWork> workSource(
        new NextMiner::StratumClient("localhost", 9171,
                                     "VnUxCjoLSv1U5pxegJBGxX8anDnop77DWz", "x",
                                     log.get()));

    // Wait until the workSource has work for us
    while(!workSource->getWork()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    bool stale = false;
    workSource->registerWorker([&](const bool discardOld){
        if(discardOld) {
            stale = true;
        }
    });

    workSource->suggestDifficulty(2000);

    bool running = true;
    while(running) {
        auto work = workSource->getWork();
        stale = false;
        const uint64_t target = EndSwap(*reinterpret_cast<uint64_t*>(&HexToBytes(CompactToTarget(work->getTarget()).ToString())[0]));
        uint64_t lowest = 0xffffffffffffffff;
        std::vector<uint8_t> lowestHash;

        log->printf("Trying to get: " + std::to_string(target) + " " + CompactToTarget(work->getTarget()).ToString(),
                    NextMiner::Log::Severity::Notice);

        std::vector<uint8_t> outputBuffer;
        outputBuffer.resize(32);

        uint32_t nonce = 0;

        auto headerBytes = work->getBytes();

        #ifdef __linux
        cpu_set_t cpuset;
        CPU_ZERO(&cpuset);
        CPU_SET(0, &cpuset);
        #endif // __linux

        std::thread minerThread([&]{

            /* Believe it or not, C++11 is _still_ a low level language,
               thus we have to set affinity after the thread has started.
               Sleep for a while before starting to give us time.
            */
            std::this_thread::sleep_for(std::chrono::milliseconds(1));

            uint64_t startTime = std::chrono::duration_cast
                                 <std::chrono::milliseconds>(
                                 std::chrono::system_clock::now()
                                 .time_since_epoch()).count();

            auto printHashrate = [&]{
                const uint64_t endTime = std::chrono::duration_cast
                                         <std::chrono::milliseconds>(
                                         std::chrono::system_clock::now()
                                         .time_since_epoch()).count();
                const auto rate = nonce / (double(endTime - startTime) / 1000);
                const double hashes = rate / 1000;
                log->printf("Miner: HR: "
                            + std::to_string(hashes)
                            + " KH/s",
                            NextMiner::Log::Severity::Notice);
                startTime = std::chrono::duration_cast
                            <std::chrono::milliseconds>(
                            std::chrono::system_clock::now()
                            .time_since_epoch()).count();
            };

            while(!stale) {
                *reinterpret_cast<uint32_t*>(&headerBytes[76]) = EndSwap(++nonce);

                lyra2re2_hash(reinterpret_cast<const char*>(&headerBytes[0]),
                              reinterpret_cast<char*>(&outputBuffer[0]));

                const uint64_t result = *reinterpret_cast<uint64_t*>(&outputBuffer[24]);
                if(result < target) {
                    std::thread([&, work = move(work), nonce]{
                        work->setNonce(nonce);
                        const auto res = workSource->submitWork(*work);
                        log->printf("Found share!! Valid: " +
                                std::to_string(std::get<0>(res)) +
                                " " +
                                std::get<1>(res),
                                NextMiner::Log::Severity::Notice);
                    }).detach();
                    break;
                }

                if(result < lowest) {
                    lowest = result;
                    lowestHash = outputBuffer;
                }

                if(nonce % 1000000 == 0) {
                    log->printf(std::to_string(lowest) + " " + BytesToHex(lowestHash),
                                NextMiner::Log::Severity::Notice);
                }

                if(nonce == std::numeric_limits<uint32_t>::max()) {
                    work->newExtranonce2();
                    printHashrate();
                }
            }

            printHashrate();
        });

        // There's also no way to do cross platform thread properties in C++ yet
        #ifdef __linux
        const int rcAffinity = pthread_setaffinity_np(minerThread.native_handle(),
                                                      sizeof(cpu_set_t),
                                                      &cpuset);

        if(rcAffinity != 0) {
            log->printf("Couldn't set thread affinity",
                        NextMiner::Log::Severity::Warning);
        }
        #endif // __linux

        minerThread.join();
    }



    return 0;
}
