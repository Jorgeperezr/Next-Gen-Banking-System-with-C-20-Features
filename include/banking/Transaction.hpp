#pragma once

#include <chrono>
#include <string>
#include <utility>

#include "banking/Money.hpp"

namespace banking {

// Registro de un único movimiento en una cuenta. Inmutable una vez creado.
class Transaction {
   public:
    enum class Type { Deposit, Withdrawal, TransferIn, TransferOut };

    // Movimiento nuevo: marca el instante actual.
    Transaction(Type type, Money amount, Money balanceAfter,
                std::string description)
        : type_(type),
          amount_(amount),
          balanceAfter_(balanceAfter),
          description_(std::move(description)),
          timestamp_(std::chrono::system_clock::now()) {}

    // Reconstrucción desde disco: el instante ya se conoce.
    Transaction(Type type, Money amount, Money balanceAfter,
                std::string description,
                std::chrono::system_clock::time_point timestamp)
        : type_(type),
          amount_(amount),
          balanceAfter_(balanceAfter),
          description_(std::move(description)),
          timestamp_(timestamp) {}

    Type type() const noexcept { return type_; }
    Money amount() const noexcept { return amount_; }
    Money balanceAfter() const noexcept { return balanceAfter_; }
    const std::string& description() const noexcept { return description_; }
    std::chrono::system_clock::time_point timestamp() const noexcept {
        return timestamp_;
    }

    const char* typeLabel() const noexcept {
        switch (type_) {
            case Type::Deposit:     return "Deposito";
            case Type::Withdrawal:  return "Extraccion";
            case Type::TransferIn:  return "Transf. recibida";
            case Type::TransferOut: return "Transf. enviada";
        }
        return "Desconocido";
    }

   private:
    Type type_;
    Money amount_;
    Money balanceAfter_;
    std::string description_;
    std::chrono::system_clock::time_point timestamp_;
};

}  // namespace banking
