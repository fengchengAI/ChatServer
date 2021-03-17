//
// Created by feng on 2021/3/15.
//

#ifndef UNIX_NETWORK_LOG_H
#define UNIX_NETWORK_LOG_H

#include <string>
#include <functional>
class Log {
public:
    enum Level {
        ALL = 0,
        INFO = 1,
        WARN = 2,
        ERROR = 3,
        FATAL = 4,
        OFF = 5
    };
    //typedef std::function<void(const std::string &, Level)> LogCallback;
    static Level level;
    static void log(const std::string &message, Level level = ALL);
    static void log_with_date_time(const std::string &message, Level level = ALL);
    static void redirect(const std::string &filename);
    //static void set_callback(LogCallback cb);
    //static void reset();
    static void stop();

private:
    static std::ofstream output;
    //static LogCallback log_callback;
};


#endif //UNIX_NETWORK_LOG_H
