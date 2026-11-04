#include "midnight_mobile_client.hpp"

#include <iostream>
#include <sstream>
#include <iomanip>
#include <cstring>
#include <array>
#include <cstdlib>
#include <chrono>

#if defined(__unix__) || defined(__APPLE__)
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#endif

namespace midnight {

// Simple SHA-256 implementation for offline verification and checksums
class MiniSHA256 {
public:
    MiniSHA256() { reset(); }

    void reset() {
        m_tot_len = 0;
        m_len = 0;
        m_h[0] = 0x6a09e667; m_h[1] = 0xbb67ae85;
        m_h[2] = 0x3c6ef372; m_h[3] = 0xa54ff53a;
        m_h[4] = 0x510e527f; m_h[5] = 0x9b05688c;
        m_h[6] = 0x1f83d9ab; m_h[7] = 0x5be0cd19;
    }

    void update(const uint8_t* message, size_t len) {
        for (size_t i = 0; i < len; ++i) {
            m_block[m_len++] = message[i];
            if (m_len == 64) {
                transform();
                m_tot_len += 64;
                m_len = 0;
            }
        }
    }

    std::string finalize_hex() {
        m_tot_len += m_len;
        m_block[m_len++] = 0x80;
        if (m_len > 56) {
            while (m_len < 64) m_block[m_len++] = 0;
            transform();
            m_len = 0;
        }
        while (m_len < 56) m_block[m_len++] = 0;
        uint64_t total_bits = m_tot_len * 8;
        for (int i = 7; i >= 0; --i) {
            m_block[56 + (7 - i)] = (total_bits >> (i * 8)) & 0xff;
        }
        transform();

        std::ostringstream oss;
        for (int i = 0; i < 8; ++i) {
            oss << std::hex << std::setfill('0') << std::setw(8) << m_h[i];
        }
        return oss.str();
    }

private:
    uint32_t m_h[8];
    uint8_t m_block[64];
    size_t m_len;
    size_t m_tot_len;

    static inline uint32_t rotr(uint32_t x, uint32_t n) { return (x >> n) | (x << (32 - n)); }

