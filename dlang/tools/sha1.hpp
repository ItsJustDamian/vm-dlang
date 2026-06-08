#include <iostream>
#include <string>
#include <sstream>
#include <iomanip>
#include <vector>

class SHA1 {
private:
    uint32_t digest[5];
    std::string buffer;
    uint64_t transforms;

    void reset() {
        digest[0] = 0x67452301;
        digest[1] = 0xEFCDAB89;
        digest[2] = 0x98BADCFE;
        digest[3] = 0x10325476;
        digest[4] = 0xC3D2E1F0;
        buffer = "";
        transforms = 0;
    }

    uint32_t rol(const uint32_t value, const size_t bits) {
        return (value << bits) | (value >> (32 - bits));
    }

    void transform(uint32_t block[64]) {
        uint32_t w[80];
        for (size_t i = 0; i < 16; i++) {
            w[i] = (block[i * 4 + 0] << 24) | (block[i * 4 + 1] << 16) | (block[i * 4 + 2] << 8) | (block[i * 4 + 3]);
        }
        for (size_t i = 16; i < 80; i++) {
            w[i] = rol(w[i - 3] ^ w[i - 8] ^ w[i - 14] ^ w[i - 16], 1);
        }

        uint32_t a = digest[0], b = digest[1], c = digest[2], d = digest[3], e = digest[4];

        for (size_t i = 0; i < 80; i++) {
            uint32_t f = 0, k = 0;
            if (i < 20) { f = (b & c) | (~b & d); k = 0x5A827999; }
            else if (i < 40) { f = b ^ c ^ d; k = 0x6ED9EBA1; }
            else if (i < 60) { f = (b & c) | (b & d) | (c & d); k = 0x8F1BBCDC; }
            else { f = b ^ c ^ d; k = 0xCA62C1D6; }

            uint32_t temp = rol(a, 5) + f + e + k + w[i];
            e = d; d = c; c = rol(b, 30); b = a; a = temp;
        }

        digest[0] += a; digest[1] += b; digest[2] += c; digest[3] += d; digest[4] += e;
        transforms++;
    }

public:
    SHA1() { reset(); }

    std::string hash(const std::string& src) {
        reset();
        std::vector<uint32_t> block(16, 0);
        size_t orig_len = src.length();
        uint64_t bit_len = orig_len * 8;

        buffer = src;
        buffer.push_back((char)0x80);

        while (buffer.length() % 64 != 56) {
            buffer.push_back((char)0x00);
        }

        for (size_t i = 0; i < 14; i++) {
            uint32_t val = ((uint8_t)buffer[i * 4 + 0] << 24) | ((uint8_t)buffer[i * 4 + 1] << 16) |
                ((uint8_t)buffer[i * 4 + 2] << 8) | ((uint8_t)buffer[i * 4 + 3]);
            block[i] = val;
        }

        block[14] = (uint32_t)(bit_len >> 32);
        block[15] = (uint32_t)(bit_len & 0xFFFFFFFF);

        transform(block.data());

        std::stringstream ss;
        for (size_t i = 0; i < 5; i++) {
            ss << std::hex << std::setw(8) << std::setfill('0') << digest[i];
        }
        return ss.str();
    }
};