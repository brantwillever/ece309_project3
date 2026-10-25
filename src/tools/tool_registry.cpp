// src/tools/tool_registry.cpp — YOU IMPLEMENT (spec §4.3).
//
// A hand-written hash table. No std::unordered_map / std::map / std::set.

#include "tools/tool_registry.h"

#include <string>
#include <utility>

ToolRegistry::ToolRegistry() : buckets_(16) {}
ToolRegistry::~ToolRegistry() = default;

std::size_t ToolRegistry::hash(std::string_view key) noexcept {
    std::size_t value = 14695981039346656037ull;
    for (unsigned char ch : key) {
        value ^= ch;
        value *= 1099511628211ull;
    }
    return value;
}

bool ToolRegistry::add(std::unique_ptr<Tool> tool) {
    if (!tool) {
        return false;
    }

    const std::string name = tool->name();
    const std::size_t bucket_index = hash(name) % buckets_.size();
    for (const auto& existing : buckets_[bucket_index]) {
        if (existing->name() == name) {
            return false;
        }
    }

    if (size_ + 1 > buckets_.size()) {
        std::vector<std::vector<std::unique_ptr<Tool>>> new_buckets(buckets_.size() * 2);
        for (auto& bucket : buckets_) {
            for (auto& existing : bucket) {
                const std::size_t new_index = hash(existing->name()) % new_buckets.size();
                new_buckets[new_index].push_back(std::move(existing));
            }
        }
        buckets_.swap(new_buckets);
    }

    const std::size_t new_index = hash(name) % buckets_.size();
    buckets_[new_index].push_back(std::move(tool));
    ++size_;
    return true;
}

Tool* ToolRegistry::find(std::string_view name) const {
    const auto& bucket = buckets_[hash(name) % buckets_.size()];
    for (const auto& tool : bucket) {
        if (tool->name() == name) {
            return tool.get();
        }
    }
    return nullptr;
}

std::size_t ToolRegistry::size() const noexcept {
    return size_;
}

void ToolRegistry::for_each(const std::function<void(const Tool&)>& fn) const {
    for (const auto& bucket : buckets_) {
        for (const auto& tool : bucket) {
            fn(*tool);
        }
    }
}

std::size_t ToolRegistry::bucket_count() const noexcept {
    return buckets_.size();
}

double ToolRegistry::load_factor() const noexcept {
    return static_cast<double>(size_) / static_cast<double>(buckets_.size());
}

std::size_t ToolRegistry::max_probe_length() const {
    std::size_t longest = 0;
    for (const auto& bucket : buckets_) {
        if (bucket.size() > longest) {
            longest = bucket.size();
        }
    }
    return longest;
}
