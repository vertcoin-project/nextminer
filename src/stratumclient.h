#ifndef STRATUMCLIENT_H_INCLUDED
#define STRATUMCLIENT_H_INCLUDED

#include <string>

#include <jsonrpccpp/client/connectors/tcpsocketclient.h>

#include "getwork.h"
#include "stratumrpc.h"

namespace NextMiner {
    class StratumClient : public GetWork {
        public:
            StratumClient(const std::string& host,
                          const unsigned int port,
                          const std::string& username,
                          const std::string& password);

            ~StratumClient();

            class StratumJob : public Work {
                public:
                    StratumJob();
                    ~StratumJob();

                    virtual std::array<uint8_t, 80> getBytes();

                    virtual void setNonce(const uint32_t nonce);

                    virtual uint32_t getTarget();
            };

            virtual void registerWorker(std::function<void(const Work&)> cb);

            virtual std::unique_ptr<Work> getWork();

            virtual std::tuple<bool, std::string> submitWork(const Work& work);

            virtual void suggestTarget(const uint32_t target);

        private:
            std::unique_ptr<StratumRPCClient> client;
            std::unique_ptr<jsonrpc::TcpSocketClient> socket;
    };
}

#endif // STRATUMCLIENT_H_INCLUDED
