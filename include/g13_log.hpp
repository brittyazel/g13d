//
// Created by khampf on 08-05-2020.
//

#ifndef G13_LOG_HPP
#define G13_LOG_HPP

#include <log4cpp/Category.hh>
#include <log4cpp/OstreamAppender.hh>

#include <memory>

#define G13_LOG(message) do { log4cpp::Category::getRoot() << message; std::cout.flush(); } while(0)
#define G13_ERR(message) do { log4cpp::Category::getRoot() << log4cpp::Priority::ERROR << message; std::cout.flush(); } while(0)
#define G13_DBG(message) do { log4cpp::Category::getRoot() << log4cpp::Priority::DEBUG << message; std::cout.flush(); } while(0)
#define G13_OUT(message) do { log4cpp::Category::getRoot() << log4cpp::Priority::INFO << message; std::cout.flush(); } while(0)

namespace G13 {
    void start_logging();
    void stop_logging();
    void SetLogLevel(log4cpp::Priority::PriorityLevel lvl);
    void SetLogLevel(const std::string& level);

    inline std::unique_ptr<log4cpp::OstreamAppender> appender1;
    inline bool logging_initialized = false;
}

#endif
