#include "banking/Account.hpp"

#include <stdexcept>
#include <utility>

#include "banking/Security.hpp"

namespace banking {

Account::Account(std::string owner, std::string passwordHash,
                 Money initialBalance)
    : owner_(std::move(owner)),
      passwordHash_(std::move(passwordHash)),
      balance_(initialBalance) {
    if (initialBalance.isNegative()) {
        throw std::invalid_argument("El saldo inicial no puede ser negativo.");
    }
    if (initialBalance.isPositive()) {
        history_.emplace_back(Transaction::Type::Deposit, initialBalance,
                              balance_, "Saldo inicial");
    }
}

Account::Account(RestoreTag, std::string owner, std::string passwordHash,
                 Money balance)
    : owner_(std::move(owner)),
      passwordHash_(std::move(passwordHash)),
      balance_(balance) {}

bool Account::verifyPassword(const std::string& password) const {
    return security::verifyPassword(password, passwordHash_);
}

void Account::deposit(Money amount, std::string description,
                      Transaction::Type type) {
    if (!amount.isPositive()) {
        throw std::invalid_argument("El monto a depositar debe ser positivo.");
    }
    balance_ += amount;
    history_.emplace_back(type, amount, balance_, std::move(description));
}

bool Account::withdraw(Money amount, std::string description,
                       Transaction::Type type) {
    if (!amount.isPositive()) {
        throw std::invalid_argument("El monto a extraer debe ser positivo.");
    }
    if (amount > balance_) {
        return false;  // fondos insuficientes
    }
    balance_ -= amount;
    history_.emplace_back(type, amount, balance_, std::move(description));
    return true;
}

}  // namespace banking
