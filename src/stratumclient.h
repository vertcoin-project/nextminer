#ifndef STRATUMCLIENT_H_INCLUDED
#define STRATUMCLIENT_H_INCLUDED

#include <string>
#include <thread>

#include <SFML/Network/TcpSocket.hpp>

#include "getwork.h"

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
            sf::TcpSocket socket;

            bool authorize(const std::string& username,
                           const std::string& password);

            Json::Value request(const std::string& method,
                                const Json::Value& params);

            uint64_t reqId;
            std::map<uint64_t, Json::Value> responses;

            std::unique_ptr<std::thread> responseThread;

            void responseFunction();

            bool running;
    };
}

#endif // STRATUMCLIENT_H_INCLUDED
