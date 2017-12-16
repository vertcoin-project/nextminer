#include "gate.h"

NextMiner::Gate::Gate() {
    gate_open = false;
}

void NextMiner::Gate::notify() {
    std::unique_lock<std::mutex> lock(m);
    gate_open = true;
    cv.notify_all();
}

void NextMiner::Gate::wait() const {
    std::unique_lock<std::mutex> lock(m);
    cv.wait(lock, [this]{ return gate_open; });
}

void NextMiner::Gate::reset() {
    std::unique_lock<std::mutex> lock(m);
    gate_open = false;
}
