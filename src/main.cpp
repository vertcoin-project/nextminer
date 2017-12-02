#include "Lyra2RE/Lyra2RE.h"

#include "util.h"
#include "stratumclient.h"

int main() {
    std::unique_ptr<NextMiner::Log> log(new NextMiner::Log);
    std::unique_ptr<NextMiner::GetWork> workSource(
        new NextMiner::StratumClient("18.250.0.71", 9271, "jamesl222", "x", log.get()));

    auto work = workSource->getWork();
    while(!work) {
        work = workSource->getWork();
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    const uint256 target = CompactToTarget(work->getTarget());
    uint256 lowest("ffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffffff");

    log->printf("Trying to get: " + target.ToString(),
                NextMiner::Log::Severity::Notice);

    std::vector<uint8_t> outputBuffer;
    outputBuffer.resize(32);

    uint32_t nonce = 0;

    while(true) {
        work->setNonce(nonce++);
        const auto headerBytes = work->getBytes();

        lyra2re2_hash(reinterpret_cast<const char*>(&headerBytes[0]),
                      reinterpret_cast<char*>(&outputBuffer[0]));

        const uint256 result = CBigNum(outputBuffer).getuint256();

        if(nonce % (1000) == 0) {
            log->printf(lowest.ToString(), NextMiner::Log::Severity::Notice);
        }

        if(result < target) {
            const auto res = workSource->submitWork(*work);
            log->printf("Found valid share!! " + std::get<1>(res), NextMiner::Log::Severity::Notice);
            break;
        }

        if(result < lowest) {
            lowest = result;
        }
    }

    return 0;
}
