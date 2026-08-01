#pragma once

/**
 * @file Logging.hpp
 * @brief Thread-safe structured logging with replaceable output sinks.
 */

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
/** @addtogroup connected_simulator
 *  @{
 */
/** @brief Severity attached to a LogRecord. */
enum class LogLevel
{
    Debug,   ///< Periodic diagnostic or telemetry detail.
    Info,    ///< Normal lifecycle or configuration event.
    Warning, ///< Recoverable abnormal condition.
    Error    ///< Protection trip or failed operation.
};

/** @brief Structured message passed unchanged to every registered sink. */
struct LogRecord
{
    std::chrono::system_clock::time_point timestamp; ///< Host wall-clock timestamp.
    LogLevel level; ///< Record severity.
    std::string source; ///< Subsystem producing the event.
    std::string message; ///< Human-readable event text.
};

/** @brief Narrow destination interface used by Logger. */
class ILogSink
{
public:
    virtual ~ILogSink() = default;
    /**
     * @brief Consume one structured record.
     * @param record Record valid for the duration of this call.
     */
    virtual void write(const LogRecord& record) = 0;
};

/** @brief Logging abstraction injected into SimulationEngine. */
class ILogger
{
public:
    virtual ~ILogger() = default;
    /** @brief Create and publish one record to the configured destination(s). */
    virtual void log(LogLevel level, std::string_view source,
                     std::string_view message) = 0;
};

/**
 * @brief Thread-safe logger that fans records out to all registered sinks.
 *
 * The sink collection is copied while locked and callbacks run after the lock
 * is released, allowing a sink to log or perform UI handoff without deadlock.
 */
class Logger final : public ILogger
{
public:
    /**
     * @brief Register a shared sink.
     * @throws std::invalid_argument when @p sink is null.
     */
    void addSink(std::shared_ptr<ILogSink> sink);
    /** @copydoc ILogger::log */
    void log(LogLevel level, std::string_view source,
             std::string_view message) override;
private:
    std::mutex mutex_;
    std::vector<std::shared_ptr<ILogSink>> sinks_;
};

/** @brief Append-only, immediately flushed persistent log destination. */
class FileLogSink final : public ILogSink
{
public:
    /**
     * @brief Open a log and create missing parent directories.
     * @param path Destination log path.
     * @throws std::runtime_error when the file cannot be opened.
     */
    explicit FileLogSink(const std::filesystem::path& path);
    ~FileLogSink() override;
    /** @copydoc ILogSink::write */
    void write(const LogRecord& record) override;
private:
    class Impl;
    std::unique_ptr<Impl> impl_;
};

/** @brief Thread-safe in-memory sink used by automated tests and diagnostics. */
class MemoryLogSink final : public ILogSink
{
public:
    /** @copydoc ILogSink::write */
    void write(const LogRecord& record) override;
    /** @return A consistent copy of all records captured so far. */
    [[nodiscard]] std::vector<LogRecord> records() const;
private:
    mutable std::mutex mutex_;
    std::vector<LogRecord> records_;
};

/** @brief Sink Adapter that forwards each record to a supplied callable. */
class CallbackLogSink final : public ILogSink
{
public:
    /** @brief Callback signature; the record is valid only during the call. */
    using Callback = std::function<void(const LogRecord&)>;
    /** @param callback Function invoked synchronously by write(). */
    explicit CallbackLogSink(Callback callback);
    /** @copydoc ILogSink::write */
    void write(const LogRecord& record) override;
private:
    Callback callback_;
};

/** @return Timestamped single-line representation suitable for files and the GUI. */
[[nodiscard]] std::string formatLogRecord(const LogRecord& record);
/** @return Stable uppercase name for @p level. */
[[nodiscard]] const char* toString(LogLevel level) noexcept;
/** @} */
} // namespace simulator
