#include "cpumanager.h"

#include "cpuworker.h"
#include "util.h"

NextMiner::CPUManager::CPUManager(NextMiner::GetWork* workSource, NextMiner::Log* log) {
    this->workSource = workSource;
    this->log = log;
    running = true;
    stale = false;

    this->workSource->registerWorker([&](const bool discardOld){
        if(discardOld) {
            stale = true;
        }
    });

    workers.push_back(std::make_unique<CPUWorker>());

    managerThread.reset(new std::thread([this]{
        while(running) {
            uint64_t nonce = 0;
            const uint64_t startTime = std::chrono::duration_cast
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
                this->log->printf("Miner: HR: "
                            + std::to_string(hashes)
                            + " KH/s",
                            NextMiner::Log::Severity::Notice);
            };

            auto work = this->workSource->getWork();
            stale = false;
            const uint64_t target = EndSwap(*reinterpret_cast<uint64_t*>(&HexToBytes(CompactToTarget(work->getTarget()).ToString())[0]));

            auto headerBytes = work->getBytes();

            const uint32_t workSize = 50000;

            for(uint32_t i = 0; i < std::numeric_limits<uint32_t>::max() && !stale; i += workSize) {
                const auto res = workers[0]->scanHash(headerBytes, i, i + workSize, target);
                if(!res.empty()) {
                    for(const auto solution : res) {
                        work->setNonce(solution);
                        const auto submitRes = this->workSource->submitWork(*work);
                        this->log->printf("Found share!! Valid: " +
                                std::to_string(std::get<0>(submitRes)) +
                                " " +
                                std::get<1>(submitRes),
                                NextMiner::Log::Severity::Notice);
                    }
                }
                nonce += workSize;
            }

            printHashrate();
            work->newExtranonce2();
        }
    }));
}

NextMiner::CPUManager::~CPUManager() {
    running = false;
    managerThread->join();
}
