#include <sstream>
#include <random>
#include <iomanip>

#include <SFML/Network/IpAddress.hpp>

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

    if(socket.connect(host, port) != sf::Socket::Status::Done) {
        log->printf("Failed to connect to stratum server", Log::Severity::Error);
    }

    socket.setBlocking(false);

    running = true;

    responseThread.reset(new std::thread(&NextMiner::StratumClient::responseFunction, this));

    if(!authorize(username, password)) {
        log->printf("Failed to authorize with stratum server", Log::Severity::Error);
    }
}

NextMiner::StratumClient::~StratumClient() {
    running = false;
    responseThread->join();
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
    std::default_random_engine generator(seed);
    std::uniform_int_distribution<uint64_t> distribution(0, std::pow(2, extranonce2Size * 8));

    std::stringstream ss;
    ss << std::setw(2 * extranonce2Size) << std::setfill('0')
       << std::hex << distribution(generator);

    extranonce2 = ss.str();
    nonce = 0;
}

void NextMiner::StratumClient::StratumJob::operator=(const StratumJob& other) {
    jobId = other.jobId;
    prevHash = other.prevHash;
    coinb1 = other.coinb1;
    coinb2 = other.coinb2;
    merkleHashes = other.merkleHashes;
    version = other.version;
    nbits = other.nbits;
    ntime = other.ntime;
    nonce = other.nonce;
    target = other.target;
    extranonce1 = other.extranonce1;
    extranonce2 = other.extranonce2;
}

void NextMiner::StratumClient::StratumJob::setNonce(const uint32_t nonce) {
    this->nonce = nonce;
}

uint32_t NextMiner::StratumClient::StratumJob::getTarget() {
    return target;
}

std::vector<uint8_t> NextMiner::StratumClient::StratumJob::getBytes() {
    const std::string coinbaseHex = coinb1 + extranonce1 + extranonce2 + coinb2;

    const auto coinbaseBytes = HexToBytes(coinbaseHex);
    const auto coinbaseBinHash = DoubleSHA256(coinbaseBytes);

    std::vector<uint8_t> merkleRoot = coinbaseBinHash;
    for(const auto& h : merkleHashes) {
        const auto hashBytes = HexToBytes(h);
        merkleRoot.insert(merkleRoot.end(), hashBytes.begin(), hashBytes.end());

        merkleRoot = DoubleSHA256(merkleRoot);
    }

    const std::string merkleRootHex = BytesToHex(merkleRoot);

    std::stringstream ss;
    ss << std::hex << std::setfill('0') << std::setw(8) << nonce;

    const std::string blockHeaderHex = this->version + prevHash +
                                       merkleRootHex + ntime +
                                       nbits + ss.str();

    return HexToBytes(blockHeaderHex);
}

void NextMiner::StratumClient::registerWorker(std::function<void(const NextMiner::GetWork::Work&)> cb) {
    // TODO
}

std::unique_ptr<NextMiner::GetWork::Work> NextMiner::StratumClient::getWork() {
    StratumJob* job = new StratumJob;
    *job = *currentJob;

    return std::unique_ptr<GetWork::Work>(job);
}

std::tuple<bool, std::string> NextMiner::StratumClient::submitWork(const NextMiner::GetWork::Work& work) {
    // TODO
}

void NextMiner::StratumClient::suggestTarget(const uint32_t target) {
    std::stringstream ss;

    ss << std::hex << target;

    Json::Value req;
    req["id"] = reqId++;
    req["method"] = "mining.suggest_target";
    req["jsonrpc"] = "2.0";
    req["params"] = ss.str();

    sendJson(req);
}

void NextMiner::StratumClient::responseFunction() {
    while(running) {
        std::string buffer(102400, '\0');
        size_t received;
        socketLock.lock();
        if(socket.receive(&buffer[0], buffer.capacity(), received) == sf::Socket::Status::Done) {
            socketLock.unlock();
            buffer.resize(received);

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
                        } else if(method == "client.show_message") {
                            log->printf("Message from server: " +
                                        res["params"].asString(),
                                        Log::Severity::Notice);
                        } else if(method == "mining.notify") {
                            // TODO
                            log->printf("Got notify",
                                        Log::Severity::Notice);
                        } else if(method == "mining.set_difficulty") {
                            // TODO
                            log->printf("Got set_difficulty",
                                        Log::Severity::Notice);
                        } else if(method == "mining.set_extranonce") {
                            // TODO
                        }
                    }
                }
            }
        } else {
            socketLock.unlock();
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
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
        if(attempts > 50) {
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

