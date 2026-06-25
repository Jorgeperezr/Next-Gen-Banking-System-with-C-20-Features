# Next-Gen Banking System (C++20)

A command-line banking system written in modern C++20. It supports creating
accounts, deposits, withdrawals, transfers between accounts, and viewing an
account's status with its full transaction history. Passwords are hashed and
account data persists between runs.

## Features

- Amounts stored as integer cents to avoid floating-point rounding errors
- Salted password hashing (passwords are never stored in plain text)
- Per-account transaction history with timestamps
- Automatic persistence to disk
- Strict input validation
- Unit tests and continuous integration

## Build and run

Requires a C++20 compiler with `std::format` support (GCC 13+, Clang 17+, or a
recent MSVC).

```bash
cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j
./build/banking
```

Run the tests:

```bash
ctest --test-dir build --output-on-failure
```

## Project structure

```
include/banking/   Public headers: Money, Account, Bank, Transaction, Security
src/               Implementations and the command-line interface (main.cpp)
tests/             Unit tests
CMakeLists.txt     Build configuration
```

The business logic lives in a separate `banking_core` library so it can be
tested in isolation and reused.

## Security note

The password hashing in this project is for demonstration purposes. A
production system should use a dedicated password-hashing function such as
Argon2id, bcrypt, or scrypt.

## License

See the [LICENSE](LICENSE) file.
