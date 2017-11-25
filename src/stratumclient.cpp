#include <sstream>

#include <SFML/Network/IpAddress.hpp>

#include "stratumclient.h"

NextMiner::StratumClient::StratumClient(const std::string& host,
                                        const unsigned int port,
                                        const std::string& username,
                                        const std::string& password) {
    reqId = 0;

    if(socket.connect(host, port) != sf::Socket::Status::Done) {
        throw std::runtime_error("Failed to connect to stratum server");
    }

    running = true;

    responseThread.reset(new std::thread(&NextMiner::StratumClient::responseFunction, this));

    if(!authorize(username, password)) {
        throw std::runtime_error("Failed to authorize with stratum server");
    }
}

NextMiner::StratumClient::~StratumClient() {
    running = false;
    responseThread->join();
}

void NextMiner::StratumClient::registerWorker(std::function<void(const NextMiner::GetWork::Work&)> cb) {
    // TODO
}

std::unique_ptr<NextMiner::GetWork::Work> NextMiner::StratumClient::getWork() {
    // TODO
}

std::tuple<bool, std::string> NextMiner::StratumClient::submitWork(const NextMiner::GetWork::Work& work) {
    // TODO
}

void NextMiner::StratumClient::suggestTarget(const uint32_t target) {
    // TODO
}

void NextMiner::StratumClient::responseFunction() {
    while(running) {
        std::string buffer(1024, '\0');
        size_t received;
        if(socket.receive(&buffer[0], buffer.capacity(), received) == sf::Socket::Status::Done) {
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
                        responses.insert(std::make_pair(res["id"].asUInt64(), res));
                    } else if(res.isMember("method")) {
                        const std::string method = res["method"].asString();

                        if(method == "client.get_version") {
                            // TODO
                        } else if(method == "client.reconnect") {
                            // TODO
                        } else if(method == "client.show_message") {
                            // TODO
                        } else if(method == "mining.notify") {
                            // TODO
                        } else if(method == "mining.set_difficulty") {
                            // TODO
                        } else if(method == "mining.set_extranonce") {
                            // TODO
                        }
                    }
                }
            }
        } else {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    }
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

    Json::StreamWriterBuilder builder;
    builder["commentStyle"] = "None";
    builder["indentation"] = "";

    const std::string rawReq = Json::writeString(builder, req) + "\n";

    socket.send(rawReq.c_str(), rawReq.size());

    unsigned int attempts = 0;
    auto it = responses.find(ourId);
    while(it == responses.end()) {
        attempts++;
        if(attempts > 50) {
            throw std::runtime_error("Lost connection with stratum server");
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(5));
        it = responses.find(ourId);
    }

    const Json::Value response = it->second;

    responses.erase(ourId);

    return response;
}

