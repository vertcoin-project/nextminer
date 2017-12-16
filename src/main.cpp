#ifdef __linux
#include <pthread.h>
#endif // __linux

#include "Lyra2RE/Lyra2RE.h"

#include "util.h"
#include "stratumclient.h"
#include "cpumanager.h"

int main() {
    std::unique_ptr<NextMiner::Log> log(new NextMiner::Log);
    std::unique_ptr<NextMiner::GetWork> workSource(
        new NextMiner::StratumClient("18.250.5.90", 9171,
                                     "VnUxCjoLSv1U5pxegJBGxX8anDnop77DWz", "x",
                                     log.get()));

    // Wait until the workSource has work for us
    while(!workSource->getWork()) {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }

    std::unique_ptr<NextMiner::CPUManager> CPU(new NextMiner::CPUManager(workSource.get(), log.get()));

    std::this_thread::sleep_for(std::chrono::seconds(100 * 20));

    return 0;
}
