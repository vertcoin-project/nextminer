#ifndef STRATUMRPC_H_INCLUDED
#define STRATUMRPC_H_INCLUDED

#include <jsonrpccpp/client.h>

namespace NextMiner {
    class StratumRPCClient : public jsonrpc::Client {
        public:
            StratumRPCClient(jsonrpc::IClientConnector &conn,
                           jsonrpc::clientVersion_t type
                           = jsonrpc::JSONRPC_CLIENT_V2) :
                           jsonrpc::Client(conn, type) {}

            Json::Value authorize(const std::string& username,
                                  const std::string& password)
                                  throw (jsonrpc::JsonRpcException) {
                Json::Value params;
                params.append(username);
                params.append(password);

                const Json::Value result = this->CallMethod("authorize",
                                                            params);

                if(result.)
            }

            /*Json::Value getrawtransaction(const std::string& id, const bool verbose)
            throw (jsonrpc::JsonRpcException) {
                Json::Value p;
                p.append(id);
                p.append(verbose);
                const Json::Value result = this->CallMethod("getrawtransaction", p);
                if((result.isObject() && verbose) || (result.isString() && !verbose)) {
                    return result;
                } else {
                    throw jsonrpc::JsonRpcException(jsonrpc::Errors::ERROR_CLIENT_INVALID_RESPONSE,
                                                    result.toStyledString());
                }
            }*/
    };
}

#endif // STRATUMRPC_H_INCLUDED
