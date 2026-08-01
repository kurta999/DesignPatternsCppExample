#include "catalog/PatternDemo.hpp"

#include <cstddef>
#include <memory>
#include <sstream>
#include <utility>
#include <vector>

namespace catalog::behavioral
{
namespace
{
class ServiceValve
{
public:
    void open() noexcept { open_ = true; ++count_; }
    void close() noexcept { open_ = false; ++count_; }
    bool isOpen() const noexcept { return open_; }
    std::size_t count() const noexcept { return count_; }
private:
    bool open_{false};
    std::size_t count_{0U};
};
class ICommand
{
public:
    virtual ~ICommand() = default;
    virtual void execute() noexcept = 0;
    virtual void undo() noexcept = 0;
};
class OpenValveCommand final : public ICommand
{
public:
    explicit OpenValveCommand(ServiceValve& valve) noexcept : valve_{valve} {}
    void execute() noexcept override { wasOpen_ = valve_.isOpen(); valve_.open(); }
    void undo() noexcept override { if (!wasOpen_) valve_.close(); }
private:
    ServiceValve& valve_;
    bool wasOpen_{false};
};
class MaintenanceQueue
{
public:
    void execute(std::unique_ptr<ICommand> command)
    {
        command->execute();
        history_.push_back(std::move(command));
    }
    void undoLast() noexcept
    {
        if (!history_.empty()) { history_.back()->undo(); history_.pop_back(); }
    }
private:
    std::vector<std::unique_ptr<ICommand>> history_{};
};
}
DemoResult runCommand()
{
    ServiceValve drain;
    MaintenanceQueue tool;
    tool.execute(std::make_unique<OpenValveCommand>(drain));
    require(drain.isOpen(), "open command failed");
    tool.undoLast();
    require(!drain.isOpen(), "command rollback failed");
    std::ostringstream text;
    text << "Opened and rolled back a coolant drain valve (" << drain.count() << " actuations).";
    return {"Behavioral", "Command", text.str()};
}
}

