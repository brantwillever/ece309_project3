#include "core/conversation.h"

#include <utility>

Conversation::~Conversation() {
    delete[] data_ptr;
    data_ptr = nullptr;
    count = 0;
    capacity = 0;
}

Conversation::Conversation(const Conversation& other)
    : data_ptr(nullptr), count(other.count), capacity(other.capacity) {
    if (capacity == 0) {
        return;
    }

    data_ptr = new Message[capacity];
    for (std::size_t i = 0; i < count; ++i) {
        data_ptr[i] = other.data_ptr[i];
    }
}

Conversation& Conversation::operator=(const Conversation& other) {
    if (this == &other) {
        return *this;
    }

    Message* new_data = nullptr;
    std::size_t new_capacity = other.capacity;
    std::size_t new_count = other.count;

    if (new_capacity > 0) {
        new_data = new Message[new_capacity];
        for (std::size_t i = 0; i < new_count; ++i) {
            new_data[i] = other.data_ptr[i];
        }
    }

    delete[] data_ptr;
    data_ptr = new_data;
    count = new_count;
    capacity = new_capacity;

    return *this;
}

Conversation::Conversation(Conversation&& other) noexcept
    : data_ptr(other.data_ptr), count(other.count), capacity(other.capacity) {
    other.data_ptr = nullptr;
    other.count = 0;
    other.capacity = 0;
}

Conversation& Conversation::operator=(Conversation&& other) noexcept {
    if (this == &other) {
        return *this;
    }

    delete[] data_ptr;
    data_ptr = other.data_ptr;
    count = other.count;
    capacity = other.capacity;

    other.data_ptr = nullptr;
    other.count = 0;
    other.capacity = 0;

    return *this;
}

void Conversation::append(Message m) {
    if (count == capacity) {
        std::size_t new_capacity = capacity;
        if (new_capacity == 0) {
            new_capacity = 1;
        } else {
            new_capacity = new_capacity * 2;
        }

        Message* new_data = new Message[new_capacity];
        for (std::size_t i = 0; i < count; ++i) {
            new_data[i] = data_ptr[i];
        }

        delete[] data_ptr;
        data_ptr = new_data;
        capacity = new_capacity;
    }

    data_ptr[count] = m;
    ++count;
}
