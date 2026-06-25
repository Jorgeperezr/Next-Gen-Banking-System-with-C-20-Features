#pragma once

#include <string>
#include <vector>

#include "banking/Money.hpp"
#include "banking/Transaction.hpp"

namespace banking {

class Account {
   public:
    // Etiqueta para reconstruir una cuenta desde disco sin generar un
    // movimiento automatico de "saldo inicial".
    struct RestoreTag {};

    // Cuenta nueva. `passwordHash` ya debe venir hasheado (ver Security.hpp).
    // Lanza std::invalid_argument si el saldo inicial es negativo.
    Account(std::string owner, std::string passwordHash, Money initialBalance);

    // Reconstruccion desde disco: fija los campos sin tocar el historial.
    Account(RestoreTag, std::string owner, std::string passwordHash,
            Money balance);

    const std::string& owner() const noexcept { return owner_; }
    Money balance() const noexcept { return balance_; }
    const std::vector<Transaction>& history() const noexcept {
        return history_;
    }
    const std::string& passwordHash() const noexcept { return passwordHash_; }

    bool verifyPassword(const std::string& password) const;
    void changeOwner(std::string newOwner) { owner_ = std::move(newOwner); }
    void setPasswordHash(std::string hash) {
        passwordHash_ = std::move(hash);
    }

    // Ingresa dinero. Lanza std::invalid_argument si `amount` no es positivo.
    void deposit(Money amount, std::string description = "Deposito",
                 Transaction::Type type = Transaction::Type::Deposit);

    // Retira dinero. Devuelve false si los fondos son insuficientes.
    // Lanza std::invalid_argument si `amount` no es positivo.
    bool withdraw(Money amount, std::string description = "Extraccion",
                  Transaction::Type type = Transaction::Type::Withdrawal);

    // Usado por la capa de persistencia al cargar movimientos guardados.
    void appendHistory(Transaction txn) {
        history_.push_back(std::move(txn));
    }

   private:
    std::string owner_;
    std::string passwordHash_;
    Money balance_;
    std::vector<Transaction> history_;
};

}  // namespace banking
