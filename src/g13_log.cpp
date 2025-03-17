//
// Created by Britt Yazel on 03-16-2025.
//

#include <log4cpp/BasicLayout.hh>
#include <log4cpp/Category.hh>

#include "g13_log.hpp"
#include "g13_main.hpp"

namespace G13 {
    void start_logging() {
        if (logging_initialized) {
            return; // Prevent re-initialization
        }

        appender1 = std::make_unique<log4cpp::OstreamAppender>("console", &std::cout);
        appender1->setLayout(new log4cpp::BasicLayout());

        log4cpp::Category& root = log4cpp::Category::getRoot();
        root.addAppender(appender1.get());
        root.setPriority(log4cpp::Priority::INFO);

        logging_initialized = true;
    }

    void stop_logging() {
        if (!logging_initialized) {
            return;
        }

        log4cpp::Category::shutdown();

        // Don't manually delete/reset appender1, to prevent double free
        appender1.release(); // Release ownership but don’t delete it

        logging_initialized = false;
    }

    void SetLogLevel(const log4cpp::Priority::PriorityLevel lvl) {
        if (!logging_initialized) {
            return;
        }

        G13_OUT("Setting log level to " << lvl);
        log4cpp::Category::getRoot().setPriority(lvl);
    }

    void SetLogLevel(const std::string& level) {
        if (!logging_initialized) {
            return;
        }

        log4cpp::Category& root = log4cpp::Category::getRoot();
        try {
            const auto numLevel = log4cpp::Priority::getPriorityValue(level);
            root.setPriority(numLevel);
        }
        catch (const std::invalid_argument&) {
            G13_ERR("Unknown log level: " << level);
        }
    }
}
