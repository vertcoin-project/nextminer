#ifndef STRATUMCLIENT_H_INCLUDED
#define STRATUMCLIENT_H_INCLUDED

#include <string>
#include <thread>
#include <mutex>
#include <random>

#include <SFML/Network/TcpSocket.hpp>

#include <json/value.h>

#include "getwork.h"
#include "log.h"

namespace NextMiner {
    class StratumClient : public GetWork {
        public:
            StratumClient(const std::string& host,
                          const unsigned int port,
                          const std::string& username,
                          const std::string& password,
                          Log* log);

            virtual ~StratumClient();

            class StratumJob : public Work {
                friend class StratumClient;

                public:
                    StratumJob(const Json::Value& notifyPayload,
                               const uint32_t target,
                               const std::string& extranonce1,
                               const unsigned int extranonce2Size);
                    StratumJob();
                    ~StratumJob();

                    virtual void operator=(const Work& other);

                    virtual std::vector<uint8_t> getBytes() const;

                    virtual void setNonce(const uint32_t nonce);

                    virtual uint32_t getTarget() const;

                    virtual void newExtranonce2();

                    virtual std::unique_ptr<Work> clone() const;

                protected:
                    std::string jobId;
                    std::string prevHash;
                    std::string coinb1;
                    std::string coinb2;
                    std::vector<std::string> merkleHashes;
                    std::string version;
                    std::string nbits;
                    std::string ntime;

                    uint32_t nonce;
                    uint32_t target;

                    std::string extranonce1;
                    std::string extranonce2;

                    unsigned int extranonce2Size;

                    std::default_random_engine generator;

            };

            virtual void registerWorker(std::function<void(const bool)> cb);

            virtual std::unique_ptr<Work> getWork();

            virtual std::tuple<bool, std::string> submitWork(const Work& work);

            virtual void suggestDifficulty(const double difficulty);

        private:
            sf::TcpSocket socket;
            std::mutex socketLock;

            bool authorize(const std::string& username,
                           const std::string& password);

            void subscribe();

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
            std::mutex jobLock;

            Log* log;

            struct currentParams {
                std::string extranonce1;
                unsigned int extranonce2Size;
                uint32_t target;
            } currentParams;

            std::vector<std::function<void(const bool)>> callbacks;
            std::mutex callbacksLock;

            std::string username;
    };
}

#endif // STRATUMCLIENT_H_INCLUDED
