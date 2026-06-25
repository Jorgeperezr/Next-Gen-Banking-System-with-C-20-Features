// Tests sin dependencias externas: un mini-framework con un contador de fallos.
// Se ejecuta con `ctest` o directamente. Devuelve codigo != 0 si algo falla.

#include <cstdio>
#include <cstdlib>
#include <iostream>
#include <string>

#include "banking/Account.hpp"
#include "banking/Bank.hpp"
#include "banking/Money.hpp"
#include "banking/Security.hpp"
#include "banking/Transaction.hpp"

namespace {

int g_checks = 0;
int g_failures = 0;

#define CHECK(cond)                                                       \
    do {                                                                  \
        ++g_checks;                                                       \
        if (!(cond)) {                                                    \
            ++g_failures;                                                 \
            std::cerr << "  FAIL [" << __LINE__ << "]: " << #cond << "\n"; \
        }                                                                 \
    } while (0)

using namespace banking;

void testMoney() {
    std::cout << "Money...\n";
    // El clasico problema del punto flotante NO ocurre con centimos.
    const Money a = Money::fromDouble(0.1);
    const Money b = Money::fromDouble(0.2);
    CHECK((a + b) == Money::fromDouble(0.3));
    CHECK((a + b).cents() == 30);

    CHECK(Money::fromCents(150).toString() == "$1.50");
    CHECK(Money::fromCents(123456789).toString() == "$1,234,567.89");
    CHECK(Money::fromCents(-150).toString() == "-$1.50");
    CHECK(Money::fromCents(0).toString() == "$0.00");

    // Redondeo al centimo mas cercano.
    CHECK(Money::fromDouble(19.999).cents() == 2000);
    CHECK(Money::fromDouble(19.991).cents() == 1999);

    CHECK(Money::fromCents(100) < Money::fromCents(200));
    CHECK(Money::fromCents(200) > Money::fromCents(100));
}

void testAccountValidation() {
    std::cout << "Account (validacion)...\n";
    Account acc("Ana", security::hashPassword("clave"), Money::fromCents(1000));
    CHECK(acc.balance() == Money::fromCents(1000));

    // Deposito positivo OK.
    acc.deposit(Money::fromCents(500));
    CHECK(acc.balance() == Money::fromCents(1500));

    // Deposito no positivo => excepcion (la vulnerabilidad del codigo original).
    bool threw = false;
    try {
        acc.deposit(Money::fromCents(-100));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);

    // Retiro positivo dentro del saldo OK.
    CHECK(acc.withdraw(Money::fromCents(500)));
    CHECK(acc.balance() == Money::fromCents(1000));

    // Fondos insuficientes => false, sin cambiar el saldo.
    CHECK(!acc.withdraw(Money::fromCents(999999)));
    CHECK(acc.balance() == Money::fromCents(1000));

    // Retiro negativo => excepcion (no debe "regalar" dinero).
    threw = false;
    try {
        acc.withdraw(Money::fromCents(-100));
    } catch (const std::invalid_argument&) {
        threw = true;
    }
    CHECK(threw);
}

void testSecurity() {
    std::cout << "Security...\n";
    const std::string h1 = security::hashPassword("secreto");
    const std::string h2 = security::hashPassword("secreto");

    // Nunca se guarda el texto plano.
    CHECK(h1.find("secreto") == std::string::npos);
    // Misma contrasena, distinta sal => distinto hash (frustra rainbow tables).
    CHECK(h1 != h2);
    // Verificacion correcta e incorrecta.
    CHECK(security::verifyPassword("secreto", h1));
    CHECK(!security::verifyPassword("incorrecta", h1));
    CHECK(!security::verifyPassword("secreto", "formato_invalido"));
}

void testBankTransfers() {
    std::cout << "Bank (transferencias y errores)...\n";
    Bank bank;
    CHECK(bank.createAccount(1, "Ana", "claveA", Money::fromCents(10000)) ==
          Result::Success);
    CHECK(bank.createAccount(2, "Beto", "claveB", Money::fromCents(0)) ==
          Result::Success);

    // ID duplicado.
    CHECK(bank.createAccount(1, "Otro", "x", Money::fromCents(0)) ==
          Result::DuplicateId);

    // Transferencia correcta.
    CHECK(bank.transfer(1, 2, Money::fromCents(3000), "claveA") ==
          Result::Success);
    CHECK(bank.find(1)->balance() == Money::fromCents(7000));
    CHECK(bank.find(2)->balance() == Money::fromCents(3000));

    // Contrasena incorrecta: nada cambia.
    CHECK(bank.transfer(1, 2, Money::fromCents(1000), "mala") ==
          Result::WrongPassword);
    CHECK(bank.find(1)->balance() == Money::fromCents(7000));

    // Misma cuenta.
    CHECK(bank.transfer(1, 1, Money::fromCents(100), "claveA") ==
          Result::SameAccount);

    // Destino inexistente.
    CHECK(bank.transfer(1, 99, Money::fromCents(100), "claveA") ==
          Result::TargetNotFound);

    // Fondos insuficientes.
    CHECK(bank.transfer(1, 2, Money::fromCents(999999), "claveA") ==
          Result::InsufficientFunds);

    // Monto negativo (vulnerabilidad del codigo original) rechazado.
    CHECK(bank.transfer(1, 2, Money::fromCents(-500), "claveA") ==
          Result::InvalidAmount);
    CHECK(bank.find(1)->balance() == Money::fromCents(7000));
}

void testBankPasswordChange() {
    std::cout << "Bank (cambio de credenciales)...\n";
    Bank bank;
    bank.createAccount(1, "Ana", "vieja", Money::fromCents(100));

    CHECK(bank.changePassword(1, "nueva", "incorrecta") ==
          Result::WrongPassword);
    CHECK(bank.changePassword(1, "nueva", "vieja") == Result::Success);
    // La vieja ya no sirve; la nueva si.
    CHECK(bank.withdraw(1, Money::fromCents(50), "vieja") ==
          Result::WrongPassword);
    CHECK(bank.withdraw(1, Money::fromCents(50), "nueva") == Result::Success);

    CHECK(bank.changeOwner(1, "Ana Lopez", "nueva") == Result::Success);
    CHECK(bank.find(1)->owner() == "Ana Lopez");
}

void testPersistence() {
    std::cout << "Persistencia (round-trip)...\n";
    const std::string path = "test_bank_data.tmp";
    std::remove(path.c_str());

    {
        Bank bank;
        bank.createAccount(7, "Ana", "clave", Money::fromCents(5000));
        bank.createAccount(8, "Beto", "clave2", Money::fromCents(2000));
        bank.transfer(7, 8, Money::fromCents(1500), "clave");
        CHECK(bank.saveToFile(path));
    }

    {
        Bank loaded;
        CHECK(loaded.loadFromFile(path));
        CHECK(loaded.size() == 2);
        CHECK(loaded.find(7) != nullptr);
        CHECK(loaded.find(7)->balance() == Money::fromCents(3500));
        CHECK(loaded.find(8)->balance() == Money::fromCents(3500));
        CHECK(loaded.find(7)->owner() == "Ana");
        // El hash se preserva: la contrasena sigue verificandose tras recargar.
        CHECK(loaded.withdraw(7, Money::fromCents(500), "clave") ==
              Result::Success);
        // El historial se preserva (saldo inicial + transferencia + retiro).
        CHECK(loaded.find(7)->history().size() >= 3);
    }

    std::remove(path.c_str());
}

}  // namespace

int main() {
    std::cout << "=== Ejecutando tests ===\n";
    testMoney();
    testAccountValidation();
    testSecurity();
    testBankTransfers();
    testBankPasswordChange();
    testPersistence();

    std::cout << "\n=== Resultado: " << (g_checks - g_failures) << "/"
              << g_checks << " comprobaciones pasaron ===\n";
    if (g_failures > 0) {
        std::cout << g_failures << " FALLARON\n";
        return 1;
    }
    std::cout << "TODOS LOS TESTS PASARON\n";
    return 0;
}
