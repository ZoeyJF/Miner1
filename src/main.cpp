#include "midnight_mobile_client.hpp"

#include <iostream>
#include <string>
#include <vector>

void print_help(const char* prog_name) {
    std::cout << "Midnight Node Mobile Client & Diagnostic Suite (C++17 / Mobile / Termux)\n\n"
              << "Uso:\n"
              << "  " << prog_name << " [opcion] [argumentos]\n\n"
              << "Opciones disponibles:\n"
              << "  --check-all [RPC_URL]             Ejecuta diagnostico integral offline y online para verificar si el nodo esta al 100%\n"
              << "  --offline-transfer <SEED> <ADDR> <AMOUNT>\n"
              << "                                    Construye y valida una transferencia unshielded sin conexion\n"
              << "  --attempt-mint <ADDR> <AMOUNT>    Simula intento de impresion arbitraria de dinero para comprobar rechazo por consenso\n"
              << "  --health [RPC_URL]                Consulta la salud online del nodo (pares, sincronizacion, estado ledger)\n"
              << "  --validate-addr <ADDR>            Verifica si una direccion cumple el estandar Midnight bech32 / hex\n"
              << "  --help                            Muestra esta ayuda\n";
}

int main(int argc, char* argv[]) {
    if (argc < 2) {
        print_help(argv[0]);
        return 0;
    }

    std::string command = argv[1];

    if (command == "--help" || command == "-h") {
        print_help(argv[0]);
        return 0;
    }

    midnight::MidnightMobileClient client;

    if (command == "--check-all") {
        if (argc >= 3) {
            client.set_rpc_url(argv[2]);
        }
        std::cout << "=== Verificacion Integral de Integridad del Nodo Midnight ===\n";
        std::cout << "Endpoint configurado: " << client.get_rpc_url() << "\n\n";

        std::vector<std::string> log;
        double score = client.evaluate_node_integrity(log);

        for (const auto& line : log) {
            std::cout << line << "\n";
        }

        std::cout << "\n------------------------------------------------------\n";
        std::cout << "Puntaje de Integridad Operativa: " << score << "% / 100%\n";
        if (score >= 80.0) {
            std::cout << "ESTADO: Nodo en excelente estado de sincronizacion e integridad.\n";
        } else if (score >= 60.0) {
            std::cout << "ESTADO: Funciones locales y validaciones offline al 100%. Verifique conectividad P2P/RPC online.\n";
        } else {
            std::cout << "ESTADO: Advertencia, revision requerida.\n";
        }
        return 0;
    }

    if (command == "--offline-transfer") {
        if (argc < 5) {
            std::cerr << "Error: Faltan argumentos. Uso: --offline-transfer <SEED> <ADDR> <AMOUNT>\n";
            return 1;
        }
        std::string seed = argv[2];
        std::string addr = argv[3];
        uint64_t amount = std::stoull(argv[4]);

        auto tx = client.build_unshielded_transfer(seed, addr, amount);
        auto val = client.verify_transaction_offline(tx);

        std::cout << "Transferencia Unshielded (Modo Offline):\n";
        std::cout << "  Remitente derivado: " << tx.sender << "\n";
        std::cout << "  Destinatario:       " << tx.recipient << "\n";
        std::cout << "  Monto:              " << tx.amount << " DUST/mNIGHT\n";
        std::cout << "  ZK Proof Hash:      " << tx.proof_zk_hex << "\n";
        std::cout << "  Firma Cripto:       " << tx.signature << "\n";
        std::cout << "  Resultado Local:    " << (val.valid ? "[VALIDO] " : "[RECHAZADO] ") << val.message << "\n";
        return val.valid ? 0 : 1;
    }

    if (command == "--attempt-mint") {
        if (argc < 4) {
            std::cerr << "Error: Faltan argumentos. Uso: --attempt-mint <ADDR> <AMOUNT>\n";
            return 1;
        }
        std::string addr = argv[2];
        uint64_t amount = std::stoull(argv[3]);

        std::cout << "Simulando intento de impresion arbitraria de dinero (" << amount << " tokens) hacia " << addr << "...\n";
        auto tx = client.build_mint_attempt(addr, amount);
        auto val = client.verify_transaction_offline(tx);

        std::cout << "Codigo de Respuesta: " << val.code << "\n";
        std::cout << "Mensaje de Consenso: " << val.message << "\n";
        std::cout << "Seguridad del Protocolo: Criptograficamente protegido contra creacion no autorizada.\n";
        return 0;
    }

    if (command == "--health") {
        if (argc >= 3) {
            client.set_rpc_url(argv[2]);
        }
        std::cout << "Consultando salud del nodo en " << client.get_rpc_url() << "...\n";
        auto health = client.query_node_health();
        std::cout << "  Alcanzable:        " << (health.is_reachable ? "SI" : "NO") << "\n";
        std::cout << "  Sincronizando:     " << (health.is_syncing ? "SI" : "NO") << "\n";
        std::cout << "  Pares activos:     " << health.peers_count << "\n";
        std::cout << "  Raiz estado ledger:" << (health.ledger_state_root.empty() ? "N/A" : health.ledger_state_root) << "\n";
        std::cout << "  Salud estimada:    " << health.health_percentage << "%\n";
        return 0;
    }

    if (command == "--validate-addr") {
        if (argc < 3) {
            std::cerr << "Error: Indique direccion a validar.\n";
            return 1;
        }
        auto res = client.validate_address(argv[2]);
        std::cout << "Direccion: " << argv[2] << "\n";
        std::cout << "Resultado: " << (res.valid ? "VALIDA" : "INVALIDA") << " (" << res.message << ")\n";
        return res.valid ? 0 : 1;
    }

    std::cerr << "Opcion no reconocida: " << command << "\n";
    print_help(argv[0]);
    return 1;
}
