#include "midnight_mobile_client.hpp"

#include <iostream>
#include <cassert>
#include <vector>

void test_address_validation() {
    std::cout << "[RUN] Testing address validation..." << std::endl;
    midnight::MidnightMobileClient client;

    // Valid addresses
    auto v1 = client.validate_address("mn_addr_undeployed1gkasr3z3vwyscy2jpp53nzr37v7n4r3lsfgj6v5g584dakjzt0xqun4d4r");
    assert(v1.valid && "Address should be valid");

    auto v2 = client.validate_address("0x1234567890abcdef1234567890abcdef12345678");
    assert(v2.valid && "Hex 20-byte address should be valid");

    // Invalid addresses
    auto v3 = client.validate_address("invalid_random_string");
    assert(!v3.valid && "Invalid string must be rejected");

    auto v4 = client.validate_address("0x123");
    assert(!v4.valid && "Too short hex address must be rejected");

    std::cout << "[PASS] Address validation passed." << std::endl;
}

void test_offline_unshielded_transfer() {
    std::cout << "[RUN] Testing offline transfer generation and verification..." << std::endl;
    midnight::MidnightMobileClient client;

    std::string seed = "0000000000000000000000000000000000000000000000000000000000000001";
    std::string recipient = "mn_addr_undeployed1gkasr3z3vwyscy2jpp53nzr37v7n4r3lsfgj6v5g584dakjzt0xqun4d4r";
    uint64_t amount = 500;

    auto tx = client.build_unshielded_transfer(seed, recipient, amount);
    assert(tx.amount == 500);
    assert(!tx.sender.empty());
    assert(tx.is_signed);
    assert(!tx.signature.empty());
    assert(!tx.proof_zk_hex.empty());

    auto v = client.verify_transaction_offline(tx);
    assert(v.valid && "Generated transfer must pass offline validation");

    // Corrupt amount to 0
    tx.amount = 0;
    auto v_zero = client.verify_transaction_offline(tx);
    assert(!v_zero.valid && "Zero amount must be rejected");

    std::cout << "[PASS] Offline transfer generation and verification passed." << std::endl;
}

void test_security_mint_protection() {
    std::cout << "[RUN] Testing protection against arbitrary money printing / unbacked minting..." << std::endl;
    midnight::MidnightMobileClient client;

    std::string recipient = "mn_addr_undeployed1gkasr3z3vwyscy2jpp53nzr37v7n4r3lsfgj6v5g584dakjzt0xqun4d4r";
    uint64_t amount = 99999999;

    auto mint_tx = client.build_mint_attempt(recipient, amount);
    auto v = client.verify_transaction_offline(mint_tx);

    assert(!v.valid && "Arbitrary mint attempt MUST be rejected");
    assert(v.code == "ERR_LEDGER_CONSTRAINT_VIOLATION");

    std::cout << "[PASS] Protocol security prevents arbitrary minting." << std::endl;
}

void test_integrity_evaluation() {
    std::cout << "[RUN] Testing integrity evaluation pipeline..." << std::endl;
    midnight::MidnightMobileClient client;

    std::vector<std::string> log;
    double score = client.evaluate_node_integrity(log);

    // In offline environment without a running node, local tests should pass (+60%)
    assert(score >= 60.0 && "Offline integrity checks must score at least 60%");
    assert(!log.empty() && "Diagnostic logs must be generated");

    std::cout << "[PASS] Integrity evaluation pipeline completed with score: " << score << "%" << std::endl;
}

int main() {
    std::cout << "Starting Midnight Mobile Client Unit & Integration Tests..." << std::endl;
    test_address_validation();
    test_offline_unshielded_transfer();
    test_security_mint_protection();
    test_integrity_evaluation();
    std::cout << "ALL TESTS PASSED SUCCESSFULLY!" << std::endl;
    return 0;
}
