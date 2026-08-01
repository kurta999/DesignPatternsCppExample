#include "simulator/Logging.hpp"

#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace simulator
{
const char* toString(const LogLevel level) noexcept
{
    switch (level)
    {
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warning: return "WARN";
    case LogLevel::Error: return "ERROR";
    }
    return "UNKNOWN";
}

std::string formatLogRecord(const LogRecord& record)
{
    const auto time = std::chrono::system_clock::to_time_t(record.timestamp);
    const auto milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(
        record.timestamp.time_since_epoch()) % 1000;
    std::tm localTime{};
#ifdef _WIN32
    localtime_s(&localTime, &time);
#else
    localtime_r(&time, &localTime);
#endif
    std::ostringstream text;
    text << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S") << '.'
         << std::setfill('0') << std::setw(3) << milliseconds.count()
         << " [" << toString(record.level) << "] [" << record.source << "] "
         << record.message;
    return text.str();
}

void Logger::addSink(std::shared_ptr<ILogSink> sink)
{
    if (!sink) throw std::invalid_argument("log sink must not be null");
    const std::lock_guard<std::mutex> lock{mutex_};
    sinks_.push_back(std::move(sink));
}

void Logger::log(const LogLevel level, const std::string_view source,
                 const std::string_view message)
{
    const LogRecord record{std::chrono::system_clock::now(), level,
                           std::string{source}, std::string{message}};
    std::vector<std::shared_ptr<ILogSink>> sinks;
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        sinks = sinks_;
    }
    for (const auto& sink : sinks) sink->write(record);
}

class FileLogSink::Impl
{
public:
    explicit Impl(const std::filesystem::path& path)
    {
        if (path.has_parent_path()) std::filesystem::create_directories(path.parent_path());
        stream_.open(path, std::ios::out | std::ios::app);
        if (!stream_) throw std::runtime_error("cannot open simulation log file");
    }
    void write(const LogRecord& record)
    {
        const std::lock_guard<std::mutex> lock{mutex_};
        stream_ << formatLogRecord(record) << '\n';
        stream_.flush();
    }
private:
    std::mutex mutex_;
    std::ofstream stream_;
};

FileLogSink::FileLogSink(const std::filesystem::path& path) : impl_{std::make_unique<Impl>(path)} {}
FileLogSink::~FileLogSink() = default;
void FileLogSink::write(const LogRecord& record) { impl_->write(record); }

void MemoryLogSink::write(const LogRecord& record)
{
    const std::lock_guard<std::mutex> lock{mutex_};
    records_.push_back(record);
}
std::vector<LogRecord> MemoryLogSink::records() const
{
    const std::lock_guard<std::mutex> lock{mutex_};
    return records_;
}

CallbackLogSink::CallbackLogSink(Callback callback) : callback_{std::move(callback)}
{
    if (!callback_) throw std::invalid_argument("log callback must not be empty");
}
void CallbackLogSink::write(const LogRecord& record) { callback_(record); }
} // namespace simulator

