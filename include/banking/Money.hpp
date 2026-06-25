#pragma once

#include <algorithm>
#include <cmath>
#include <compare>
#include <cstdint>
#include <format>
#include <stdexcept>
#include <string>

namespace banking {

// Tipo seguro para representar dinero.
//
// Almacena la cantidad en CÉNTIMOS como un entero de 64 bits. Esto evita por
// completo los errores de redondeo del punto flotante (por ejemplo, en `double`
// la expresión 0.1 + 0.2 no es exactamente 0.3). En software financiero usar
// `double` para dinero es un error clásico; aquí lo evitamos por diseño.
class Money {
   public:
    constexpr Money() noexcept = default;

    // Construye Money a partir de céntimos (la unidad mínima).
    static constexpr Money fromCents(std::int64_t cents) noexcept {
        return Money(cents);
    }

    // Construye Money a partir de un valor decimal, redondeando al céntimo
    // más cercano (p. ej. 19.99 -> 1999 céntimos).
    static Money fromDouble(double amount) {
        if (!std::isfinite(amount)) {
            throw std::invalid_argument("El monto no es un número válido.");
        }
        return Money(static_cast<std::int64_t>(std::llround(amount * 100.0)));
    }

    constexpr std::int64_t cents() const noexcept { return cents_; }
    constexpr double toDouble() const noexcept {
        return static_cast<double>(cents_) / 100.0;
    }

    constexpr bool isPositive() const noexcept { return cents_ > 0; }
    constexpr bool isNegative() const noexcept { return cents_ < 0; }
    constexpr bool isZero() const noexcept { return cents_ == 0; }

    // --- Aritmética ---
    constexpr Money operator+(Money other) const noexcept {
        return Money(cents_ + other.cents_);
    }
    constexpr Money operator-(Money other) const noexcept {
        return Money(cents_ - other.cents_);
    }
    constexpr Money& operator+=(Money other) noexcept {
        cents_ += other.cents_;
        return *this;
    }
    constexpr Money& operator-=(Money other) noexcept {
        cents_ -= other.cents_;
        return *this;
    }

    // Comparación de tres vías (C++20): genera <, <=, >, >=, == y != de golpe.
    constexpr auto operator<=>(const Money&) const noexcept = default;
    constexpr bool operator==(const Money&) const noexcept = default;

    // Representación legible con separador de miles, p. ej. "$1,234.56".
    std::string toString() const {
        const bool negative = cents_ < 0;
        // Valor absoluto seguro incluso para INT64_MIN.
        const std::uint64_t abs =
            negative ? static_cast<std::uint64_t>(-(cents_ + 1)) + 1
                     : static_cast<std::uint64_t>(cents_);
        const std::uint64_t whole = abs / 100;
        const std::uint64_t frac = abs % 100;

        // Agrupar los miles: "1234567" -> "1,234,567".
        const std::string digits = std::to_string(whole);
        std::string grouped;
        int count = 0;
        for (auto it = digits.rbegin(); it != digits.rend(); ++it) {
            if (count != 0 && count % 3 == 0) grouped.push_back(',');
            grouped.push_back(*it);
            ++count;
        }
        std::reverse(grouped.begin(), grouped.end());

        return std::format("{}${}.{:02}", negative ? "-" : "", grouped, frac);
    }

   private:
    explicit constexpr Money(std::int64_t cents) noexcept : cents_(cents) {}
    std::int64_t cents_ = 0;
};

}  // namespace banking
