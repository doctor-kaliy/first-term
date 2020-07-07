#include "big_integer.h"

#include <cstring>
#include <stdexcept>
#include <algorithm>
#include <utility>
#include <climits>

using data_storage = uint_vector;

template<typename T>
using binary_operation = big_integer::binary_operation<T>;

static uint64_t const BASE64 = 0x100000000ULL;

big_integer::big_integer(int32_t sign, data_storage const& other_data) : data(other_data), sign(sign) {}

big_integer::big_integer() : sign(0) {}

big_integer::big_integer(big_integer const& other) = default;

big_integer::big_integer(data_storage const& other) : big_integer(1, other) {}

big_integer::big_integer(int a) {
    if (a != 0) {
        sign = a < 0 ? -1 : 1;
        if (a == INT_MIN) {
            data.push_back(static_cast<uint32_t>(INT_MAX) + 1);
        } else {
            data.push_back(std::abs(a));
        }
    } else {
        sign = 0;
    }
}

big_integer::big_integer(uint32_t a) : sign(a != 0) {
    if (a != 0) {
        data.push_back(a);
    }
}

big_integer::big_integer(std::string const& str) : big_integer() {
    size_t len = str.size();
    if (len == 0 || (len == 1 && !isdigit(str[0]))) {
        throw std::runtime_error("Invalid string");
    }
    size_t ptr = 0;
    while (str[0] == ' ') {
        ++ptr;
    }
    int32_t signum = 1;
    if (str[ptr] == '-') {
        signum = -1;
        ++ptr;
    } else if (str[ptr] == '+') {
        ++ptr;
    }
    while (str[ptr] == '0') {
        ptr++;
    }
    *this = 0;
    if (ptr == len) {
        signum = 0;
    } else {
        while (ptr < len) {
            if (!isdigit(str[ptr])) {
                throw std::runtime_error("Invalid string");
            }
            *this *= 10;
            *this += static_cast<int>(str[ptr] - '0');
            ++ptr;
        }
    }
    sign = signum;
}

size_t big_integer::size() const {
    return data.size();
}

static uint32_t lowest_8_bytes(uint64_t value) {
    return static_cast<uint32_t>(value & (BASE64 - 1));
}

static uint64_t get_data64(data_storage const& data, size_t id) {
    return data[id];
}

static int32_t compare_abs(data_storage const& words, data_storage const& other_words) {
    if (words.size() == other_words.size()) {
        size_t ptr = 0;
        while (ptr < words.size() && words[ptr] == other_words[ptr]) {
            ptr++;
        }
        if (ptr == words.size()) {
            return 0;
        }
        return (words[ptr] < other_words[ptr])? -1 : 1;
    }
    return (words.size() < other_words.size())? -1 : 1;
}

static uint32_t get_word(data_storage const& val, size_t n) {
    if (n >= val.size()) {
        return 0;
    }
    return val[val.size() - n - 1];
}

static void remove_zeroes(data_storage& v) {
    if (v.empty()) {
        return;
    }
    size_t ptr = 0;
    while (ptr < v.size() && v[ptr] == 0) {
        ptr++;
    }
    if (ptr == v.size()) {
        v.resize(0);
        return;
    }
    v.resize(std::move(v.begin() + ptr, v.end(), v.begin()) - v.begin());
}

static uint64_t add(uint32_t a, uint32_t b) {
    return static_cast<uint64_t>(a) + b;
}

static uint64_t sub(uint32_t a, uint32_t b) {
    return static_cast<uint64_t>(a) - b;
}

static void apply_arithmetic_long(data_storage& a, data_storage const& b,
                                  size_t begin, size_t end, binary_operation<uint64_t> const& op) {
    int32_t carry = 0;
    for (size_t i = begin; i < end; ++i) {
        uint64_t swc = op(get_word(a, i), get_word(b, i - begin)) + carry;
        a[a.size() - i - 1] = lowest_8_bytes(swc);
        carry = (swc >> 32ULL);
    }
}