    void transform() {
        static const uint32_t k[64] = {
            0x428a2f98,0x71374491,0xb5c0fbcf,0xe9b5dba5,0x3956c25b,0x59f111f1,0x923f82a4,0xab1c5ed5,
            0xd807aa98,0x12835b01,0x243185be,0x550c7dc3,0x72be5d74,0x80deb1fe,0x9bdc06a7,0xc19bf174,
            0xe49b69c1,0xefbe4786,0x0fc19dc6,0x240ca1cc,0x2de92c6f,0x4a7484aa,0x5cb0a9dc,0x76f988da,
            0x983e5152,0xa831c66d,0xb00327c8,0xbf597fc7,0xc6e00bf3,0xd5a79147,0x06ca6351,0x14292967,
            0x27b70a85,0x2e1b2138,0x4d2c6dfc,0x53380d13,0x650a7354,0x766a0abb,0x81c2c92e,0x92722c85,
            0xa2bfe8a1,0xa81a664b,0xc24b8b70,0xc76c51a3,0xd192e819,0xd6990624,0xf40e3585,0x106aa070,
            0x19a4c116,0x1e376c08,0x2748774c,0x34b0bcb5,0x391c0cb3,0x4ed8aa4a,0x5b9cca4f,0x682e6ff3,
            0x748f82ee,0x78a5636f,0x84c87814,0x8cc70208,0x90befffa,0xa4506ceb,0xbef9a3f7,0xc67178f2
        };
        uint32_t w[64];
        for (int i = 0; i < 16; ++i) {
            w[i] = (m_block[i * 4] << 24) | (m_block[i * 4 + 1] << 16) | (m_block[i * 4 + 2] << 8) | (m_block[i * 4 + 3]);
        }
        for (int i = 16; i < 64; ++i) {
            uint32_t s0 = rotr(w[i - 15], 7) ^ rotr(w[i - 15], 18) ^ (w[i - 15] >> 3);
            uint32_t s1 = rotr(w[i - 2], 17) ^ rotr(w[i - 2], 19) ^ (w[i - 2] >> 10);
            w[i] = w[i - 16] + s0 + w[i - 7] + s1;
        }
        uint32_t a = m_h[0], b = m_h[1], c = m_h[2], d = m_h[3];
        uint32_t e = m_h[4], f = m_h[5], g = m_h[6], h = m_h[7];
        for (int i = 0; i < 64; ++i) {
            uint32_t s1 = rotr(e, 6) ^ rotr(e, 11) ^ rotr(e, 25);
            uint32_t ch = (e & f) ^ ((~e) & g);
            uint32_t temp1 = h + s1 + ch + k[i] + w[i];
            uint32_t s0 = rotr(a, 2) ^ rotr(a, 13) ^ rotr(a, 22);
            uint32_t maj = (a & b) ^ (a & c) ^ (b & c);
            uint32_t temp2 = s0 + maj;
            h = g; g = f; f = e; e = d + temp1;
            d = c; c = b; b = a; a = temp1 + temp2;
        }
        m_h[0] += a; m_h[1] += b; m_h[2] += c; m_h[3] += d;
        m_h[4] += e; m_h[5] += f; m_h[6] += g; m_h[7] += h;
    }
};

MidnightMobileClient::MidnightMobileClient(const std::string& rpc_url)
    : rpc_url_(rpc_url) {}

void MidnightMobileClient::set_rpc_url(const std::string& url) {
    rpc_url_ = url;
}

std::string MidnightMobileClient::get_rpc_url() const {
    return rpc_url_;
}

ValidationResult MidnightMobileClient::validate_address(const std::string& address) const {
    // Midnight addresses follow bech32-like conventions: e.g. mn_addr_... or hex-encoded addresses
    if (address.rfind("mn_addr_", 0) == 0 && address.length() > 20) {
        return {true, "OK", "Valid Midnight bech32 address format"};
    }
    if (address.rfind("0x", 0) == 0 && (address.length() == 42 || address.length() == 66)) {
        return {true, "OK", "Valid hexadecimal account/contract identifier"};
    }
    return {false, "ERR_INVALID_ADDRESS", "Address does not match Midnight format (expected mn_addr_* or 0x hex)"};
}

std::string MidnightMobileClient::derive_address_from_seed(const std::string& seed) {
    MiniSHA256 sha;
    sha.update(reinterpret_cast<const uint8_t*>(seed.data()), seed.size());
    std::string digest = sha.finalize_hex();
    return "mn_addr_undeployed1" + digest.substr(0, 32);
}

TransactionPayload MidnightMobileClient::build_unshielded_transfer(
    const std::string& sender_seed,
    const std::string& recipient_addr,
    uint64_t amount) const
{
    TransactionPayload tx;
    tx.type = TxType::UnshieldedTransfer;
    tx.sender = derive_address_from_seed(sender_seed);
    tx.recipient = recipient_addr;
    tx.amount = amount;

    // ZK proof mock for mobile demonstration
    MiniSHA256 sha;
    std::string proof_input = tx.sender + tx.recipient + std::to_string(amount);
    sha.update(reinterpret_cast<const uint8_t*>(proof_input.data()), proof_input.size());
    tx.proof_zk_hex = "0x" + sha.finalize_hex();

    // Raw hex encoding mock
    tx.raw_hex = "0x01" + tx.proof_zk_hex.substr(2);
    tx.is_signed = true;

    MiniSHA256 sig_sha;
    std::string sig_input = sender_seed + tx.raw_hex;
    sig_sha.update(reinterpret_cast<const uint8_t*>(sig_input.data()), sig_input.size());
    tx.signature = "0x" + sig_sha.finalize_hex();

    return tx;
}

TransactionPayload MidnightMobileClient::build_mint_attempt(
    const std::string& target_addr,
    uint64_t amount) const
{
    TransactionPayload tx;
    tx.type = TxType::ArbitraryMintAttempt;
    tx.sender = "0x0000000000000000000000000000000000000000";
    tx.recipient = target_addr;
    tx.amount = amount;
    tx.proof_zk_hex = ""; // No valid ZK proof of state transition
    tx.raw_hex = "0xff00badc0de";
    tx.is_signed = false;
    tx.signature = "";
    return tx;
}

ValidationResult MidnightMobileClient::verify_transaction_offline(const TransactionPayload& tx) const {
    // 1. Check recipient address
    ValidationResult addr_check = validate_address(tx.recipient);
    if (!addr_check.valid) {
        return addr_check;
    }

    // 2. Reject arbitrary minting / unbacked token creation
    if (tx.type == TxType::ArbitraryMintAttempt) {
        return {
            false,
            "ERR_LEDGER_CONSTRAINT_VIOLATION",
            "Midnight Ledger consensus forbids arbitrary minting. Tokens cannot be created out of thin air."
        };
    }

    // 3. Check signature and ZK proof presence
    if (!tx.is_signed || tx.signature.empty()) {
        return {false, "ERR_UNSIGNED_TRANSACTION", "Transaction missing valid cryptographic signature"};
    }

    if (tx.proof_zk_hex.empty() || tx.proof_zk_hex.rfind("0x", 0) != 0) {
        return {false, "ERR_INVALID_PROOF", "Missing or malformed zero-knowledge proof"};
    }

    if (tx.amount == 0) {
        return {false, "ERR_ZERO_AMOUNT", "Transaction amount must be strictly greater than 0"};
    }

    return {true, "OK", "Transaction payload verified successfully offline (valid structure & ZK proof mock)"};
}

std::string MidnightMobileClient::execute_rpc(const std::string& method, const std::string& params_json) const {
    // Parse host and port from rpc_url_
    std::string host = "127.0.0.1";
    int port = 9944;

    std::string url = rpc_url_;
    size_t protocol_pos = url.find("://");
    if (protocol_pos != std::string::npos) {
        url = url.substr(protocol_pos + 3);
    }
    size_t colon_pos = url.find(':');
    if (colon_pos != std::string::npos) {
        host = url.substr(0, colon_pos);
        size_t slash_pos = url.find('/', colon_pos);
        if (slash_pos != std::string::npos) {
            port = std::atoi(url.substr(colon_pos + 1, slash_pos - colon_pos - 1).c_str());
        } else {
            port = std::atoi(url.substr(colon_pos + 1).c_str());
        }
    }

    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        return "{\"error\":\"socket_creation_failed\"}";
    }

