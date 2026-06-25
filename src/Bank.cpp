#include "banking/Bank.hpp"

#include <algorithm>
#include <chrono>
#include <fstream>
#include <sstream>
#include <string>
#include <utility>

#include "banking/Security.hpp"
#include "banking/Transaction.hpp"

namespace banking {

std::string toMessage(Result r) {
    switch (r) {
        case Result::Success:           return "Operacion exitosa.";
        case Result::AccountNotFound:   return "Error: cuenta no encontrada.";
        case Result::SourceNotFound:    return "Error: cuenta de origen no encontrada.";
        case Result::TargetNotFound:    return "Error: cuenta de destino no encontrada.";
        case Result::WrongPassword:     return "Error: contrasena incorrecta.";
        case Result::InsufficientFunds: return "Error: fondos insuficientes.";
        case Result::InvalidAmount:     return "Error: el monto debe ser positivo.";
        case Result::DuplicateId:       return "Error: el ID de cuenta ya esta en uso.";
        case Result::SameAccount:       return "Error: origen y destino no pueden ser la misma cuenta.";
    }
    return "Error desconocido.";
}

Result Bank::createAccount(int id, const std::string& owner,
                           const std::string& password, Money initialBalance) {
    if (initialBalance.isNegative()) return Result::InvalidAmount;
    if (accounts_.contains(id)) return Result::DuplicateId;

    accounts_[id] = std::make_unique<Account>(
        owner, security::hashPassword(password), initialBalance);
    return Result::Success;
}

Result Bank::deposit(int id, Money amount) {
    if (!amount.isPositive()) return Result::InvalidAmount;
    auto it = accounts_.find(id);
    if (it == accounts_.end()) return Result::AccountNotFound;
    it->second->deposit(amount);
    return Result::Success;
}

Result Bank::withdraw(int id, Money amount, const std::string& password) {
    if (!amount.isPositive()) return Result::InvalidAmount;
    auto it = accounts_.find(id);
    if (it == accounts_.end()) return Result::AccountNotFound;
    if (!it->second->verifyPassword(password)) return Result::WrongPassword;
    if (!it->second->withdraw(amount)) return Result::InsufficientFunds;
    return Result::Success;
}

Result Bank::transfer(int sourceId, int targetId, Money amount,
                      const std::string& password) {
    if (!amount.isPositive()) return Result::InvalidAmount;
    if (sourceId == targetId) return Result::SameAccount;

    auto src = accounts_.find(sourceId);
    if (src == accounts_.end()) return Result::SourceNotFound;
    auto dst = accounts_.find(targetId);
    if (dst == accounts_.end()) return Result::TargetNotFound;

    if (!src->second->verifyPassword(password)) return Result::WrongPassword;

    // Comprobamos los fondos retirando primero; si falla, no se toca el destino.
    const std::string outDesc =
        "Transferencia a cuenta " + std::to_string(targetId);
    if (!src->second->withdraw(amount, outDesc,
                               Transaction::Type::TransferOut)) {
        return Result::InsufficientFunds;
    }
    const std::string inDesc =
        "Transferencia desde cuenta " + std::to_string(sourceId);
    dst->second->deposit(amount, inDesc, Transaction::Type::TransferIn);
    return Result::Success;
}

Result Bank::changeOwner(int id, const std::string& newOwner,
                         const std::string& password) {
    auto it = accounts_.find(id);
    if (it == accounts_.end()) return Result::AccountNotFound;
    if (!it->second->verifyPassword(password)) return Result::WrongPassword;
    it->second->changeOwner(newOwner);
    return Result::Success;
}

Result Bank::changePassword(int id, const std::string& newPassword,
                            const std::string& currentPassword) {
    auto it = accounts_.find(id);
    if (it == accounts_.end()) return Result::AccountNotFound;
    if (!it->second->verifyPassword(currentPassword)) {
        return Result::WrongPassword;
    }
    it->second->setPasswordHash(security::hashPassword(newPassword));
    return Result::Success;
}

const Account* Bank::find(int id) const {
    auto it = accounts_.find(id);
    return it == accounts_.end() ? nullptr : it->second.get();
}

std::vector<int> Bank::accountIds() const {
    std::vector<int> ids;
    ids.reserve(accounts_.size());
    for (const auto& [id, _] : accounts_) ids.push_back(id);
    std::ranges::sort(ids);
    return ids;
}

namespace {
// Reemplaza caracteres que romperian el formato tabulado del archivo.
std::string sanitize(std::string s) {
    for (char& c : s) {
        if (c == '\t' || c == '\n' || c == '\r') c = ' ';
    }
    return s;
}
}  // namespace

bool Bank::saveToFile(const std::string& path) const {
    std::ofstream out(path, std::ios::trunc);
    if (!out) return false;

    out << "# Next-Gen Banking System - data file v1\n";
    for (const auto& [id, acc] : accounts_) {
        out << "ACCOUNT\t" << id << '\t' << sanitize(acc->owner()) << '\t'
            << acc->passwordHash() << '\t' << acc->balance().cents() << '\t'
            << acc->history().size() << '\n';
        for (const auto& t : acc->history()) {
            const auto secs = std::chrono::duration_cast<std::chrono::seconds>(
                                  t.timestamp().time_since_epoch())
                                  .count();
            out << "TXN\t" << static_cast<int>(t.type()) << '\t'
                << t.amount().cents() << '\t' << t.balanceAfter().cents()
                << '\t' << secs << '\t' << sanitize(t.description()) << '\n';
        }
    }
    return static_cast<bool>(out);
}

bool Bank::loadFromFile(const std::string& path) {
    std::ifstream in(path);
    if (!in) return false;

    std::unordered_map<int, std::unique_ptr<Account>> loaded;
    Account* current = nullptr;
    std::string line;

    try {
        while (std::getline(in, line)) {
            if (line.empty() || line[0] == '#') continue;

            std::istringstream ss(line);
            std::string tag;
            std::getline(ss, tag, '\t');

            if (tag == "ACCOUNT") {
                std::string idStr, owner, hash, balStr, nStr;
                std::getline(ss, idStr, '\t');
                std::getline(ss, owner, '\t');
                std::getline(ss, hash, '\t');
                std::getline(ss, balStr, '\t');
                std::getline(ss, nStr, '\t');

                const int id = std::stoi(idStr);
                const std::int64_t bal = std::stoll(balStr);
                auto acc = std::make_unique<Account>(
                    Account::RestoreTag{}, owner, hash, Money::fromCents(bal));
                current = acc.get();
                loaded[id] = std::move(acc);
            } else if (tag == "TXN" && current != nullptr) {
                std::string typeStr, amtStr, balStr, secsStr, desc;
                std::getline(ss, typeStr, '\t');
                std::getline(ss, amtStr, '\t');
                std::getline(ss, balStr, '\t');
                std::getline(ss, secsStr, '\t');
                std::getline(ss, desc, '\t');

                const auto type =
                    static_cast<Transaction::Type>(std::stoi(typeStr));
                const Money amt = Money::fromCents(std::stoll(amtStr));
                const Money balAfter = Money::fromCents(std::stoll(balStr));
                const std::chrono::system_clock::time_point tp{
                    std::chrono::seconds(std::stoll(secsStr))};
                current->appendHistory(
                    Transaction(type, amt, balAfter, desc, tp));
            }
        }
    } catch (...) {
        return false;  // archivo corrupto: no alteramos el estado actual
    }

    accounts_ = std::move(loaded);
    return true;
}

}  // namespace banking