static data_storage apply_binary_long(data_storage const& a, data_storage const &b,
                                      binary_operation<uint64_t> const& op) {
    data_storage res(std::max(a.size(), b.size()) + 1, 0);
    std::move(a.begin(), a.end(), res.end() - a.size());
    apply_arithmetic_long(res, b, 0, res.size(), op);
    remove_zeroes(res);
    return res;
}

static data_storage apply_add_long(data_storage const& a, data_storage const &b) {
    return apply_binary_long(a, b, add);
}

static data_storage apply_subtract_long(data_storage const& a, data_storage const &b) {
    return apply_binary_long(a, b, sub);
}

static void short_mul(data_storage &a, uint32_t rhs) {
    uint32_t carry = 0;
    for (size_t i = 0; i < a.size(); ++i) {
        uint64_t swc = static_cast<uint64_t>(get_word(a, i)) * rhs + carry;
        a[a.size() - i - 1] = lowest_8_bytes(swc);
        carry = (swc >> 32ULL);
    }
    if (carry) {
        a.insert(a.begin(), carry);
    }
}

big_integer& big_integer::add_signed(int32_t rhs_sign, data_storage const& rhs_words) {
    data_storage new_data = data;
    if (rhs_sign == 0) {
        return *this;
    } else if (sign == 0) {
        sign = rhs_sign;
        new_data = rhs_words;
    } else if (sign == rhs_sign) {
        new_data = apply_add_long(new_data, rhs_words);
    } else {
        int32_t cmp = compare_abs(new_data, rhs_words);
        if (cmp == 0) {
            new_data.clear();
            sign = 0;
        } else {
            new_data = (cmp > 0 ? apply_subtract_long(new_data, rhs_words) : apply_subtract_long(rhs_words, new_data));
            sign = (sign == cmp? 1 : -1);
        }
    }
    data = new_data;
    return *this;
}

big_integer& big_integer::operator+=(big_integer const& rhs) {
    return add_signed(rhs.sign, rhs.data);
}

big_integer& big_integer::operator-=(big_integer const& rhs) {
    return add_signed(-rhs.sign, rhs.data);
}

big_integer& big_integer::operator*=(big_integer const& rhs) {
    if (sign != 0U && rhs.sign == 0U) {
        *this = 0U;
    }
    if (sign == 0U) {
        return *this;
    }

    data_storage other_data(rhs.data);
    data.insert(data.begin(), 0U);
    data_storage res(data.size() + other_data.size(), 0U);

    for (size_t i = 0; i < other_data.size(); i++) {
        data_storage tmp(data);
        short_mul(tmp, other_data[other_data.size() - i - 1]);
        apply_arithmetic_long(res, tmp, i, tmp.size() + i, add);
    }

    remove_zeroes(res);
    return (*this = big_integer(sign * rhs.sign, res));
}

static void short_div(data_storage& data, uint32_t rhs) {
    uint64_t rest = 0;
    for (uint32_t& i : data) {
        uint64_t x = (rest << 32ULL) | i;
        i = lowest_8_bytes(x / rhs);
        rest = x % rhs;
    }
    remove_zeroes(data);
}

static bool smaller(data_storage const &a, size_t start, data_storage const &b, size_t prefix) {
    size_t i = 0;
    while (i < prefix && a[i + start] == get_word(b, prefix - i - 1)) {
        ++i;
    }
    return i < prefix && a[i + start] < get_word(b, prefix - i - 1);
}

static void difference(data_storage& a, size_t st, data_storage const& b, size_t prefix) {
    if (a.size() - st < prefix) {
        return;
    }
    apply_arithmetic_long(a, b, a.size() - st - prefix, a.size() - st, sub);
}

static __uint128_t get_word128(data_storage const& data, size_t i) {
    return get_word(data, i);
}

static __uint128_t build128(data_storage const& data, size_t size, size_t start) {
    __uint128_t res = 0;
    for (size_t i = start; i < size + start; ++i) {
        res += get_word128(data, data.size() - i - 1) << ((size - 1ULL - i + start) * 32ULL);
    }
    return res;
}

