#ifndef STRATUMCLIENT_H_INCLUDED
#define STRATUMCLIENT_H_INCLUDED

#include <string>
#include <thread>
#include <mutex>

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
                    StratumJob(const Json::Value& notifyPayload,
                               const uint32_t target);
                    StratumJob();
                    ~StratumJob();

                    void operator=(const StratumJob& other);

                    virtual std::array<uint8_t, 80> getBytes();

                    virtual void setNonce(const uint32_t nonce);

                    virtual uint32_t getTarget();

                protected:
                    std::string jobId;
                    std::string prevHash;
                    std::string coinb1;
                    std::string coinb2;
                    std::vector<std::string> merkleHashes;
                    uint32_t version;
                    uint32_t nbits;
                    uint32_t ntime;

                    uint32_t nonce;
                    uint32_t target;
            };

            virtual void registerWorker(std::function<void(const Work&)> cb);

            virtual std::unique_ptr<Work> getWork();

            virtual std::tuple<bool, std::string> submitWork(const Work& work);

            virtual void suggestTarget(const uint32_t target);

        private:
            sf::TcpSocket socket;
            std::mutex socketLock;

            bool authorize(const std::string& username,
                           const std::string& password);

            Json::Value request(const std::string& method,
                                const Json::Value& params);

            uint64_t reqId;
            std::map<uint64_t, Json::Value> responses;
            std::mutex responsesLock;

            std::unique_ptr<std::thread> responseThread;

            void responseFunction();

            void sendResponse(const Json::Value& result,
                              const uint64_t id,
                              const Json::Value& error = Json::nullValue);

            void sendJson(const Json::Value& payload);

            bool running;

            std::unique_ptr<StratumJob> currentJob;
    };
}

#endif // STRATUMCLIENT_H_INCLUDED
