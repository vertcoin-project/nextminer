#ifdef __linux
#include <pthread.h>
#include <sys/time.h>
#include <sys/resource.h>
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

    #ifdef __linux
    if(setpriority(PRIO_PROCESS, 0, -15) == -1) {
        log->printf("Failed to set process priority",
                    NextMiner::Log::Severity::Warning);
    }
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);
    if(sched_setaffinity(0, sizeof(&cpuset), &cpuset) == -1) {
        log->printf("Failed to set process affinity",
                    NextMiner::Log::Severity::Warning);
    }
    #endif // __linux

    std::unique_ptr<NextMiner::CPUManager> CPU(new NextMiner::CPUManager(workSource.get(), log.get()));

    std::this_thread::sleep_for(std::chrono::seconds(120));

    return 0;
}
