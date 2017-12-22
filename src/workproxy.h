#ifndef WORKPROXY_H_INCLUDED
#define WORKPROXY_H_INCLUDED

#include <map>

#include "getwork.h"

namespace NextMiner {
    class WorkProxy : public GetWork {
        public:
            WorkProxy(GetWork* workSource,
                      const unsigned int weight);
            virtual ~WorkProxy();

            virtual void registerWorker(std::function<void(const bool)> cb);

            virtual std::unique_ptr<Work> getWork();

            virtual std::tuple<bool, std::string> submitWork(const Work& work);

            virtual void suggestDifficulty(const double difficulty);

            void addWorkSource(GetWork* workSource,
                               const unsigned int weight);

            class ProxyJob : public Work {
                friend class WorkProxy;

                public:
                    ProxyJob(GetWork::Work* work, GetWork* workSource);
                    ProxyJob();
                    ~ProxyJob();

                    virtual void operator=(const Work& other);

                    virtual std::vector<uint8_t> getBytes() const;

                    virtual void setNonce(const uint32_t nonce);

                    virtual uint32_t getTarget() const;

                    virtual void newExtranonce2();

                    virtual std::unique_ptr<Work> clone() const;

                protected:
                    std::unique_ptr<GetWork::Work> work;
                    GetWork* workSource;
            };

        private:
            std::map<GetWork*, unsigned int> workSources;
            std::vector<std::function<void(const bool)>> callbacks;

            unsigned int totalWeight;

            GetWork* getCurrentWorkSource();

            unsigned int counter;

            void callback(const bool discardOld);
    };
}

#endif // WORKPROXY_H_INCLUDED
