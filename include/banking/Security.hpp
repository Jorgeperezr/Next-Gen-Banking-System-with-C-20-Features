#pragma once

#include <string>

namespace banking::security {

// Genera un hash con sal (salt) única para una contraseña.
// Formato devuelto: "<salt_hex>$<hash_hex>".
//
//  ADVERTENCIA (importante):
//  Esta implementacion usa FNV-1a con key-stretching. Sirve para DEMOSTRAR los
//  conceptos correctos: (1) nunca almacenar la contrasena en texto plano,
//  (2) usar una sal unica por usuario para frustrar tablas rainbow, y
//  (3) aplicar estiramiento de clave para encarecer la fuerza bruta.
//
//  NO es apta para produccion. Un sistema real DEBE usar una funcion disenada
//  especificamente para contrasenas, como Argon2id, bcrypt o scrypt
//  (por ejemplo a traves de libsodium). FNV no es una funcion criptografica.
std::string hashPassword(const std::string& password);

// Verifica una contraseña contra un hash almacenado en el formato anterior.
bool verifyPassword(const std::string& password, const std::string& stored);

}  // namespace banking::security
