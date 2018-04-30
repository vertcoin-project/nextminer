#include "regtest.h"
#include "util.h"
#include "Lyra2RE/Lyra2RE.h"

#include <algorithm>
#include <sstream>
#include <iomanip>

NextMiner::RegTest::RegTest(const uint32_t target) {
    this->target = target;
}

NextMiner::RegTest::~RegTest() {

}

NextMiner::RegTest::RegTestJob::RegTestJob() {

}

NextMiner::RegTest::RegTestJob::~RegTestJob() {

}

NextMiner::RegTest::RegTestJob::RegTestJob(const uint32_t target) {
    this->target = target;

    generator.seed(std::time(nullptr));

    newExtranonce2();

    nonce = 0;
}

void NextMiner::RegTest::RegTestJob::operator=(const Work& other) {
    const RegTestJob& RegTestOther = dynamic_cast<const RegTestJob&>(other);

    nonce = RegTestOther.nonce;
    target = RegTestOther.target;
    randomHex = RegTestOther.randomHex;
}

void NextMiner::RegTest::RegTestJob::setNonce(const uint32_t nonce) {
    this->nonce = nonce;
}

uint32_t NextMiner::RegTest::RegTestJob::getTarget() const {
    return target;
}

void NextMiner::RegTest::RegTestJob::newExtranonce2() {
    std::stringstream buffer;

    std::uniform_int_distribution<uint32_t> distribution(0, std::numeric_limits<uint32_t>::max());

    for(unsigned int i = 0; i < 76; i += 4) {
        buffer << std::hex << std::setfill('0') << std::setw(8) << distribution(generator);
    }

    randomHex = buffer.str();
}

std::unique_ptr<NextMiner::GetWork::Work> NextMiner::RegTest::RegTestJob::clone() const {
    RegTestJob* job = new RegTestJob;
    *job = *this;

    return std::unique_ptr<NextMiner::GetWork::Work>(job);
}

std::vector<uint8_t> NextMiner::RegTest::RegTestJob::getBytes() const {
    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << EndSwap(nonce);

    const std::string blockHeaderHex = randomHex + ss.str();

    return ReverseWords(HexToBytes(blockHeaderHex));
}

void NextMiner::RegTest::registerWorker(std::function<void(const bool)> cb) {
    std::lock_guard<std::mutex> lock(callbacksLock);
    callbacks.push_back(cb);
}

std::unique_ptr<NextMiner::GetWork::Work> NextMiner::RegTest::getWork() {
    if(currentJob) {
        currentJob->newExtranonce2();
        RegTestJob* job = new RegTestJob;

        jobLock.lock();
        *job = *currentJob;
        jobLock.unlock();

        return std::unique_ptr<GetWork::Work>(job);
    } else {
        return std::unique_ptr<GetWork::Work>();
    }
}

std::tuple<bool, std::string> NextMiner::RegTest::submitWork(const NextMiner::GetWork::Work& work) {
    const RegTestJob& job = dynamic_cast<const RegTestJob&>(work);

    // TODO: move this validation code to the GPU manager code
    // RegTest should be PoW-agnostic
    std::vector<uint8_t> outputBuffer;
    outputBuffer.resize(32);

    const uint64_t target = EndSwap(*reinterpret_cast<uint64_t*>(&HexToBytes(CompactToTarget(work.getTarget()).ToString())[0]));
    auto headerBytes = work.getBytes();

    *reinterpret_cast<uint32_t*>(&headerBytes[76]) = EndSwap(job.nonce);

    lyra2re2_hash(reinterpret_cast<const char*>(&headerBytes[0]),
                  reinterpret_cast<char*>(&outputBuffer[0]));

    const uint64_t result = *reinterpret_cast<uint64_t*>(&outputBuffer[24]);

    if(result > target) {
        return std::make_tuple(false, "Ignoring job with hash > target hash: " +
                                      std::to_string(result) + " target: " +
                                      std::to_string(target) + " nonce: " +
                                      std::to_string(job.nonce));
    } else {
        RegTestJob* newWork = new RegTestJob(target);

        const auto hexBytes = BytesToHex(newWork->getBytes());

        jobLock.lock();
        currentJob.reset(newWork);
        jobLock.unlock();

        callbacksLock.lock();
        for(const auto& cb : callbacks) {
            cb(true);
        }
        callbacksLock.unlock();

        return std::make_tuple(true, "");
    }
}

void NextMiner::RegTest::suggestDifficulty(const double difficulty) {

}
