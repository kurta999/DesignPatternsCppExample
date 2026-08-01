#include "catalog/PatternDemo.hpp"

#include <array>
#include <cstddef>
#include <cstdint>
#include <iterator>
#include <sstream>
#include <vector>

namespace catalog::behavioral
{
namespace
{
class FaultLog
{
public:
    static constexpr std::size_t capacity = 4U;
    void push(const std::uint16_t code) noexcept
    {
        codes_[next_] = code;
        next_ = (next_ + 1U) % capacity;
        if (count_ < capacity) ++count_;
    }
    // Hides ring-buffer wraparound and presents chronological traversal.
    class ConstIterator
    {
    public:
        using iterator_category = std::forward_iterator_tag;
        using value_type = std::uint16_t;
        using difference_type = std::ptrdiff_t;
        using pointer = const std::uint16_t*;
        using reference = const std::uint16_t&;
        ConstIterator(const FaultLog& log, const std::size_t offset) noexcept
            : log_{&log}, offset_{offset} {}
        reference operator*() const noexcept
        {
            const auto oldest = (log_->next_ + capacity - log_->count_) % capacity;
            return log_->codes_[(oldest + offset_) % capacity];
        }
        ConstIterator& operator++() noexcept { ++offset_; return *this; }
        friend bool operator==(const ConstIterator& a, const ConstIterator& b) noexcept
        { return a.log_ == b.log_ && a.offset_ == b.offset_; }
        friend bool operator!=(const ConstIterator& a, const ConstIterator& b) noexcept { return !(a == b); }
    private:
        const FaultLog* log_;
        std::size_t offset_;
    };
    ConstIterator begin() const noexcept { return {*this, 0U}; }
    ConstIterator end() const noexcept { return {*this, count_}; }
private:
    std::array<std::uint16_t, capacity> codes_{};
    std::size_t next_{0U};
    std::size_t count_{0U};
};
}
DemoResult runIterator()
{
    FaultLog log;
    for (const auto code : {10U, 20U, 30U, 40U, 50U}) log.push(static_cast<std::uint16_t>(code));
    const std::vector<std::uint16_t> values{log.begin(), log.end()};
    require(values == std::vector<std::uint16_t>({20U, 30U, 40U, 50U}), "ring order failed");
    std::ostringstream text;
    text << "Traversed wrapped fault entries oldest-to-newest: " << values.front()
         << " ... " << values.back() << '.';
    return {"Behavioral", "Iterator", text.str()};
}
}

