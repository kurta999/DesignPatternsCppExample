#pragma once

#include <chrono>
#include <filesystem>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <string_view>
#include <vector>

namespace simulator
{
enum class LogLevel { Debug, Info, Warning, Error };

struct LogRecord
{
    std::chrono::system_clock::time_point timestamp;
    LogLevel level;
    std::string source;
    std::string message;
};

class ILogSink
{
public:
    virtual ~ILogSink() = default;
    virtual void write(const LogRecord& record) = 0;
};

class ILogger
{
public:
    virtual ~ILogger() = default;
    virtual void log(LogLevel level, std::string_view source,
                     std::string_view message) = 0;
};

class Logger final : public ILogger
{
public:
    void addSink(std::shared_ptr<ILogSink> sink);
    void log(LogLevel level, std::string_view source,
             std::string_view message) override;
private:
    std::mutex mutex_;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};

class FileLogSink final : public ILogSink
{
public:
    explicit FileLogSink(const std::filesystem::path& path);
    ~FileLogSink() override;
    void write(const LogRecord& record) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

class MemoryLogSink final : public ILogSink
{
public:
    void write(const LogRecord& record) override;
    [[nodiscard]] std::vector<LogRecord> records() const;
private:
    mutable std::mutex mutex_;
    std::vector<LogRecord> records_;
};

class CallbackLogSink final : public ILogSink
{
public:
    using Callback = std::function<void(const LogRecord&)>;
    explicit CallbackLogSink(Callback callback);
    void write(const LogRecord& record) override;
private:
    Callback callback_;
};

[[nodiscard]] std::string formatLogRecord(const LogRecord& record);
[[nodiscard]] const char* toString(LogLevel level) noexcept;
} // namespace simulator

