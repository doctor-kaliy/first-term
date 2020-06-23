#ifndef VECTOR_H
#define VECTOR_H

#include <algorithm>
#include <cstddef>
#include <cassert>
#include <iostream>

template <typename T>
struct vector {
    typedef T* iterator;
    typedef T const* const_iterator;

    vector();                    // O(1) nothrow
    vector(vector const&);                  // O(N) strong
    vector& operator=(vector const&); // O(N) strong

    ~vector();                              // O(N) nothrow

    T& operator[](size_t);                // O(1) nothrow
    T const& operator[](size_t) const;    // O(1) nothrow

    T* data();                              // O(1) nothrow
    T const* data() const;                  // O(1) nothrow
    size_t size() const;                    // O(1) nothrow

    T& front();                             // O(1) nothrow
    T const& front() const;                 // O(1) nothrow

    T& back();                              // O(1) nothrow
    T const& back() const;                  // O(1) nothrow
    void push_back(T const&);               // O(1)* strong
    void pop_back();                        // O(1) nothrow

    bool empty() const;                     // O(1) nothrow

    size_t capacity() const;                // O(1) nothrow
    void reserve(size_t);                   // O(N) strong
    void shrink_to_fit();                   // O(N) strong

    void clear();                           // O(N) nothrow

    void swap(vector&);                     // O(1) nothrow

    iterator begin();                       // O(1) nothrow
    iterator end();                         // O(1) nothrow

    const_iterator begin() const;           // O(1) nothrow
    const_iterator end() const;             // O(1) nothrow

    iterator insert(const_iterator pos, T const&); // O(N) weak

    iterator erase(const_iterator pos);     // O(N) weak

    iterator erase(const_iterator first, const_iterator last); // O(N) weak

private:
    static void copy_all(T*, T const*, size_t);
    static void destroy_all(T*, size_t);

    T* new_buffer(size_t) const;
    void push_back_realloc(T const&);
    void init(T*, size_t, size_t);
    void delete_data();

    T* data_;
    size_t size_;
    size_t capacity_;
};

template<typename T>
void vector<T>::init(T* new_data, size_t new_size, size_t new_capacity) {
    data_ = new_data;
    size_ = new_size;
    capacity_ = new_capacity;
}

template<typename T>
void vector<T>::delete_data() {
    if (data_) {
        destroy_all(data_, size_);
        operator delete(data_);
    }
}

template<typename T>
vector<T>::vector() {
    data_ = nullptr;
    size_ = 0;
    capacity_ = 0;
}

template<typename T>
T* vector<T>::new_buffer(size_t size) const {
    T* new_data = static_cast<T*>(operator new(size * sizeof(T)));
    try {
        copy_all(new_data, data_, size_);
    } catch (...) {
        operator delete(new_data);
        throw;
    }
    return new_data;
}

template<typename T>
vector<T>::vector(vector const& other) : vector() {
    if (other.capacity_ != 0) {
        init(other.new_buffer(other.size_),
                other.size_, other.size_);
    }
}

template<typename T>
vector<T>& vector<T>::operator=(vector const& other) {
    if (this != &other) {
        vector(other).swap(*this);
    }
    return *this;
}

template<typename T>
void vector<T>::swap(vector& other) {
    using std::swap;

    swap(capacity_, other.capacity_);
    swap(size_, other.size_);
    swap(data_, other.data_);
}

template<typename T>
vector<T>::~vector() {
    delete_data();
}

template<typename T>
T& vector<T>::operator[](size_t i) {
    assert(i < size_);
    return data_[i];
}

template<typename T>
T const& vector<T>::operator[](size_t i) const {
    assert(i < size_);
    return *(data_ + i);
}

template<typename T>
T* vector<T>::data() {
    return data_;
}

template<typename T>
T const* vector<T>::data() const {
    return data_;
}

template<typename T>
size_t vector<T>::size() const {
    return size_;
}

template<typename T>
T& vector<T>::front() {
    assert(size_ != 0);
    return *data_;
}

template<typename T>
T const &vector<T>::front() const {
    return *data_;
}

template<typename T>
T& vector<T>::back() {
    assert(size_ != 0);
    return data_[size_ - 1];
}

template<typename T>
T const& vector<T>::back() const {
    assert(size_ != 0);
    return data_[size_ - 1];
}

template<typename T>
bool vector<T>::empty() const {
    return size_ == 0;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::begin() const {
    return data_;
}

template<typename T>
typename vector<T>::iterator vector<T>::begin() {
    return data_;
}

template<typename T>
size_t vector<T>::capacity() const {
    return capacity_;
}

template<typename T>
void vector<T>::reserve(size_t new_capacity) {
    if (new_capacity > capacity_) {
        T* new_data = new_buffer(new_capacity);
        delete_data();
        data_ = new_data;
        capacity_ = new_capacity;
    }
}

template<typename T>
void vector<T>::copy_all(T* dst, T const* src, size_t size) {
    size_t i = 0;
    try {
        for (; i < size; ++i) {
            new (dst + i) T(src[i]);
        }
    } catch (...) {
        destroy_all(dst, i);
        throw;
    }
}

template<typename T>
void vector<T>::destroy_all(T* data, size_t size) {
    for (size_t i = size; i > 0; --i) {
        data[i - 1].~T();
    }
}

template<typename T>
void vector<T>::clear() {
    destroy_all(data_, size_);
    size_ = 0;
}

template<typename T>
typename vector<T>::iterator vector<T>::end() {
    return data_ + size_;
}

template<typename T>
typename vector<T>::const_iterator vector<T>::end() const {
    return data_ + size_;
}

template<typename T>
void vector<T>::push_back(T const& el) {
    if (size_ != capacity_) {
        new (data_ + size_) T(el);
        ++size_;
    } else {
        push_back_realloc(el);
    }
}

template<typename T>
void vector<T>::push_back_realloc(T const& el) {
    vector<T> tmp(*this);
    tmp.reserve(capacity_? capacity_ * 2 : 1);
    tmp.push_back(el);
    swap(tmp);
}

template<typename T>
void vector<T>::pop_back() {
    assert(size_ != 0);
    data_[size_ - 1].~T();
    --size_;
}

template<typename T>
void vector<T>::shrink_to_fit() {
    if (size_ == capacity_) {
        return;
    }
    if (size_ != 0) {
        vector(*this).swap(*this);
    } else {
        operator delete (data_);
        init(nullptr, 0, 0);
    }
}

template<typename T>
typename vector<T>::iterator vector<T>::insert(vector::const_iterator pos, T const& v) {
    assert(pos - begin() >= 0);
    size_t old_size = size_;
    ptrdiff_t pos_ = pos - begin();
    push_back(v);
    for (size_t i = old_size; i > pos_; --i) {
        std::swap(*(begin() + i), *(begin() + i - 1));
    }
    return begin() + pos_;
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector::const_iterator pos) {
    return erase(pos, pos + 1);
}

template<typename T>
typename vector<T>::iterator vector<T>::erase(vector::const_iterator first, vector::const_iterator last) {
    assert(size_ != 0);
    ptrdiff_t shift_last = last - begin();
    ptrdiff_t shift_first = first - begin();
    std::move(begin() + shift_last, end(), begin() + shift_first);
    for (ptrdiff_t i = 0; i < last - first; ++i) {
        pop_back();
    }
    return begin() + shift_first;
}

#endif // VECTOR_H
