#pragma once
#ifdef _DEBUG

    #include <ctime>

    enum LogLevel { DEBUG, INFO, WARNING, ERROR, CRITICAL };

    static const char* levelToString(LogLevel level)
    {
        switch (level) {
        case DEBUG:
            return "DEBUG";
        case INFO:
            return "INFO";
        case WARNING:
            return "WARNING";
        case ERROR:
            return "ERROR";
        case CRITICAL:
            return "CRITICAL";
        default:
            return "UNKNOWN";
        }
    }

    static void log_message(LogLevel level, const char* message, auto&&... args)
    {
        time_t now = time(0);
        tm timeinfo = {0};
        localtime_s(&timeinfo, &now);
        char timestamp[20];
        strftime(timestamp, sizeof(timestamp), "%Y-%m-%d %H:%M:%S", &timeinfo);

        printf("[%s] %s: ",timestamp ,levelToString(level));
        printf(message, std::forward<decltype(args)>(args)...);
        printf("\n");
    }

    #define LOG(level, msg, ...) log_message(level, msg, __VA_ARGS__);

#else
    #define LOG(level, msg, ...)
#endif // _DEBUG

