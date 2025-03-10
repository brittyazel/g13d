//
// Created by khampf on 08-05-2020.
//

#ifndef G13_G13_LOG_HPP
#define G13_G13_LOG_HPP

#include <log4cpp/Category.hh>

#define G13_LOG(message) log4cpp::Category::getRoot() << message
#define G13_ERR(message) log4cpp::Category::getRoot() << log4cpp::Priority::ERROR << message
#define G13_DBG(message) log4cpp::Category::getRoot() << log4cpp::Priority::DEBUG << message
#define G13_OUT(message) log4cpp::Category::getRoot() << log4cpp::Priority::INFO << message

#endif //G13_G13_LOG_HPP
