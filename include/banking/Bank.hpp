#pragma once

#include <cstddef>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

#include "banking/Account.hpp"
#include "banking/Money.hpp"

namespace banking {

// Resultado explicito de cada operacion, para dar mensajes de error precisos
// en vez de un simple bool (que no distingue "no existe" de "mal password").
enum class Result {
    Success,
    AccountNotFound,
    SourceNotFound,
    TargetNotFound,
    WrongPassword,
    InsufficientFunds,
    InvalidAmount,
    DuplicateId,
    SameAccount,
};

// Traduce un Result a un mensaje en espanol para mostrar al usuario.
std::string toMessage(Result r);

class Bank {
   public:
    // Crea una cuenta. La contrasena se hashea automaticamente; el texto plano
    // nunca se almacena.
    Result createAccount(int id, const std::string& owner,
                         const std::string& password, Money initialBalance);

    // Deposito (no requiere contrasena: ingresar dinero no es sensible).
    Result deposit(int id, Money amount);

    Result withdraw(int id, Money amount, const std::string& password);

    Result transfer(int sourceId, int targetId, Money amount,
                    const std::string& password);

    Result changeOwner(int id, const std::string& newOwner,
                       const std::string& password);

    Result changePassword(int id, const std::string& newPassword,
                          const std::string& currentPassword);

    // Acceso de solo lectura; nullptr si la cuenta no existe.
    const Account* find(int id) const;

    std::vector<int> accountIds() const;
    std::size_t size() const noexcept { return accounts_.size(); }

    // --- Persistencia en archivo de texto ---
    bool saveToFile(const std::string& path) const;
    bool loadFromFile(const std::string& path);

   private:
    std::unordered_map<int, std::unique_ptr<Account>> accounts_;
};

}  // namespace banking
