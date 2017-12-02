#include "Lyra2RE/Lyra2RE.h"

#include "util.h"
#include "stratumclient.h"

int main() {
    std::unique_ptr<NextMiner::Log> log(new NextMiner::Log);
    std::unique_ptr<NextMiner::GetWork> workSource(
        new NextMiner::StratumClient("18.250.0.71", 9271, "jamesl222", "x", log.get()));

    // Wait until the workSource has work for us
    auto work = workSource->getWork();
    while(!work) {
        work = workSource->getWork();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    bool stale = false;
    workSource->registerWorker([&](const bool discardOld){
        stale = true;
    });

    bool running = true;
    while(running) {
        work = workSource->getWork();
        stale = false;
        const uint256 target = CompactToTarget(work->getTarget());
        uint256 lowest("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

        log->printf("Trying to get: " + target.ToString(),
                    NextMiner::Log::Severity::Notice);

        std::vector<uint8_t> outputBuffer;
        outputBuffer.resize(32);

        uint32_t nonce = 0;

        auto headerBytes = work->getBytes();

        while(!stale) {
            *reinterpret_cast<uint32_t*>(&headerBytes[76]) = EndSwap(++nonce);

            lyra2re2_hash(reinterpret_cast<const char*>(&headerBytes[0]),
                          reinterpret_cast<char*>(&outputBuffer[0]));

            const uint256 result = CBigNum(outputBuffer).getuint256();

            if(nonce % (100000) == 0) {
                log->printf(lowest.ToString(), NextMiner::Log::Severity::Notice);
            }

            if(result < target) {
                work->setNonce(nonce);
                const auto res = workSource->submitWork(*work);
                log->printf("Found valid share!! " + std::get<1>(res), NextMiner::Log::Severity::Notice);
                running = false;
                break;
            }

            if(result < lowest) {
                lowest = result;
            }

            if(nonce == std::numeric_limits<uint32_t>::max()) {
                work->newExtranonce2();
            }
        }
    }

    return 0;
}
