#pragma once
#include <string>

// FNV-1a constanten voor 32-bit
constexpr uint32_t FNV_OFFSET_BASIS = 2166136261U;
constexpr uint32_t FNV_PRIME = 16777619U;

// Constexpr hash functie: werkt zowel compile-time als runtime
constexpr uint32_t fnv1a_hash(const char* str, uint32_t hash = FNV_OFFSET_BASIS) {
    return (*str == '\0') ? hash : fnv1a_hash(str + 1, (hash ^ static_cast<uint32_t>(*str)) * FNV_PRIME);
}

// Helper voor std::string (alleen runtime)
inline uint32_t fnv1a_hash(const std::string& str) {
    return fnv1a_hash(str.c_str());
}