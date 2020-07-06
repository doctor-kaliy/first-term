#include "uint_vector.h"

#include <cstring>

static const size_t MAX_SIZE = ((SIZE_MAX << 1u) >> 1u);

shared_buffer* uint_vector::allocate_buffer(size_t size) {
    return static_cast<shared_buffer*>(operator new(sizeof(shared_buffer) + size * sizeof(uint32_t)));
}

size_t uint_vector::size() const {
    return (size_ & MAX_SIZE);
}

size_t uint_vector::capacity() {
    if (small()) {
        return uint_vector::STATIC_SIZE;
    } else {
        if (!dynamic_data) {
            return 0;
        }
        return dynamic_data->capacity;
    }
}

void uint_vector::delete_data() {
    if (dynamic_data) {
        dynamic_data->ref_count--;
        if (dynamic_data->ref_count == 0) {
            operator delete(dynamic_data);
        }
    }
}

void uint_vector::unshare() {
    if (dynamic_data && dynamic_data->ref_count != 1) {
        shared_buffer* new_data = new_buffer(size_);
        dynamic_data->ref_count--;
        dynamic_data = new_data;
    }
}

uint_vector::uint_vector() : size_(~MAX_SIZE) {}

uint_vector::uint_vector(size_t size, uint32_t const& value) : uint_vector() {
    resize(size, value);
}

shared_buffer* uint_vector::new_buffer(size_t size) const {
    shared_buffer* new_data = allocate_buffer(size);
    new_data->ref_count = 1;
    new_data->capacity = size;
    copy_all(new_data, dynamic_data, size_);
    return new_data;
}

uint_vector::uint_vector(uint_vector const& other) : uint_vector() {
    if (!other.empty()) {
        if (other.small()) {
            std::copy(other.static_data, other.static_data + uint_vector::STATIC_SIZE, static_data);
        } else {
            dynamic_data = other.dynamic_data;
            dynamic_data->ref_count++;
        }
        size_ = other.size_;
    }
}

uint_vector& uint_vector::operator=(uint_vector const& other) {
    if (this != &other) {
        uint_vector(other).swap(*this);
    }
    return *this;
}

void uint_vector::swap(uint_vector& other) {
    using std::swap;

    swap(static_data, other.static_data);
    swap(size_, other.size_);
}

uint_vector::~uint_vector() {
    if (!small()) {
        delete_data();
    }
}

uint32_t& uint_vector::operator[](size_t i) {
    assert(i < size());
    if (small()) {
        return static_data[i];
    }
    unshare();
    return dynamic_data->words[i];
}

uint32_t const& uint_vector::operator[](size_t i) const {
    assert(i < size());
    if (small()) {
        return static_data[i];
    }
    return dynamic_data->words[i];
}

bool uint_vector::small() const {
    return (size_ & (~MAX_SIZE)) != 0;
}

uint32_t& uint_vector::back() {
    assert(size() != 0);
    if (small()) {
        return static_data[size() - 1];
    }
    unshare();
    return dynamic_data->words[size() - 1];
}

uint32_t const& uint_vector::back() const {
    assert(size() != 0);
    if (small()) {
        return static_data[size() - 1];
    }
    return dynamic_data->words[size() - 1];
}

bool uint_vector::empty() const {
    return size() == 0;
}

uint_vector::const_iterator uint_vector::begin() const {
    if (small()) {
        return static_data;
    }
    return dynamic_data->words;
}

uint_vector::iterator uint_vector::begin() {
    if (small()) {
        return static_data;
    }
    unshare();
    return dynamic_data->words;
}

void uint_vector::reserve(size_t new_capacity) {
    if (new_capacity > capacity()) {
        shared_buffer* new_data;
        if (small()) {
            new_data = allocate_buffer(new_capacity);
            new_data->capacity = new_capacity;
            new_data->ref_count = 1;
            std::copy(static_data, static_data + uint_vector::STATIC_SIZE, new_data->words);
            size_ &= MAX_SIZE;
        } else {
            unshare();
            new_data = new_buffer(new_capacity);
            delete_data();
        }
        dynamic_data = new_data;
    }
}

void uint_vector::copy_all(shared_buffer* dst, shared_buffer const* src, size_t size) {
    std::copy(src->words, src->words + size, dst->words);
}

void uint_vector::clear() {
    if (small()) {
        size_ = (~MAX_SIZE);
    } else {
        unshare();
        size_ = 0;
    }
}

uint_vector::iterator uint_vector::end() {
    if (small()) {
        return static_data + size();
    }
    unshare();
    return dynamic_data->words + size();
}

uint_vector::const_iterator uint_vector::end() const {
    if (small()) {
        return static_data + size();
    }
    return dynamic_data->words + size();
}

void uint_vector::push_back(uint32_t const& el) {
    if (!small()) {
        unshare();
    }
    if (size() != capacity()) {
        if (small()) {
            static_data[size()] = el;
        } else {
            dynamic_data->words[size()] = el;
        }
        ++size_;
    } else {
        reserve(capacity() != 0 ? capacity() * 2 : 1);
        push_back(el);
    }
}

void uint_vector::pop_back() {
    assert(size() != 0);
    if (!small()) {
        unshare();
    }
    --size_;
}

uint_vector::iterator uint_vector::insert(uint_vector::const_iterator pos, uint32_t const& v) {
    assert(pos - begin() >= 0);
    if (!small()) {
        unshare();
    }
    size_t old_size = size();
    ptrdiff_t pos_ = pos - begin();
    push_back(v);
    for (size_t i = old_size; i > pos_; --i) {
        std::swap(*(begin() + i), *(begin() + i - 1));
    }
    return begin() + pos_;
}

void uint_vector::resize(size_t new_size) {
    while (size() > new_size) {
        pop_back();
    }
}

void uint_vector::resize(size_t new_size, uint32_t const& value) {
    while (size() < new_size) {
        push_back(value);
    }
}