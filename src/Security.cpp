#include "banking/Security.hpp"

#include <array>
#include <cstdint>
#include <format>
#include <random>
#include <string_view>

namespace banking::security {
namespace {

// Constantes del algoritmo FNV-1a de 64 bits.
constexpr std::uint64_t kFnvOffset = 14695981039346656037ULL;
constexpr std::uint64_t kFnvPrime = 1099511628211ULL;

// Numero de rondas de estiramiento. Cuanto mayor, mas caro es probar cada
// contrasena por fuerza bruta (a costa de un login un poco mas lento).
constexpr int kStretchRounds = 150000;

std::uint64_t fnv1a(std::string_view data, std::uint64_t seed) noexcept {
    std::uint64_t hash = seed;
    for (unsigned char b : data) {
        hash ^= b;
        hash *= kFnvPrime;
    }
    return hash;
}

// Mezcla los 8 bytes del hash anterior con la sal, una ronda.
std::uint64_t mixOnce(std::uint64_t h, std::uint64_t salt) noexcept {
    std::array<unsigned char, 8> bytes{};
    for (int i = 0; i < 8; ++i) {
        bytes[i] = static_cast<unsigned char>((h >> (i * 8)) & 0xFFu);
    }
    std::uint64_t hash = kFnvOffset ^ salt;
    for (unsigned char b : bytes) {
        hash ^= b;
        hash *= kFnvPrime;
    }
    return hash;
}

std::uint64_t deriveHash(const std::string& password,
                         std::uint64_t salt) noexcept {
    std::uint64_t h = fnv1a(password, kFnvOffset ^ salt);
    for (int i = 0; i < kStretchRounds; ++i) {
        h = mixOnce(h, salt);
    }
    return h;
}

std::uint64_t randomSalt() {
    std::random_device rd;
    const std::uint64_t hi = rd();
    const std::uint64_t lo = rd();
    return (hi << 32) ^ lo;
}

}  // namespace

std::string hashPassword(const std::string& password) {
    const std::uint64_t salt = randomSalt();
    const std::uint64_t hash = deriveHash(password, salt);
    return std::format("{:016x}${:016x}", salt, hash);
}

bool verifyPassword(const std::string& password, const std::string& stored) {
    const auto sep = stored.find('$');
    if (sep == std::string::npos) return false;

    std::uint64_t salt = 0;
    std::uint64_t expected = 0;
    try {
        salt = std::stoull(stored.substr(0, sep), nullptr, 16);
        expected = std::stoull(stored.substr(sep + 1), nullptr, 16);
    } catch (...) {
        return false;
    }

    const std::uint64_t actual = deriveHash(password, salt);

    // Comparacion sin ramas dependientes del valor, para no filtrar
    // informacion por el tiempo de ejecucion (timing attack).
    return (actual ^ expected) == 0;
}

}  // namespace banking::security
