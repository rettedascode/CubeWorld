#include "net/ByteBuffer.h"

#include <cstring>

namespace net {

void ByteBuffer::writeU8(uint8_t v) { m_data.push_back(v); }

void ByteBuffer::writeU16(uint16_t v) {
    m_data.push_back(static_cast<uint8_t>(v & 0xFF));
    m_data.push_back(static_cast<uint8_t>((v >> 8) & 0xFF));
}

void ByteBuffer::writeU32(uint32_t v) {
    for (int i = 0; i < 4; ++i) m_data.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
}

void ByteBuffer::writeU64(uint64_t v) {
    for (int i = 0; i < 8; ++i) m_data.push_back(static_cast<uint8_t>((v >> (i * 8)) & 0xFF));
}

void ByteBuffer::writeFloat(float v) {
    uint32_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    writeU32(bits);
}

void ByteBuffer::writeDouble(double v) {
    uint64_t bits;
    std::memcpy(&bits, &v, sizeof(bits));
    writeU64(bits);
}

void ByteBuffer::writeString(const std::string& s) {
    writeU16(static_cast<uint16_t>(s.size()));
    writeBytes(s.data(), s.size());
}

void ByteBuffer::writeBytes(const void* p, size_t n) {
    const auto* b = static_cast<const uint8_t*>(p);
    m_data.insert(m_data.end(), b, b + n);
}

bool ByteBuffer::ensure(size_t n) {
    if (remaining() < n) {
        m_error = true;
        return false;
    }
    return true;
}

uint8_t ByteBuffer::readU8() {
    if (!ensure(1)) return 0;
    return m_data[m_read++];
}

uint16_t ByteBuffer::readU16() {
    if (!ensure(2)) return 0;
    uint16_t v = static_cast<uint16_t>(m_data[m_read]) |
                 (static_cast<uint16_t>(m_data[m_read + 1]) << 8);
    m_read += 2;
    return v;
}

uint32_t ByteBuffer::readU32() {
    if (!ensure(4)) return 0;
    uint32_t v = 0;
    for (int i = 0; i < 4; ++i) v |= static_cast<uint32_t>(m_data[m_read + i]) << (i * 8);
    m_read += 4;
    return v;
}

uint64_t ByteBuffer::readU64() {
    if (!ensure(8)) return 0;
    uint64_t v = 0;
    for (int i = 0; i < 8; ++i) v |= static_cast<uint64_t>(m_data[m_read + i]) << (i * 8);
    m_read += 8;
    return v;
}

float ByteBuffer::readFloat() {
    uint32_t bits = readU32();
    float v;
    std::memcpy(&v, &bits, sizeof(v));
    return v;
}

double ByteBuffer::readDouble() {
    uint64_t bits = readU64();
    double v;
    std::memcpy(&v, &bits, sizeof(v));
    return v;
}

std::string ByteBuffer::readString() {
    const uint16_t len = readU16();
    if (!ensure(len)) return {};
    std::string s(reinterpret_cast<const char*>(&m_data[m_read]), len);
    m_read += len;
    return s;
}

} // namespace net
