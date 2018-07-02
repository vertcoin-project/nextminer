#ifndef AMDGPUMANAGER_H_INCLUDED
#define AMDGPUMANAGER_H_INCLUDED

#include "workermanager.h"
#include "log.h"

namespace NextMiner {
    class AMDGPUManager : public WorkerManager {
        public:
            AMDGPUManager(NextMiner::GetWork* workSource,
                       NextMiner::Log* log);
            virtual ~AMDGPUManager();

        private:
            NextMiner::GetWork* workSource;
            NextMiner::Log* log;

    };
}

#endif