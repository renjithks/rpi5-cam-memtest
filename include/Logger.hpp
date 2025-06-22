#pragma once
#include <iostream>
#include <string>

enum class LogLevel { INFO, WARN, ERROR };

inline void log(LogLevel level, const std::string& msg) {
    switch (level) {
        case LogLevel::INFO:
            std::cout << "[INFO] " << msg << std::endl;
            break;
        case LogLevel::WARN:
            std::cout << "[WARN] " << msg << std::endl;
            break;
        case LogLevel::ERROR:
            std::cerr << "[ERROR] " << msg << std::endl;
            break;
    }
}