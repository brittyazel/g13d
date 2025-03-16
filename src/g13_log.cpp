
#include <log4cpp/OstreamAppender.hh>
#include <log4cpp/BasicLayout.hh>
#include <log4cpp/Category.hh>
#include <memory>

#include "g13_log.hpp"
#include "g13_main.hpp"

namespace G13 {
    void start_logging() {
        auto appender1 = std::make_unique<log4cpp::OstreamAppender>("console", &std::cout);
        appender1->setLayout(new log4cpp::BasicLayout());
        log4cpp::Category& root = log4cpp::Category::getRoot();
        root.addAppender(appender1.get());

        // Keep the unique_ptr alive to avoid de-allocation
        static auto appender1_keeper = std::move(appender1);

        // TODO: this is for later when --log_file is implemented
        //    auto appender2 = std::make_unique<log4cpp::FileAppender>("default", "g13d-output.log");
        //    appender2->setLayout(new log4cpp::BasicLayout());
        //    log4cpp::Category &sub1 = log4cpp::Category::getInstance(std::string("sub1"));
        //    sub1.addAppender(appender2.get());
    }

    void SetLogLevel(const log4cpp::Priority::PriorityLevel lvl) {
        G13_OUT("set log level to " << lvl);
    }

    void SetLogLevel(const std::string& level) {
        log4cpp::Category& root = log4cpp::Category::getRoot();
        try {
            const auto numLevel = log4cpp::Priority::getPriorityValue(level);
            root.setPriority(numLevel);
        }
        catch ([[maybe_unused]] std::invalid_argument& e) {
            G13_ERR("unknown log level " << level);
        }
    }
} // namespace G13
