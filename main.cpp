#include <iostream>
#include <string>
#include <unordered_map>
#include <memory>
#include <limits>

//g++ -std=c++20 main.cpp -o main

class Account {
private:
    std::string name;
    std::string password;
    double balance;

public:
    Account(const std::string& name, const std::string& password, double initialBalance)
        : name(name), password(password), balance(initialBalance) {}

    const std::string& getName() const {
        return name;
    }

    bool verifyPassword(const std::string& enteredPassword) const {
        return password == enteredPassword;
    }

    void changeName(const std::string& newName) {
        name = newName;
    }

    void changePassword(const std::string& newPassword) {
        password = newPassword;
    }

    void deposit(double amount) {
        balance += amount;
    }

    bool withdraw(double amount) {
        if (amount <= balance) {
            balance -= amount;
            return true;
        }
        return false;
    }

    double getBalance() const {
        return balance;
    }

    void printAccountStatus() const {
        std::cout << "Nombre de la cuenta: " << name << "\nSaldo: $" << balance << "\n";
    }
};

class Bank {
private:
    std::unordered_map<int, std::shared_ptr<Account>> accounts;

public:
    bool createAccount(int id, const std::string& name, const std::string& password, double initialBalance) {
        if (accounts.find(id) != accounts.end()) {
            return false; // ID already in use
        }
        accounts[id] = std::make_shared<Account>(name, password, initialBalance);
        return true;
    }

    bool transfer(int sourceId, int targetId, double amount, const std::string& password) {
        auto sourceAccount = accounts.find(sourceId);
        auto targetAccount = accounts.find(targetId);

        if (sourceAccount != accounts.end() && targetAccount != accounts.end()) {
            if (sourceAccount->second->verifyPassword(password) && sourceAccount->second->withdraw(amount)) {
                targetAccount->second->deposit(amount);
                return true;
            }
        }
        return false;
    }

    bool deposit(int accountId, double amount) {
        auto account = accounts.find(accountId);
        if (account != accounts.end()) {
            account->second->deposit(amount);
            return true;
        }
        return false;
    }

    bool withdraw(int accountId, double amount, const std::string& password) {
        auto account = accounts.find(accountId);
        if (account != accounts.end() && account->second->verifyPassword(password)) {
            return account->second->withdraw(amount);
        }
        return false;
    }

    void showAccountStatus(int accountId) {
        auto account = accounts.find(accountId);
        if (account != accounts.end()) {
            account->second->printAccountStatus();
        } else {
            std::cout << "Error: Cuenta no encontrada.\n";
        }
    }

    bool changeAccountName(int accountId, const std::string& newName, const std::string& password) {
        auto account = accounts.find(accountId);
        if (account != accounts.end() && account->second->verifyPassword(password)) {
            account->second->changeName(newName);
            return true;
        }
        return false;
    }

    bool changeAccountPassword(int accountId, const std::string& newPassword, const std::string& password) {
        auto account = accounts.find(accountId);
        if (account != accounts.end() && account->second->verifyPassword(password)) {
            account->second->changePassword(newPassword);
            return true;
        }
        return false;
    }
};

