#pragma once

#include <cstdint>
#include <string>
#include <vector>

// net: game-agnostic reliable-UDP transport + serialization. NO OpenGL, used by
// both server and client.
namespace net {

// Little-endian binary (de)serializer. Writing appends; reading advances a
// cursor. On a short/invalid read, reads return 0/empty and ok() becomes false
// so callers can reject malformed packets instead of reading garbage.
class ByteBuffer {
public:
    ByteBuffer() = default;
    ByteBuffer(const uint8_t* data, size_t size) : m_data(data, data + size) {}

    // --- writing ---
    void writeU8(uint8_t v);
    void writeU16(uint16_t v);
    void writeU32(uint32_t v);
    void writeU64(uint64_t v);
    void writeI8(int8_t v)   { writeU8(static_cast<uint8_t>(v)); }
    void writeI16(int16_t v) { writeU16(static_cast<uint16_t>(v)); }
    void writeI32(int32_t v) { writeU32(static_cast<uint32_t>(v)); }
    void writeI64(int64_t v) { writeU64(static_cast<uint64_t>(v)); }
    void writeFloat(float v);
    void writeDouble(double v);
    void writeBool(bool v) { writeU8(v ? 1 : 0); }
    void writeString(const std::string& s); // u16 length prefix + bytes
    void writeBytes(const void* p, size_t n);

    // --- reading ---
    uint8_t  readU8();
    uint16_t readU16();
    uint32_t readU32();
    uint64_t readU64();
    int8_t  readI8()  { return static_cast<int8_t>(readU8()); }
    int16_t readI16() { return static_cast<int16_t>(readU16()); }
    int32_t readI32() { return static_cast<int32_t>(readU32()); }
    int64_t readI64() { return static_cast<int64_t>(readU64()); }
    float   readFloat();
    double  readDouble();
    bool    readBool() { return readU8() != 0; }
    std::string readString();

    bool   ok() const { return !m_error; }
    size_t remaining() const { return m_data.size() - m_read; }
    const uint8_t* data() const { return m_data.data(); }
    size_t size() const { return m_data.size(); }

private:
    bool ensure(size_t n); // mark error if fewer than n bytes remain

    std::vector<uint8_t> m_data;
    size_t               m_read  = 0;
    bool                 m_error = false;
};

} // namespace net
