#include <iostream>
#include <vector>
#include <cassert>

const uint8_t k1ByteMarker = 0b00000000;
const uint8_t k2ByteMarker = 0b11000000;
const uint8_t k3ByteMarker = 0b11100000;
const uint8_t k4ByteMarker = 0b11110000;

const uint8_t k1ByteMask =   0b10000000;
const uint8_t k2ByteMask =   0b11100000;
const uint8_t k3ByteMask =   0b11110000;
const uint8_t k4ByteMask =   0b11111000;

const uint32_t k1ByteMaxCodePoint = 0x007F;
const uint32_t k2ByteMaxCodePoint = 0x07FF;
const uint32_t k3ByteMaxCodePoint = 0xFFFF;
const uint32_t k4ByteMaxCodePoint = 0x10FFFF;

const uint8_t k1ByteFirstByteCapacity = 7;
const uint8_t k2ByteFirstByteCapacity = 5;
const uint8_t k3ByteFirstByteCapacity = 4;
const uint8_t k4ByteFirstByteCapacity = 3;

const uint8_t k1ByteCapacity = 7;
const uint8_t k2ByteCapacity = 11;
const uint8_t k3ByteCapacity = 16;
const uint8_t k4ByteCapacity = 21;

const uint8_t kResidualByteMarker = 0b10000000;
const uint8_t kResidualByteMask =   0b11000000;
const uint8_t kResidualByteCapacity = 6;



// Unicode standard restriction: UTF-32 cannot encode
// code points greater than U+10FFFF
const uint32_t kMaxCodePoint = k4ByteMaxCodePoint;

std::vector<uint32_t> utf8_to_utf32(const std::vector<uint8_t> &str) {
    std::vector<uint32_t> result;
    size_t idx = 0;
    while(idx < str.size()) {  // Loop that extracts one symbol per iter
        uint32_t wide_symbol;
        uint8_t length;
        if ((str[idx] & k4ByteMask) == k4ByteMarker) {
            length = 4;
            wide_symbol = str[idx] & ~k4ByteMask;
        } else if ((str[idx] & k3ByteMask) == k3ByteMarker) {
            length = 3;
            wide_symbol = str[idx] & ~k3ByteMask;
        } else if ((str[idx] & k2ByteMask) == k2ByteMarker) {
            length = 2;
            wide_symbol = str[idx] & ~k2ByteMask;
        } else {
            if ((str[idx] & k2ByteMask) != k1ByteMarker) {
                throw std::runtime_error("Unknown starting byte");
            }
            length = 1;
            wide_symbol = str[idx] & ~k1ByteMask;
        }
        if (idx + length > str.size()) {
            throw std::runtime_error("String too short");
        }
        for (uint8_t byte = 1; byte < length; ++byte) {
            if ((str[idx + byte] & kResidualByteMask)
                            != kResidualByteMarker) {
                throw std::runtime_error("Invalid residual byte");
            }
            wide_symbol = wide_symbol << kResidualByteCapacity;
            wide_symbol |= str[idx + byte] & ~kResidualByteMask;
        }
        result.push_back(wide_symbol);
        idx += length;
    }
    return result;
}

/*
 * Returns a bit sequence in range [from, to].
 */
uint32_t substr(uint32_t str, uint8_t from, uint8_t to) {
    return ((str << (31 - to)) >> (31 - to)) >> from;
}

std::vector<uint8_t> utf32_to_utf8(const std::vector<uint32_t> &str) {
    std::vector<uint8_t> result;
    for (size_t idx = 0; idx < str.size(); ++idx) {
        uint32_t wide_symbol = str[idx];
        uint8_t length, remaining_bits;
        if (wide_symbol > kMaxCodePoint) {
            throw std::runtime_error("Unsupported symbol");
        }
        if (wide_symbol <= k1ByteMaxCodePoint) {
            uint8_t byte = k1ByteMarker;
            length = 1;
            remaining_bits = 0;
            byte |= (uint8_t) wide_symbol;
            result.push_back(byte);
        } else if (wide_symbol <= k2ByteMaxCodePoint) {
            uint8_t byte = k2ByteMarker;
            length = 2;
            remaining_bits = k2ByteCapacity - k2ByteFirstByteCapacity;
            byte |= (uint8_t) substr(wide_symbol, remaining_bits, k2ByteCapacity - 1);
            result.push_back(byte);
        } else if (wide_symbol <= k3ByteMaxCodePoint) {
            uint8_t byte = k3ByteMarker;
            length = 3;
            remaining_bits = k3ByteCapacity - k3ByteFirstByteCapacity;
            byte |= (uint8_t) substr(wide_symbol, remaining_bits, k3ByteCapacity - 1);
            result.push_back(byte);
        } else {
            uint8_t byte = k4ByteMarker;
            length = 4;
            remaining_bits = k4ByteCapacity - k4ByteFirstByteCapacity;
            byte |= (uint8_t) substr(wide_symbol, remaining_bits, k4ByteCapacity - 1);
            result.push_back(byte);
        }
        for (size_t i = 1; i < length; ++i) {
            uint8_t byte = kResidualByteMarker;
            uint8_t to = remaining_bits - 1;
            remaining_bits -= kResidualByteCapacity;
            byte |= (uint8_t) substr(wide_symbol, remaining_bits, to);
            result.push_back(byte);
        }
    }
    return result;
}

int main() {
    std::vector<uint8_t> utf8_str = {0xF0, 0x9F, 0x92, 0xAF};
    std::vector<uint32_t> utf32_str = utf8_to_utf32(utf8_str);
    for (uint32_t i : utf32_str) {
        std::cout << i << " ";
    }
    std::cout << std::endl;
    std::vector<uint8_t> new_utf8_str = utf32_to_utf8(utf32_str);
    for (uint8_t i : new_utf8_str) {
        std::cout << (uint32_t) i << " ";
    }
    std::cout << std::endl;
    assert(utf8_str == new_utf8_str);
}