void displayMenu(Bank& bank) {
    int option = 0;
    while (option != 7) {
        std::cout << "\n             Menú             \n";
        std::cout << "1. Crear cuenta\n";
        std::cout << "2. Depositar dinero\n";
        std::cout << "3. Extraer dinero\n";
        std::cout << "4. Transferir dinero\n";
        std::cout << "5. Ver estado de cuenta\n";
        std::cout << "6. Cambiar nombre o contraseña\n";
        std::cout << "7. Salir\n";
        std::cout << "Elige una opción: ";

        if (!(std::cin >> option)) {
            std::cin.clear();
            std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::cout << "Error: Entrada inválida. Por favor, introduce un número.\n";
            continue;
        }

        int accountId, targetId;
        double amount;
        std::string name, password, confirmPassword, newPassword;

        switch (option) {
            case 1:
                std::cout << "Introduce el ID de la cuenta (número entero): ";
                std::cin >> accountId;

                std::cin.ignore();
                std::cout << "Introduce tu nombre: ";
                std::getline(std::cin, name);

                while (true) {
                    std::cout << "Introduce una contraseña: ";
                    std::cin >> password;
                    std::cout << "Confirma la contraseña: ";
                    std::cin >> confirmPassword;
                    if (password == confirmPassword) {
                        break;
                    } else {
                        std::cout << "Error: Las contraseñas no coinciden. Intenta de nuevo.\n";
                    }
                }

                std::cout << "Introduce el saldo inicial: ";
                std::cin >> amount;
                if (bank.createAccount(accountId, name, password, amount)) {
                    std::cout << "Cuenta creada con ID: " << accountId << "\n";
                } else {
                    std::cout << "Error: El ID de cuenta ya está en uso.\n";
                }
                break;
            case 2:
                std::cout << "Introduce el ID de la cuenta: ";
                std::cin >> accountId;
                std::cout << "Introduce el monto a depositar: ";
                std::cin >> amount;
                if (bank.deposit(accountId, amount)) {
                    std::cout << "Depósito exitoso\n";
                } else {
                    std::cout << "Error: No se pudo realizar el depósito. Verifica el ID de cuenta.\n";
                }
                break;
            case 3:
                std::cout << "Introduce el ID de la cuenta: ";
                std::cin >> accountId;
                std::cout << "Introduce la contraseña: ";
                std::cin >> password;
                std::cout << "Introduce el monto a extraer: ";
                std::cin >> amount;
                if (bank.withdraw(accountId, amount, password)) {
                    std::cout << "Extracción exitosa\n";
                } else {
                    std::cout << "Error: No se pudo realizar la extracción. Verifica la contraseña y el saldo.\n";
                }
                break;
            case 4:
                std::cout << "Introduce el ID de la cuenta de origen: ";
                std::cin >> accountId;
                std::cout << "Introduce la contraseña de la cuenta de origen: ";
                std::cin >> password;
                std::cout << "Introduce el ID de la cuenta de destino: ";
                std::cin >> targetId;
                std::cout << "Introduce el monto a transferir: ";
                std::cin >> amount;
                if (bank.transfer(accountId, targetId, amount, password)) {
                    std::cout << "Transferencia exitosa\n";
                } else {
                    std::cout << "Error: No se pudo realizar la transferencia. Verifica los IDs de cuenta y la contraseña.\n";
                }
                break;
            case 5:
                std::cout << "Introduce el ID de la cuenta: ";
                std::cin >> accountId;
                bank.showAccountStatus(accountId);
                break;
            case 6:
                std::cout << "Introduce el ID de la cuenta: ";
                std::cin >> accountId;
                std::cout << "Introduce la contraseña actual: ";
                std::cin >> password;
                std::cin.ignore();
                std::cout << "Elige una opción:\n";
                std::cout << "1. Cambiar nombre\n";
                std::cout << "2. Cambiar contraseña\n";
                int subOption;
                std::cin >> subOption;
                std::cin.ignore();
                if (subOption == 1) {
                    std::cout << "Introduce el nuevo nombre: ";
                    std::getline(std::cin, name);
                    if (bank.changeAccountName(accountId, name, password)) {
                        std::cout << "Nombre cambiado con éxito\n";
                    } else {
                        std::cout << "Error: No se pudo cambiar el nombre. Verifica el ID de cuenta y la contraseña.\n";
                    }
                } else if (subOption == 2) {
                    while (true) {
                        std::cout << "Introduce la nueva contraseña: ";
                        std::cin >> newPassword;
                        std::cout << "Confirma la nueva contraseña: ";
                        std::cin >> confirmPassword;
                        if (newPassword == confirmPassword) {
                            break;
                        } else {
                            std::cout << "Error: Las contraseñas no coinciden. Intenta de nuevo.\n";
                        }
                    }
                    if (bank.changeAccountPassword(accountId, newPassword, password)) {
                        std::cout << "Contraseña cambiada con éxito\n";
                    } else {
                        std::cout << "Error: No se pudo cambiar la contraseña. Verifica el ID de cuenta y la contraseña actual.\n";
                    }
                } else {
                    std::cout << "Error: Opción no válida\n";
                }
                break;
            case 7:
                std::cout << "Saliendo del sistema...\n";
                break;
            default:
                std::cout << "Error: Opción no válida. Por favor, elige una opción del 1 al 7.\n";
                break;
        }
    }
}

int main() {
    Bank bank;
    displayMenu(bank);
    return 0;
}