big_integer& big_integer::operator/=(big_integer const &other) {
    if (compare_abs(data, other.data) < 0) {
        return (*this = 0);
    }

    sign *= other.sign;

    if (other.size() == 1) {
        short_div(data, other.data[0]);
        return *this;
    }

    data_storage this_abs(data);
    data_storage other_abs(other.data);

    uint32_t f = lowest_8_bytes(BASE64
                                / (get_data64(other_abs, 0) + 1));
    short_mul(this_abs, f);
    short_mul(other_abs, f);

    this_abs.insert(this_abs.begin(), 0);
    size_t m = other_abs.size() + 1;
    size_t n = this_abs.size();
    data.resize(n - m + 1);

    for (size_t i = m, j = 0; i <= n; ++i, ++j) {
        __uint128_t x = build128(this_abs, 3, j);
        __uint128_t y = build128(other_abs, 2, 0);

        uint32_t qt = lowest_8_bytes(std::min(static_cast<uint64_t>(x / y), BASE64 - 1));
        data_storage dq(other_abs);
        short_mul(dq, qt);

        if (smaller(this_abs, j, dq, m)) {
            qt--;
            apply_subtract_long(dq, other_abs);
        }

        data[j] = qt;
        difference(this_abs, j, dq, m);
    }

    remove_zeroes(data);
    return *this;
}

big_integer& big_integer::operator%=(big_integer const& rhs) {
    *this -= (*this / rhs) * rhs;
    return *this;
}

static size_t not_zero_id(data_storage const& value) {
    for (size_t i = value.size(); i > 0; --i) {
        if (value[i - 1] != 0) {
            return value.size() - i;
        }
    }
    return value.size();
}

uint32_t big_integer::get_signed(size_t id, size_t not_zero_pos) const {
    if (sign == 0) {
        return 0;
    }
    if (id > data.size()) {
        return sign == 1? 0 : -1;
    } else if (id == data.size()) {
        uint32_t word = 0;
        return sign == 1? word : (id <= not_zero_pos? -word : ~word);
    } else {
        uint32_t word = data[data.size() - id - 1];
        return sign == 1? word : (id <= not_zero_pos? -word : ~word);
    }
}

static big_integer get_value(data_storage& value) {
    if (!value.empty() && value[0] >> 31u) {
        for (uint32_t& i : value) {
            i = ~i;
        }
        remove_zeroes(value);
        return big_integer(-1, value) - 1;
    }
    remove_zeroes(value);
    if (value.empty()) {
        return 0;
    }
    return big_integer(value);
}


big_integer& big_integer::bit_operation(big_integer const& rhs,
                                        binary_operation<uint32_t> const& op) {
    data_storage result(std::max(data.size(), rhs.data.size()) + 1, 0);
    size_t pos1 = not_zero_id(data);
    size_t pos2 = not_zero_id(rhs.data);
    for (size_t i = 0; i < result.size(); ++i) {
        result[i] = op(get_signed(result.size() - i - 1, pos1),
                       rhs.get_signed(result.size() - i - 1, pos2));
    }
    *this = get_value(result);
    return *this;
}

uint32_t bit_and(uint32_t a, uint32_t b) {
    return a & b;
}

uint32_t bit_or(uint32_t a, uint32_t b) {
    return a | b;
}

uint32_t bit_xor(uint32_t a, uint32_t b) {
    return a ^ b;
}

big_integer& big_integer::operator&=(big_integer const& rhs) {
    return bit_operation(rhs, bit_and);
}

big_integer& big_integer::operator|=(big_integer const& rhs) {
    return bit_operation(rhs, bit_or);
}

big_integer& big_integer::operator^=(big_integer const& rhs) {
    return bit_operation(rhs, bit_xor);
}

big_integer& big_integer::operator<<=(int rhs) {
    if (rhs < 0) {
        return *this >>= (-rhs);
    }
    size_t big_shift = rhs / 32;
    uint32_t small_shift = rhs % 32;
    data.resize(big_shift + data.size(), 0U);
    if (small_shift == 0) {
        return *this;
    }
    return (*this *= static_cast<uint32_t>(1ULL << small_shift));
}

