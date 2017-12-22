#include <sstream>
#include <random>
#include <iomanip>
#include <iostream>

#include <SFML/Network/IpAddress.hpp>

#include <json/reader.h>
#include <json/writer.h>

#include "stratumclient.h"
#include "version.h"
#include "log.h"
#include "util.h"

NextMiner::StratumClient::StratumClient(const std::string& host,
                                        const unsigned int port,
                                        const std::string& username,
                                        const std::string& password,
                                        NextMiner::Log* log) {
    reqId = 0;
    this->log = log;
    this->username = username;

    if(socket.connect(host, port) != sf::Socket::Status::Done) {
        log->printf("Failed to connect to stratum server", Log::Severity::Error);
    }

    socket.setBlocking(false);

    running = true;

    responseThread.reset(new std::thread(&NextMiner::StratumClient::responseFunction, this));

    subscribe();

    if(!authorize(username, password)) {
        log->printf("Failed to authorize with stratum server", Log::Severity::Error);
    }

    Json::Value req;
    req["id"] = reqId++;
    req["jsonrpc"] = "2.0";
    req["params"] = Json::nullValue;
    req["method"] = "mining.extranonce.subscribe";

    sendJson(req);
}

NextMiner::StratumClient::~StratumClient() {
    running = false;
    responseThread->join();
}

void NextMiner::StratumClient::subscribe() {
    Json::Value params;
    params.append(version);
    const Json::Value res = request("mining.subscribe", params);
    currentParams.extranonce1 = res["result"][1].asString();
    currentParams.extranonce2Size = res["result"][2].asUInt();
}

NextMiner::StratumClient::StratumJob::StratumJob() {

}

NextMiner::StratumClient::StratumJob::~StratumJob() {

}

NextMiner::StratumClient::StratumJob::StratumJob(const Json::Value& notifyPayload,
                                                 const uint32_t target,
                                                 const std::string& extranonce1,
                                                 const unsigned int extranonce2Size) {
    jobId = notifyPayload[0].asString();
    prevHash = notifyPayload[1].asString();
    coinb1 = notifyPayload[2].asString();
    coinb2 = notifyPayload[3].asString();
    for(const auto& merkleHash : notifyPayload[4]) {
        merkleHashes.push_back(merkleHash.asString());
    }
    version = notifyPayload[5].asString();
    nbits = notifyPayload[6].asString();
    ntime = notifyPayload[7].asString();

    this->target = target;
    this->extranonce1 = extranonce1;

    std::seed_seq seed(jobId.begin(), jobId.end());
    generator.seed(seed);

    this->extranonce2Size = extranonce2Size;

    newExtranonce2();

    nonce = 0;
}

void NextMiner::StratumClient::StratumJob::operator=(const Work& other) {
    const StratumJob& stratumOther = dynamic_cast<const StratumJob&>(other);

    jobId = stratumOther.jobId;
    prevHash = stratumOther.prevHash;
    coinb1 = stratumOther.coinb1;
    coinb2 = stratumOther.coinb2;
    merkleHashes = stratumOther.merkleHashes;
    version = stratumOther.version;
    nbits = stratumOther.nbits;
    ntime = stratumOther.ntime;
    nonce = stratumOther.nonce;
    target = stratumOther.target;
    extranonce1 = stratumOther.extranonce1;
    extranonce2 = stratumOther.extranonce2;
    generator = stratumOther.generator;
}

void NextMiner::StratumClient::StratumJob::setNonce(const uint32_t nonce) {
    this->nonce = nonce;
}

uint32_t NextMiner::StratumClient::StratumJob::getTarget() const {
    return target;
}

void NextMiner::StratumClient::StratumJob::newExtranonce2() {
    std::uniform_int_distribution<uint64_t> distribution(0,
                                            std::pow(2, extranonce2Size * 8)
                                                                        - 1);

    std::stringstream ss;
    ss << std::setw(extranonce2Size * 2) << std::setfill('0')
       << std::hex << distribution(generator);

    extranonce2 = ss.str();
}