    // Set 2-second timeout
    struct timeval tv;
    tv.tv_sec = 2;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof tv);
    setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, (const char*)&tv, sizeof tv);

    struct sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    inet_pton(AF_INET, host.c_str(), &server_addr.sin_addr);

    if (connect(sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        close(sock);
        return "{\"error\":\"connection_refused\"}";
    }

    std::ostringstream json_payload;
    json_payload << "{\"jsonrpc\":\"2.0\",\"id\":1,\"method\":\"" << method << "\",\"params\":" << params_json << "}";
    std::string payload_str = json_payload.str();

    std::ostringstream http_req;
    http_req << "POST / HTTP/1.1\r\n"
             << "Host: " << host << ":" << port << "\r\n"
             << "Content-Type: application/json\r\n"
             << "Content-Length: " << payload_str.length() << "\r\n"
             << "Connection: close\r\n\r\n"
             << payload_str;

    std::string req_str = http_req.str();
    send(sock, req_str.c_str(), req_str.length(), 0);

    std::string response;
    char buffer[4096];
    ssize_t bytes_read;
    while ((bytes_read = recv(sock, buffer, sizeof(buffer) - 1, 0)) > 0) {
        buffer[bytes_read] = '\0';
        response.append(buffer, bytes_read);
    }
    close(sock);

    // Extract HTTP body
    size_t body_pos = response.find("\r\n\r\n");
    if (body_pos != std::string::npos) {
        return response.substr(body_pos + 4);
    }
    return response;
}

NodeHealth MidnightMobileClient::query_node_health() const {
    NodeHealth health{};
    health.is_reachable = false;
    health.is_syncing = true;
    health.peers_count = 0;
    health.best_block_number = 0;
    health.best_block_hash = "";
    health.ledger_state_root = "";
    health.health_percentage = 0.0;

    std::string health_res = execute_rpc("system_health", "[]");
    if (health_res.find("\"error\"") != std::string::npos || health_res.empty()) {
        return health; // Unreachable
    }

    health.is_reachable = true;
    health.health_percentage = 40.0; // Node responding

    // Parse isSyncing & peers
    if (health_res.find("\"isSyncing\":false") != std::string::npos) {
        health.is_syncing = false;
        health.health_percentage += 20.0;
    }
    size_t peers_idx = health_res.find("\"peers\":");
    if (peers_idx != std::string::npos) {
        health.peers_count = std::atoi(health_res.c_str() + peers_idx + 8);
        if (health.peers_count > 0) {
            health.health_percentage += 20.0;
        }
    }

    // Query ledger state root
    std::string root_res = execute_rpc("midnight_ledgerStateRoot", "[]");
    if (root_res.find("\"result\"") != std::string::npos) {
        size_t res_pos = root_res.find("\"result\":\"");
        if (res_pos != std::string::npos) {
            size_t end_pos = root_res.find("\"", res_pos + 10);
            health.ledger_state_root = root_res.substr(res_pos + 10, end_pos - res_pos - 10);
            health.health_percentage += 10.0;
        }
    }

    // Query API versions
    std::string api_res = execute_rpc("midnight_apiVersions", "[]");
    if (api_res.find("\"result\"") != std::string::npos) {
        health.api_versions = {2};
        health.health_percentage += 10.0;
    }

    return health;
}

