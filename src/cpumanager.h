#ifndef CPUMANAGER_H_INCLUDED
#define CPUMANAGER_H_INCLUDED

#include "workermanager.h"
#include "cpuworker.h"
#include "log.h"

namespace NextMiner {
    class CPUManager : public WorkerManager {
        public:
            CPUManager(NextMiner::GetWork* workSource,
                       NextMiner::Log* log);
            virtual ~CPUManager();

        private:
            NextMiner::GetWork* workSource;
            NextMiner::Log* log;
            std::vector<std::unique_ptr<NextMiner::CPUWorker>> workers;
            std::unique_ptr<std::thread> managerThread;

            bool running;
            bool stale;
    };
}

#endif // CPUMANAGER_H_INCLUDED