std::unique_ptr<NextMiner::GetWork::Work> NextMiner::StratumClient::StratumJob::clone() const {
    StratumJob* job = new StratumJob;
    *job = *this;

    return std::unique_ptr<NextMiner::GetWork::Work>(job);
}

std::vector<uint8_t> NextMiner::StratumClient::StratumJob::getBytes() const {
    const std::string coinbaseHex = coinb1 + extranonce1 + extranonce2 + coinb2;

    const auto coinbaseBytes = HexToBytes(coinbaseHex);
    const auto coinbaseBinHash = DoubleSHA256(coinbaseBytes);

    std::vector<uint8_t> merkleRoot = coinbaseBinHash;
    for(const auto& h : merkleHashes) {
        const auto hashBytes = HexToBytes(h);
        merkleRoot.insert(merkleRoot.end(), hashBytes.begin(), hashBytes.end());

        merkleRoot = DoubleSHA256(merkleRoot);
    }

    const std::string merkleRootHex = BytesToHex(ReverseWords(merkleRoot));

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << EndSwap(nonce);

    const std::string blockHeaderHex = this->version + prevHash +
                                       merkleRootHex + ntime +
                                       nbits + ss.str();

    return ReverseWords(HexToBytes(blockHeaderHex));
}

void NextMiner::StratumClient::registerWorker(std::function<void(const bool)> cb) {
    std::lock_guard<std::mutex> lock(callbacksLock);
    callbacks.push_back(cb);
}

std::unique_ptr<NextMiner::GetWork::Work> NextMiner::StratumClient::getWork() {
    if(currentJob) {
        currentJob->newExtranonce2();
        StratumJob* job = new StratumJob;

        jobLock.lock();
        *job = *currentJob;
        jobLock.unlock();

        return std::unique_ptr<GetWork::Work>(job);
    } else {
        return std::unique_ptr<GetWork::Work>();
    }
}

std::tuple<bool, std::string> NextMiner::StratumClient::submitWork(const NextMiner::GetWork::Work& work) {
    const StratumJob& job = dynamic_cast<const StratumJob&>(work);

    Json::Value params;
    params.append(username);
    params.append(job.jobId);
    params.append(job.extranonce2);
    params.append(job.ntime);

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << EndSwap(job.nonce);
    params.append(ss.str());

    const Json::Value res = request("mining.submit", params);
    if(res["result"].asBool()) {
        return std::make_tuple(true, "");
    } else {
        return std::make_tuple(false, res["error"].asString());
    }
}

void NextMiner::StratumClient::suggestDifficulty(const double difficulty) {
    Json::Value params;
    params.append(difficulty);

    Json::Value req;
    req["id"] = reqId++;
    req["method"] = "mining.suggest_difficulty";
    req["jsonrpc"] = "2.0";
    req["params"] = params;

    sendJson(req);
}

