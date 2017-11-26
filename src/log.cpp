#include <chrono>
#include <thread>
#include <sstream>
#include <iomanip>
#include <iostream>

#include "log.h"
#include "version.h"

NextMiner::Log::Log(const std::string& logfile, const bool printToConsole) {
    logfileStream.open(logfile, std::ios::out | std::ios::app);
    if(!logfileStream.is_open()) {
        throw std::runtime_error("Could not open given logfile");
    }

    this->printToConsole = printToConsole;

    logfileStream << "\n\n\n\n\n";

    printf("Started " + version, Severity::Notice);
}

NextMiner::Log::~Log() {

}

void NextMiner::Log::printf(const std::string& msg, const Severity& severity) {
    std::thread([=]{
        std::stringstream ss;

        const auto now = std::chrono::system_clock::now();
        const auto now_c = std::chrono::system_clock::to_time_t(now);
        ss << std::put_time(std::localtime(&now_c), "%D %T %Z");

        switch(severity) {
            case Severity::Error: {
                ss << "   ERROR   ";
                break;
            }

            case Severity::Notice: {
                ss << "   NOTICE  ";
                break;
            }

            case Severity::Warning: {
                ss << "   WARNING ";
                break;
            }

            default: {
                return;
            }
        }

        ss << msg;

        std::lock_guard<std::mutex> lock(logMutex);
        logfileStream << ss.str() << std::endl;
        logfileStream.flush();
        if(printToConsole) {
            std::cout << ss.str() << std::endl;
        }

        if(severity == Severity::Error) {
            throw std::runtime_error(msg);
        }
    }).detach();
}