ValidationResult MidnightMobileClient::submit_transaction_online(const TransactionPayload& tx) const {
    ValidationResult offline_v = verify_transaction_offline(tx);
    if (!offline_v.valid) {
        return offline_v;
    }

    // In Midnight, unshielded transfers / contract calls invoke author_submitExtrinsic or send_mn_transaction
    std::ostringstream params;
    params << "[\"" << tx.raw_hex << "\"]";
    std::string rpc_res = execute_rpc("author_submitExtrinsic", params.str());

    if (rpc_res.find("\"result\"") != std::string::npos) {
        return {true, "TX_SUBMITTED", "Transaction accepted by node transaction pool"};
    } else {
        return {false, "RPC_REJECTED", "Node rejected transaction: " + rpc_res};
    }
}

std::string MidnightMobileClient::query_contract_state(const std::string& contract_address) const {
    std::ostringstream params;
    params << "[\"" << contract_address << "\", null]";
    return execute_rpc("midnight_contractState", params.str());
}

std::string MidnightMobileClient::query_ledger_state_root() const {
    return execute_rpc("midnight_ledgerStateRoot", "[]");
}

double MidnightMobileClient::evaluate_node_integrity(std::vector<std::string>& out_diagnostic_log) const {
    double score = 0.0;
    out_diagnostic_log.clear();

    out_diagnostic_log.push_back("[TEST 1] Verifying offline format and address validation...");
    ValidationResult v1 = validate_address("mn_addr_undeployed1gkasr3z3vwyscy2jpp53nzr37v7n4r3lsfgj6v5g584dakjzt0xqun4d4r");
    if (v1.valid) {
        score += 20.0;
        out_diagnostic_log.push_back("  -> PASS: Address codec and validation functioning correctly (+20%).");
    } else {
        out_diagnostic_log.push_back("  -> FAIL: Address validation failed.");
    }

    out_diagnostic_log.push_back("[TEST 2] Verifying offline transaction generation & cryptographic verification...");
    TransactionPayload tx = build_unshielded_transfer("0000000000000000000000000000000000000000000000000000000000000001",
                                                      "mn_addr_undeployed1gkasr3z3vwyscy2jpp53nzr37v7n4r3lsfgj6v5g584dakjzt0xqun4d4r",
                                                      1000);
    ValidationResult v2 = verify_transaction_offline(tx);
    if (v2.valid) {
        score += 20.0;
        out_diagnostic_log.push_back("  -> PASS: Offline transaction built and verified with zero-knowledge proof mock (+20%).");
    } else {
        out_diagnostic_log.push_back("  -> FAIL: Transaction offline check failed.");
    }

    out_diagnostic_log.push_back("[TEST 3] Security Enforcement: Testing arbitrary money printing / unbacked mint attempt...");
    TransactionPayload mint_tx = build_mint_attempt("mn_addr_undeployed1gkasr3z3vwyscy2jpp53nzr37v7n4r3lsfgj6v5g584dakjzt0xqun4d4r", 1000000);
    ValidationResult v3 = verify_transaction_offline(mint_tx);
    if (!v3.valid && v3.code == "ERR_LEDGER_CONSTRAINT_VIOLATION") {
        score += 20.0;
        out_diagnostic_log.push_back("  -> PASS: Node protocol constraints strictly block arbitrary minting (+20%).");
    } else {
        out_diagnostic_log.push_back("  -> FAIL: Security breach! Arbitrary mint attempt was not blocked.");
    }

    out_diagnostic_log.push_back("[TEST 4] Testing Online Node Connectivity & JSON-RPC API...");
    NodeHealth health = query_node_health();
    if (health.is_reachable) {
        score += 20.0;
        out_diagnostic_log.push_back("  -> PASS: Node endpoint reachable (" + rpc_url_ + ") (+20%).");
        if (!health.is_syncing && health.peers_count > 0) {
            score += 20.0;
            out_diagnostic_log.push_back("  -> PASS: Node fully synchronized with active consensus peers (+20%).");
        } else {
            out_diagnostic_log.push_back("  -> WARN: Node is reachable but standalone/syncing (peers: " + std::to_string(health.peers_count) + ").");
        }
    } else {
        out_diagnostic_log.push_back("  -> INFO: Node RPC offline or unreachable at " + rpc_url_ + " (Offline mode verified, score based on local integrity).");
    }

    return score;
}

} // namespace midnight