void NextMiner::StratumClient::responseFunction() {
    std::string leftOvers;
    while(running) {
        std::string buffer;
        buffer.resize(1024 * 20);
        size_t received;
        socketLock.lock();
        if(socket.receive(&buffer[0], buffer.capacity(), received) != sf::Socket::Status::Error) {
            socketLock.unlock();
            buffer.resize(received);
            if(leftOvers.size() > 0) {
                buffer = leftOvers + buffer;
                leftOvers = "";
            }

            std::stringstream ss(buffer);
            std::string line;
            while(std::getline(ss, line, '\n')) {
                Json::CharReaderBuilder rbuilder;
                rbuilder["collectComments"] = false;
                std::string errs;
                std::stringstream input(line);

                Json::Value res;
                try {
                    Json::parseFromStream(rbuilder, input, &res, &errs);
                } catch(const Json::Exception& e) {
                    res = Json::nullValue;
                }

                if(errs.size() > 0) {
                    leftOvers = line;
                }

                if(res.isObject()) {
                    // Is it a response or a request?
                    if(res.isMember("result")) {
                        responsesLock.lock();
                        responses.insert(std::make_pair(res["id"].asUInt64(), res));
                        responsesLock.unlock();
                    } else if(res.isMember("method")) {
                        const std::string method = res["method"].asString();
                        const uint64_t id = res["id"].asUInt64();

                        if(method == "client.get_version") {
                            sendResponse(version, id);
                        } else if(method == "client.reconnect") {
                            // TODO
                            log->printf("Stratum: client.reconnect unimplemented",
                                        Log::Severity::Warning);
                        } else if(method == "client.show_message") {
                            log->printf("Stratum: Message from server: " +
                                        res["params"][0].asString(),
                                        Log::Severity::Notice);
                        } else if(method == "mining.notify") {
                            if(currentParams.extranonce2Size <= 32) {
                                StratumJob* newWork = new StratumJob(
                                                      res["params"],
                                                      currentParams.target,
                                                      currentParams.extranonce1,
                                                      currentParams
                                                      .extranonce2Size);

                                const auto hexBytes = BytesToHex(newWork->getBytes());

                                jobLock.lock();
                                currentJob.reset(newWork);
                                jobLock.unlock();

                                callbacksLock.lock();
                                for(const auto& cb : callbacks) {
                                    cb(res["params"][8].asBool());
                                }
                                callbacksLock.unlock();

                                log->printf("Stratum: mining.notify " + hexBytes,
                                Log::Severity::Notice);
                            }
                        } else if(method == "mining.set_difficulty") {
                            std::thread([this, res]{
                                currentParams.target = DiffToCompact(
                                                       res["params"][0]
                                                       .asDouble() / 256);

                                log->printf("Stratum: mining.set_difficulty(" +
                                            res["params"][0].asString() +
                                            ") nBits: " +
                                            std::to_string(currentParams.target),
                                        Log::Severity::Notice);
                            }).detach();
                        } else if(method == "mining.set_extranonce") {
                            currentParams.extranonce1 = res["params"][0]
                                                        .asString();
                            currentParams.extranonce2Size = res["params"][1]
                                                            .asUInt();
                        }
                    }
                }
            }
        } else {
            socketLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(100));
        }
    }
}

void NextMiner::StratumClient::sendResponse(const Json::Value& result,
                                            const uint64_t id,
                                            const Json::Value& error) {
    std::thread([=]{
        Json::Value res;

        res["id"] = id;
        res["jsonrpc"] = "2.0";
        res["result"] = result;
        res["error"] = error;

        sendJson(res);
    }).detach();
}

void NextMiner::StratumClient::sendJson(const Json::Value& payload) {
    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "";

    const std::string rawReq = Json::writeString(builder, payload) + "\n";

    socketLock.lock();
    socket.setBlocking(true);
    socket.send(rawReq.c_str(), rawReq.size());
    socket.setBlocking(false);
    socketLock.unlock();
}

bool NextMiner::StratumClient::authorize(const std::string& username,
                                         const std::string& password) {
    Json::Value params;
    params.append(username);
    params.append(password);
    const Json::Value res = request("mining.authorize", params);

    return res["error"].empty();
}

Json::Value NextMiner::StratumClient::request(const std::string& method,
                                              const Json::Value& params) {
    Json::Value req;

    const auto ourId = reqId++;

    req["id"] = ourId;
    req["jsonrpc"] = "2.0";
    req["params"] = params;
    req["method"] = method;

    sendJson(req);

    unsigned int attempts = 0;

    responsesLock.lock();
    auto it = responses.find(ourId);
    while(it == responses.end()) {
        responsesLock.unlock();

        attempts++;
        if(attempts > 2000) {
            throw std::runtime_error("Lost connection with stratum server");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));

        responsesLock.lock();
        it = responses.find(ourId);
    }

    const Json::Value response = it->second;

    responses.erase(ourId);
    responsesLock.unlock();

    return response;
}

