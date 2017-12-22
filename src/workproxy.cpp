#include "workproxy.h"

NextMiner::WorkProxy::WorkProxy(GetWork* workSource,
                                const unsigned int weight) {
    totalWeight = 0;
    counter = 0;
    addWorkSource(workSource, weight);
}

NextMiner::WorkProxy::~WorkProxy() {

}

void NextMiner::WorkProxy::addWorkSource(NextMiner::GetWork* workSource,
                                         const unsigned int weight) {
    workSources.insert(std::make_pair(workSource, weight));
    totalWeight += weight;

    workSource->registerWorker([&](const bool discardOld){
        if(getCurrentWorkSource() == workSource) {
            callback(discardOld);
        }
    });
}

NextMiner::GetWork* NextMiner::WorkProxy::getCurrentWorkSource() {
    const unsigned int slot = counter % totalWeight;
    unsigned int current = 0;
    for(const auto& workSource : workSources) {
        if(current + workSource.second >= slot) {
            return workSource.first;
        }

        current += workSource.second;
    }

    throw std::runtime_error("Fatal logic error");
}

void NextMiner::WorkProxy::callback(const bool discardOld) {
    for(const auto& cb : callbacks) {
        cb(discardOld);
    }
}

void NextMiner::WorkProxy::registerWorker(std::function<void(const bool)> cb) {
    callbacks.push_back(cb);
}

std::unique_ptr<NextMiner::GetWork::Work> NextMiner::WorkProxy::getWork() {
    counter++;
    auto workSource = getCurrentWorkSource();
    return std::unique_ptr<GetWork::Work>(new ProxyJob(workSource->getWork().get(),
                                                       workSource));
}

std::tuple<bool, std::string> NextMiner::WorkProxy::submitWork(const NextMiner::GetWork::Work& work) {
    const auto& proxyJob = dynamic_cast<const ProxyJob&>(work);

    return proxyJob.workSource->submitWork(work);
}

void NextMiner::WorkProxy::suggestDifficulty(const double difficulty) {
    for(const auto& workSource : workSources) {
        workSource.first->suggestDifficulty(difficulty);
    }
}

NextMiner::WorkProxy::ProxyJob::ProxyJob() {
    workSource = nullptr;
}

NextMiner::WorkProxy::ProxyJob::ProxyJob(NextMiner::GetWork::Work* work,
                                         NextMiner::GetWork* workSource) {
    this->work = work->clone();
    this->workSource = workSource;
}

NextMiner::WorkProxy::ProxyJob::~ProxyJob() {

}

void NextMiner::WorkProxy::ProxyJob::operator=(const NextMiner::GetWork::Work& other) {
    const auto& otherProxyJob = dynamic_cast<const ProxyJob&>(other);

    this->work = other.clone();

    this->workSource = otherProxyJob.workSource;
}

std::vector<uint8_t> NextMiner::WorkProxy::ProxyJob::getBytes() const {
    return work->getBytes();
}

void NextMiner::WorkProxy::ProxyJob::setNonce(const uint32_t nonce) {
    work->setNonce(nonce);
}

uint32_t NextMiner::WorkProxy::ProxyJob::getTarget() const {
    return work->getTarget();
}

void NextMiner::WorkProxy::ProxyJob::newExtranonce2() {
    work->newExtranonce2();
}

std::unique_ptr<NextMiner::GetWork::Work> NextMiner::WorkProxy::ProxyJob::clone() const {
    return std::unique_ptr<GetWork::Work>(new ProxyJob(work.get(), workSource));
}
