#include "stratumclient.h"

NextMiner::StratumClient::StratumClient(const std::string& host,
                                        const unsigned int port,
                                        const std::string& username,
                                        const std::string& password) {
    socket.reset(new jsonrpc::TcpSocketClient(host, port));
    client.reset(new StratumRPCClient(*socket));


}

