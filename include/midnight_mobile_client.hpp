#ifndef MIDNIGHT_MOBILE_CLIENT_HPP
#define MIDNIGHT_MOBILE_CLIENT_HPP

#include <string>
#include <vector>
#include <cstdint>
#include <memory>

namespace midnight {

// Types of operations supported on Midnight Ledger
enum class TxType {
    UnshieldedTransfer,
    ContractCall,
    ContractDeploy,
    BridgeDepositObservation,
    ArbitraryMintAttempt // Evaluated to demonstrate rejection
};

struct TransactionPayload {
    TxType type;
    std::string sender;
    std::string recipient;
    uint64_t amount;
    std::string proof_zk_hex;
    std::string raw_hex;
    bool is_signed;
    std::string signature;
};

struct NodeHealth {
    bool is_reachable;
    bool is_syncing;
    uint32_t peers_count;
    std::string best_block_hash;
    uint64_t best_block_number;
    std::string ledger_state_root;
    std::vector<uint32_t> api_versions;
    double health_percentage;
};

struct ValidationResult {
    bool valid;
    std::string code;
    std::string message;
};

class MidnightMobileClient {
public:
    MidnightMobileClient(const std::string& rpc_url = "http://127.0.0.1:9944");
    ~MidnightMobileClient() = default;

    // Configuration
    void set_rpc_url(const std::string& url);
    std::string get_rpc_url() const;

    // Offline capabilities (Full mobile standalone verification)
    ValidationResult validate_address(const std::string& address) const;
    TransactionPayload build_unshielded_transfer(const std::string& sender_seed,
                                                 const std::string& recipient_addr,
                                                 uint64_t amount) const;
    TransactionPayload build_mint_attempt(const std::string& target_addr,
                                          uint64_t amount) const;
    ValidationResult verify_transaction_offline(const TransactionPayload& tx) const;

    // Online capabilities (JSON-RPC Substrate & Midnight Pallets)
    NodeHealth query_node_health() const;
    ValidationResult submit_transaction_online(const TransactionPayload& tx) const;
    std::string query_contract_state(const std::string& contract_address) const;
    std::string query_ledger_state_root() const;

    // Comprehensive node test (combines online & offline checks to verify if node is 100%)
    double evaluate_node_integrity(std::vector<std::string>& out_diagnostic_log) const;

private:
    std::string rpc_url_;

    // Helper for executing HTTP/JSON-RPC requests without heavy external dependencies
    std::string execute_rpc(const std::string& method, const std::string& params_json) const;
    static std::string derive_address_from_seed(const std::string& seed);
};

} // namespace midnight

#endif // MIDNIGHT_MOBILE_CLIENT_HPP
