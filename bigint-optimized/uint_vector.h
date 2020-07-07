#ifndef BIGINT_UINT_VECTOR_H
#define BIGINT_UINT_VECTOR_H

#include <algorithm>
#include <cstddef>
#include <cassert>
#include <iostream>

struct shared_buffer {
    size_t ref_count;
    size_t capacity;
    uint32_t words[];
};

struct uint_vector {
    typedef uint32_t *iterator;
    typedef uint32_t const *const_iterator;

    uint_vector();                    // O(1) nothrow
    uint_vector(uint_vector const &);                  // O(N) strong
    uint_vector(size_t, uint32_t const &);

    uint_vector &operator=(uint_vector const &); // O(N) strong

    ~uint_vector();                              // O(N) nothrow

    uint32_t &operator[](size_t);                // O(1) nothrow
    uint32_t const &operator[](size_t) const;    // O(1) nothrow

    size_t size() const;                    // O(1) nothrow

    uint32_t &back();                              // O(1) nothrow
    uint32_t const &back() const;                  // O(1) nothrow
    void push_back(uint32_t const &);               // O(1)* strong
    void pop_back();                        // O(1) nothrow

    bool empty() const;                     // O(1) nothrow

    void reserve(size_t);                   // O(N) strong

    void clear();                           // O(N) nothrow
    void resize(size_t);

    void resize(size_t, uint32_t const &);

    void swap(uint_vector &);                     // O(1) nothrow

    iterator begin();                       // O(1) nothrow
    iterator end();                         // O(1) nothrow

    const_iterator begin() const;           // O(1) nothrow
    const_iterator end() const;             // O(1) nothrow

    iterator insert(const_iterator pos, uint32_t const &);

private:
    static void copy_all(shared_buffer *, shared_buffer const *, size_t);
    static shared_buffer *allocate_buffer(size_t);

    bool small() const;
    size_t capacity();
    shared_buffer *new_buffer(size_t) const;
    void delete_data();
    void unshare();

    static const size_t BYTES = 8 + sizeof(shared_buffer*);
    static const size_t STATIC_SIZE = BYTES / sizeof(uint32_t);

    size_t size_;
    union {
        shared_buffer* dynamic_data;
        uint32_t static_data[STATIC_SIZE];
    } storage;
};

#endif // BIGINT_UINT_VECTOR_H
