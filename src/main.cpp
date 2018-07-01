#include "Lyra2RE/Lyra2RE.h"

#include "util.h"
#include "stratumclient.h"
#include "cpumanager.h"
#include "workproxy.h"

int main() {
    std::unique_ptr<NextMiner::Log> log(new NextMiner::Log);
    std::unique_ptr<NextMiner::GetWork> workSource1(
        new NextMiner::StratumClient("18.250.0.71", 9271,
                                     "VnUxCjoLSv1U5pxegJBGxX8anDnop77DWz", "x",
                                     log.get()));

    std::unique_ptr<NextMiner::GetWork> workSource2(
        new NextMiner::StratumClient("198.204.233.158", 9171,
                                     "VnUxCjoLSv1U5pxegJBGxX8anDnop77DWz", "x",
                                     log.get()));

    // Wait until the workSource has work for us
    while(!workSource2->getWork() || !workSource1->getWork()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::unique_ptr<NextMiner::WorkProxy> workProxy(
        new NextMiner::WorkProxy(workSource1.get(), 10));

    workProxy->addWorkSource(workSource2.get(), 10);

    std::unique_ptr<NextMiner::CPUManager> CPU(
        new NextMiner::CPUManager(workProxy.get(), log.get()));

    std::this_thread::sleep_for(std::chrono::seconds(120));

    return 0;
}
