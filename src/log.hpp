#pragma once

struct LogString {
    std::string_view     fmt;
    std::source_location loc;

    template<typename T>
    LogString(const T& fmt) :
        fmt(fmt),
        loc(std::source_location::current())
    {}
};

template<typename... Args>
void LogInfo(LogString msg, Args&&... args) {
    printf("[INFO] [%s:%d]: %s\n", msg.loc.file_name(), (int)msg.loc.line(),
        std::vformat(msg.fmt, std::make_format_args(args...)).c_str());
}

template<typename... Args>
void LogWarn(LogString msg, Args&&... args) {
    printf("[WARN] [%s:%d]: %s\n", msg.loc.file_name(), (int)msg.loc.line(),
        std::vformat(msg.fmt, std::make_format_args(args...)).c_str());
}

template<typename... Args>
void LogError(LogString msg, Args&&... args) {
    fprintf(stderr, "[ERROR] [%s:%d]: %s\n", msg.loc.file_name(), (int)msg.loc.line(),
        std::vformat(msg.fmt, std::make_format_args(args...)).c_str());
}

