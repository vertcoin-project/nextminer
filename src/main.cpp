#include "stratumclient.h"

int main() {
    std::shared_ptr<NextMiner::GetWork> workSource(
        new NextMiner::StratumClient("18.250.0.71", 9171, "jamesl22", "x"));


    std::this_thread::sleep_for(std::chrono::milliseconds(20000));
    return 0;
}