big_integer& big_integer::operator>>=(int rhs) {
    if (rhs < 0) {
        return *this <<= (-rhs);
    }
    size_t big_shift = rhs / 32;
    uint32_t small_shift = rhs % 32;
    if (big_shift >= data.size()) {
        return (*this = 0);
    }
    size_t pos = not_zero_id(data);
    data.resize(data.size() - big_shift);
    data.insert(data.begin(), 0);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = get_signed(data.size() - i - 1, pos);
    }
    uint64_t tmp = get_data64(data, 0);
    uint64_t shifted = ((tmp << (32ULL - small_shift))
                        | (tmp << 32U));
    data[0] = shifted >> 32ULL;
    shifted <<= 32ULL;
    for (size_t i = 1; i < data.size(); ++i) {
        shifted |= (get_data64(data, i) << (32ULL - small_shift));
        data[i] = shifted >> 32ULL;
        shifted <<= 32ULL;
    }
    return (*this = get_value(data));
}

big_integer big_integer::operator+() const {
    return *this;
}

big_integer big_integer::operator-() const {
    return big_integer(-this->sign, this->data);
}

big_integer big_integer::operator~() const {
    return -*this - 1;
}

big_integer& big_integer::operator++() {
    return (*this += 1);
}

big_integer big_integer::operator++(int) {
    big_integer r(*this);
    ++*this;
    return r;
}

big_integer& big_integer::operator--() {
    return (*this -= 1);
}

big_integer big_integer::operator--(int) {
    big_integer r = *this;
    --*this;
    return r;
}

big_integer operator+(big_integer a, big_integer const& b) {
    return a += b;
}

big_integer operator-(big_integer a, big_integer const& b) {
    return a -= b;
}

big_integer operator*(big_integer a, big_integer const& b) {
    return a *= b;
}

big_integer operator/(big_integer a, big_integer const& b) {
    return a /= b;
}

big_integer operator%(big_integer a, big_integer const& b) {
    return a %= b;
}

big_integer operator&(big_integer a, big_integer const& b) {
    return a &= b;
}

big_integer operator|(big_integer a, big_integer const& b) {
    return a |= b;
}

big_integer operator^(big_integer a, big_integer const& b) {
    return a ^= b;
}

big_integer operator<<(big_integer a, int b) {
    return a <<= b;
}

big_integer operator>>(big_integer a, int b) {
    return a >>= b;
}

bool operator==(big_integer const& a, big_integer const& b) {
    return !(a < b) && !(b < a);
}

bool operator!=(big_integer const& a, big_integer const& b) {
    return !(a == b);
}

bool operator<(big_integer const& a, big_integer const& b) {
    if (a.sign == b.sign) {
        if (a.sign == 0) {
            return false;
        }
        return ((compare_abs(a.data, b.data) * a.sign) < 0);
    }
    return a.sign < b.sign;
}

bool operator>(big_integer const& a, big_integer const& b) {
    return !(a <= b);
}

bool operator<=(big_integer const& a, big_integer const& b) {
    return a < b || a == b;
}

bool operator>=(big_integer const& a, big_integer const& b) {
    return !(a < b);
}

std::string to_string(big_integer const& a) {
    std::string result;
    if (a.sign == 0) {
        return "0";
    }
    big_integer s(a);
    s.sign = 1;
    while (s > 0) {
        big_integer temp(s % 10);
        if (temp == 0) {
            result.push_back('0');
        } else {
            result.push_back(temp.data[0] + '0');
        }
        s /= 10;
    }
    if (a.sign == -1) {
        result.push_back('-');
    }
    reverse(result.begin(), result.end());
    return result;
}

big_integer &big_integer::operator=(big_integer const &other) {
    big_integer tmp(other);
    data.swap(tmp.data);
    std::swap(sign, tmp.sign);
    return *this;
}

std::ostream& operator<<(std::ostream& s, big_integer const& a) {
    return s << to_string(a);
}
