#ifndef CORE_CONVERSATION_H
#define CORE_CONVERSATION_H

#include "core/message.h"

#include <cstddef>
#include <stdexcept>

class Conversation {
public:
    Conversation() = default;

    ~Conversation();

    Conversation(const Conversation& other);
    Conversation& operator=(const Conversation& other);

    Conversation(Conversation&& other) noexcept;
    Conversation& operator=(Conversation&& other) noexcept;

    void append(Message m);

    std::size_t size() const noexcept { return count; }

    const Message& at(std::size_t i) const {
        if (i >= count) {
            throw std::out_of_range("Conversation::at: index out of range");
        }
        return data_ptr[i];
    }

    const Message* begin() const noexcept {
        return data_ptr;
    }
    const Message* end() const noexcept {
        if (data_ptr == nullptr) {
            return nullptr;
        }
        return data_ptr + count;
    }

private:
    Message* data_ptr = nullptr;
    std::size_t count = 0;
    std::size_t capacity = 0;
};

#endif
