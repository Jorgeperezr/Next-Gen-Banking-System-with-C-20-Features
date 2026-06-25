#include <cctype>
#include <chrono>
#include <format>
#include <iostream>
#include <optional>
#include <string>

#include "banking/Account.hpp"
#include "banking/Bank.hpp"
#include "banking/Money.hpp"

// Lectura de contrasena sin eco en terminal (POSIX). En otras plataformas se
// degrada a una lectura normal.
#if defined(__unix__) || defined(__APPLE__)
#include <termios.h>
#include <unistd.h>
#endif

namespace {

using banking::Account;
using banking::Bank;
using banking::Money;
using banking::Result;

constexpr const char* kDataFile = "bank_data.txt";

// --- Utilidades de entrada (todo orientado a lineas, robusto frente a basura) ---

bool readLine(const std::string& prompt, std::string& out) {
    std::cout << prompt;
    return static_cast<bool>(std::getline(std::cin, out));
}

std::string trim(const std::string& s) {
    const auto begin = s.find_first_not_of(" \t\r\n");
    if (begin == std::string::npos) return "";
    const auto end = s.find_last_not_of(" \t\r\n");
    return s.substr(begin, end - begin + 1);
}

// Lee un entero validando que toda la linea sea un numero.
std::optional<int> readInt(const std::string& prompt) {
    std::string line;
    if (!readLine(prompt, line)) return std::nullopt;
    line = trim(line);
    if (line.empty()) return std::nullopt;
    try {
        std::size_t pos = 0;
        const int value = std::stoi(line, &pos);
        if (pos != line.size()) return std::nullopt;  // sobran caracteres
        return value;
    } catch (...) {
        return std::nullopt;
    }
}

// Lee un monto en dolares (>= 0 permitido) y lo convierte a Money.
std::optional<Money> readAmount(const std::string& prompt,
                                bool allowZero = false) {
    std::string line;
    if (!readLine(prompt, line)) return std::nullopt;
    line = trim(line);
    if (line.empty()) return std::nullopt;
    try {
        std::size_t pos = 0;
        const double value = std::stod(line, &pos);
        if (pos != line.size()) return std::nullopt;
        const Money m = Money::fromDouble(value);
        if (m.isNegative()) return std::nullopt;
        if (!allowZero && m.isZero()) return std::nullopt;
        return m;
    } catch (...) {
        return std::nullopt;
    }
}

std::string readHidden(const std::string& prompt) {
    std::cout << prompt;
    std::cout.flush();
    std::string pw;
#if defined(__unix__) || defined(__APPLE__)
    termios oldt{};
    if (tcgetattr(STDIN_FILENO, &oldt) == 0) {
        termios newt = oldt;
        newt.c_lflag &= ~static_cast<tcflag_t>(ECHO);
        tcsetattr(STDIN_FILENO, TCSANOW, &newt);
        std::getline(std::cin, pw);
        tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
        std::cout << "\n";
        return pw;
    }
#endif
    std::getline(std::cin, pw);
    return pw;
}

void formatTimestamp(std::ostream& os,
                     std::chrono::system_clock::time_point tp) {
    const auto secs = std::chrono::floor<std::chrono::seconds>(tp);
    os << std::format("{:%Y-%m-%d %H:%M:%S}", secs);  // hora UTC
}

void printHistory(const Account& acc) {
    if (acc.history().empty()) {
        std::cout << "(sin movimientos)\n";
        return;
    }
    std::cout << std::format("{:<19} | {:<16} | {:>14} | {:>14}\n", "Fecha (UTC)",
                             "Tipo", "Monto", "Saldo despues");
    std::cout << std::string(73, '-') << "\n";
    for (const auto& t : acc.history()) {
        std::string when;
        {
            std::ostringstream ss;
            formatTimestamp(ss, t.timestamp());
            when = ss.str();
        }
        std::cout << std::format("{:<19} | {:<16} | {:>14} | {:>14}\n", when,
                                 t.typeLabel(), t.amount().toString(),
                                 t.balanceAfter().toString());
    }
}

// --- Manejadores de cada opcion del menu ---

void doCreate(Bank& bank) {
    auto id = readInt("Introduce el ID de la cuenta (entero): ");
    if (!id) {
        std::cout << "Error: ID invalido.\n";
        return;
    }
    std::string owner;
    readLine("Introduce el nombre del titular: ", owner);
    owner = trim(owner);
    if (owner.empty()) {
        std::cout << "Error: el nombre no puede estar vacio.\n";
        return;
    }

    std::string password;
    while (true) {
        password = readHidden("Introduce una contrasena: ");
        if (password.empty()) {
            std::cout << "Error: la contrasena no puede estar vacia.\n";
            continue;
        }
        const std::string confirm = readHidden("Confirma la contrasena: ");
        if (password == confirm) break;
        std::cout << "Error: las contrasenas no coinciden. Intenta de nuevo.\n";
    }

    auto initial = readAmount("Introduce el saldo inicial (0 o mas): ",
                              /*allowZero=*/true);
    const Money initialBalance = initial.value_or(Money::fromCents(0));

    const Result r =
        bank.createAccount(*id, owner, password, initialBalance);
    std::cout << banking::toMessage(r) << "\n";
    if (r == Result::Success) {
        std::cout << "Cuenta creada con ID " << *id << ".\n";
    }
}

void doDeposit(Bank& bank) {
    auto id = readInt("Introduce el ID de la cuenta: ");
    if (!id) {
        std::cout << "Error: ID invalido.\n";
        return;
    }
    auto amount = readAmount("Introduce el monto a depositar: ");
    if (!amount) {
        std::cout << "Error: monto invalido (debe ser un numero positivo).\n";
        return;
    }
    std::cout << banking::toMessage(bank.deposit(*id, *amount)) << "\n";
}

void doWithdraw(Bank& bank) {
    auto id = readInt("Introduce el ID de la cuenta: ");
    if (!id) {
        std::cout << "Error: ID invalido.\n";
        return;
    }
    const std::string pw = readHidden("Introduce la contrasena: ");
    auto amount = readAmount("Introduce el monto a extraer: ");
    if (!amount) {
        std::cout << "Error: monto invalido (debe ser un numero positivo).\n";
        return;
    }
    std::cout << banking::toMessage(bank.withdraw(*id, *amount, pw)) << "\n";
}

void doTransfer(Bank& bank) {
    auto source = readInt("Introduce el ID de la cuenta de origen: ");
    if (!source) {
        std::cout << "Error: ID invalido.\n";
        return;
    }
    const std::string pw =
        readHidden("Introduce la contrasena de la cuenta de origen: ");
    auto target = readInt("Introduce el ID de la cuenta de destino: ");
    if (!target) {
        std::cout << "Error: ID invalido.\n";
        return;
    }
    auto amount = readAmount("Introduce el monto a transferir: ");
    if (!amount) {
        std::cout << "Error: monto invalido (debe ser un numero positivo).\n";
        return;
    }
    std::cout << banking::toMessage(
                     bank.transfer(*source, *target, *amount, pw))
              << "\n";
}

void doShowStatus(Bank& bank) {
    auto id = readInt("Introduce el ID de la cuenta: ");
    if (!id) {
        std::cout << "Error: ID invalido.\n";
        return;
    }
    const Account* acc = bank.find(*id);
    if (acc == nullptr) {
        std::cout << banking::toMessage(Result::AccountNotFound) << "\n";
        return;
    }
    // Mejora de seguridad: ver el estado exige la contrasena.
    const std::string pw = readHidden("Introduce la contrasena: ");
    if (!acc->verifyPassword(pw)) {
        std::cout << banking::toMessage(Result::WrongPassword) << "\n";
        return;
    }

    std::cout << "\n----- Estado de cuenta -----\n";
    std::cout << "ID:      " << *id << "\n";
    std::cout << "Titular: " << acc->owner() << "\n";
    std::cout << "Saldo:   " << acc->balance().toString() << "\n";
    std::cout << "Movimientos: " << acc->history().size() << "\n\n";
    printHistory(*acc);
}

void doChange(Bank& bank) {
    auto id = readInt("Introduce el ID de la cuenta: ");
    if (!id) {
        std::cout << "Error: ID invalido.\n";
        return;
    }
    const std::string current = readHidden("Introduce la contrasena actual: ");

    std::cout << "1. Cambiar nombre\n2. Cambiar contrasena\n";
    auto sub = readInt("Elige una opcion (1-2): ");
    if (!sub) {
        std::cout << "Error: opcion invalida.\n";
        return;
    }

    if (*sub == 1) {
        std::string newName;
        readLine("Introduce el nuevo nombre: ", newName);
        newName = trim(newName);
        if (newName.empty()) {
            std::cout << "Error: el nombre no puede estar vacio.\n";
            return;
        }
        std::cout << banking::toMessage(
                         bank.changeOwner(*id, newName, current))
                  << "\n";
    } else if (*sub == 2) {
        std::string newPassword;
        while (true) {
            newPassword = readHidden("Introduce la nueva contrasena: ");
            if (newPassword.empty()) {
                std::cout << "Error: la contrasena no puede estar vacia.\n";
                continue;
            }
            const std::string confirm =
                readHidden("Confirma la nueva contrasena: ");
            if (newPassword == confirm) break;
            std::cout << "Error: las contrasenas no coinciden.\n";
        }
        std::cout << banking::toMessage(
                         bank.changePassword(*id, newPassword, current))
                  << "\n";
    } else {
        std::cout << "Error: opcion no valida.\n";
    }
}

void runMenu(Bank& bank) {
    while (true) {
        std::cout << "\n========== Banco Next-Gen ==========\n"
                  << "1. Crear cuenta\n"
                  << "2. Depositar dinero\n"
                  << "3. Extraer dinero\n"
                  << "4. Transferir dinero\n"
                  << "5. Ver estado de cuenta\n"
                  << "6. Cambiar nombre o contrasena\n"
                  << "7. Salir\n"
                  << "Elige una opcion (1-7): ";

        std::string optLine;
        if (!std::getline(std::cin, optLine)) {  // EOF (Ctrl+D)
            bank.saveToFile(kDataFile);
            std::cout << "\nEntrada finalizada. Datos guardados. Saliendo...\n";
            return;
        }
        optLine = trim(optLine);

        int option = 0;
        try {
            std::size_t pos = 0;
            option = std::stoi(optLine, &pos);
            if (pos != optLine.size()) throw std::invalid_argument("extra");
        } catch (...) {
            std::cout << "Error: entrada invalida. Introduce un numero (1-7).\n";
            continue;
        }

        switch (option) {
            case 1: doCreate(bank); break;
            case 2: doDeposit(bank); break;
            case 3: doWithdraw(bank); break;
            case 4: doTransfer(bank); break;
            case 5: doShowStatus(bank); break;
            case 6: doChange(bank); break;
            case 7:
                bank.saveToFile(kDataFile);
                std::cout << "Datos guardados. Saliendo del sistema...\n";
                return;
            default:
                std::cout << "Error: opcion no valida. Elige del 1 al 7.\n";
                break;
        }

        // Auto-guardado tras cada vuelta del menu para no perder datos.
        bank.saveToFile(kDataFile);
    }
}

}  // namespace

int main() {
    Bank bank;
    if (bank.loadFromFile(kDataFile)) {
        std::cout << "Datos cargados desde '" << kDataFile << "' ("
                  << bank.size() << " cuenta(s)).\n";
    } else {
        std::cout << "No habia datos previos. Empezando con un banco vacio.\n";
    }

    runMenu(bank);
    return 0;
}
