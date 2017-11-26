#ifndef LOG_H_INCLUDED
#define LOG_H_INCLUDED

#include <string>
#include <fstream>
#include <mutex>

namespace NextMiner {
    class Log {
        public:
            Log(const std::string& logfile = "nextminer.log",
                const bool printToConsole = true);
            ~Log();

            enum class Severity {
                Notice,
                Warning,
                Error
            };

            void printf(const std::string& msg, const Severity& severity);

        private:
            std::ofstream logfileStream;
            std::mutex logMutex;
            bool printToConsole;
    };
}

#endif // LOG_H_INCLUDED
