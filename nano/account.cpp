#include "account.h"

#include "../blake2/blake2.h"
#include "../duthomhas/csprng.hpp"
#include "../ed25519-donna/ed25519.h"

#include "core/io/file_access_encrypted.h"

#include <vector>

const char base32_characters[33] = "13456789abcdefghijkmnopqrstuwxyz";
const char hex_characters[17] = "0123456789ABCDEF";

int key_string_to_bytes(String const & in, std::array<uint8_t, 32> & out){
    // Seed, Private Key, and Public Key should all be 64 Hexadecimal characters (representing 32 bytes of data)
    assert(in.length() == 64);
    for(int i = 0; i < in.length(); i+=2){
        out[i/2] = in.substr(i, 2).hex_to_int(false);
    }
    return 0;
}

String uint8_t_to_hex(uint8_t const & in) {
    char hex[3] = {hex_characters[in/16], hex_characters[in%16], '\0'};
    return String(hex);
}

template <typename Iter>
String bytes_to_key_string(Iter first, Iter last) {
    String out;
    for(; first != last; ++first){
        out += uint8_t_to_hex(*first);
    }
    return out;
}

String encode_base32(uint8_t* bytes, size_t length) {
    int leftover = (length * 8) % 5;
    int offset = leftover == 0 ? 0 : 5 - leftover;

    int value = 0, bits = 0;

    String output = "";

    for (size_t i = 0; i < length; i++) {
        value = (value << 8) | bytes[i];
        bits += 8;
        while (bits >= 5) {
            output += (base32_characters[(value >> (bits + offset - 5)) & 31]);
            bits -= 5;
        }
    }

    if (bits > 0) {
        output += (base32_characters[(value << (5 - (bits + offset))) & 31]);
    }

    return output;
}

void NanoAccount::set_address(String const & a) {
    if(this->address.empty())
        this->address = a;
}

NanoAccount::NanoAccount() {}

void NanoAccount::generate_keys_and_address() {
    blake2b_state hash;
    blake2b_init(&hash, private_key.size());
    blake2b_update(&hash, seed.data(), seed.size());
    blake2b_update(&hash, reinterpret_cast<uint8_t *> (&index), sizeof(uint32_t));
    blake2b_final(&hash, private_key.data(), private_key.size());
    

    // Create public key from private key
    ed25519_publickey(private_key.data(), public_key.data());

    // Create address from public key
    std::array<uint8_t, 5> checksum;
    blake2b(checksum.data(), 5, public_key.data(), 32, NULL, 0);
    std::reverse(checksum.begin(), checksum.end());

    address = "nano_" + encode_base32(public_key.data(), 32) + encode_base32(checksum.data(), 5);
}

void NanoAccount::initialize_with_new_seed() {
    duthomhas::csprng rng;
    rng(seed);

    generate_keys_and_address();
}

void NanoAccount::set_seed(String const & s) {
    set_seed_and_index(s, 0);
}

void NanoAccount::set_seed_and_index(String const & s, uint32_t index) {
    key_string_to_bytes(s, seed);
    this->index = index;

    generate_keys_and_address();
}

String NanoAccount::block_hash(String previous, String representative, String balance, String link) {
    std::array<uint8_t, 176> hash_data;
    uint8_t preamble[32] = {0x6};
    memcpy(hash_data.data(), preamble, 32);
    memcpy(&hash_data[32], get_public_key().c_str(), 32);
    memcpy(&hash_data[64], previous.c_str(), 32);
    memcpy(&hash_data[96], representative.c_str(), 32);
    memcpy(&hash_data[128], balance.c_str(), 16);
    memcpy(&hash_data[144], link.c_str(), 32);
    std::array<uint8_t, 32> hash_result;

    blake2b(hash_result.data(), hash_result.size(), hash_data.data(), hash_data.size(), NULL, 0);

    return bytes_to_key_string(hash_result.begin(), hash_result.end());
}

String NanoAccount::sign(String previous, String representative, String balance, String link) {
    std::array<uint8_t, 176> signature_data;
    uint8_t preamble[32] = {0x6};
    memcpy(signature_data.data(), preamble, 32);
    memcpy(&signature_data[32], get_public_key().c_str(), 32);
    memcpy(&signature_data[64], previous.c_str(), 32);
    memcpy(&signature_data[96], representative.c_str(), 32);
    memcpy(&signature_data[128], balance.c_str(), 16);
    memcpy(&signature_data[144], link.c_str(), 32);
    std::array<uint8_t, 64> signature_result;
    ed25519_sign(signature_data.data(), 176, private_key.data(), public_key.data(), signature_result.data());
    
    return bytes_to_key_string(signature_result.begin(), signature_result.end());
}

String NanoAccount::get_seed() {
    return bytes_to_key_string(seed.begin(), seed.end());
}

String NanoAccount::get_private_key() {
    return bytes_to_key_string(private_key.begin(), private_key.end());
}

String NanoAccount::get_public_key() {
    return bytes_to_key_string(public_key.begin(), public_key.end());
}

void NanoAccount::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_address", "address"), &NanoAccount::set_address);
    ClassDB::bind_method(D_METHOD("get_address"), &NanoAccount::get_address);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "address", PROPERTY_HINT_TYPE_STRING, ""), "set_address", "get_address");

    ClassDB::bind_method(D_METHOD("initialize_with_new_seed"), &NanoAccount::initialize_with_new_seed);
    ClassDB::bind_method(D_METHOD("set_seed", "seed"), &NanoAccount::set_seed);
    ClassDB::bind_method(D_METHOD("set_seed_and_index", "seed", "index"), &NanoAccount::set_seed_and_index);
    ClassDB::bind_method(D_METHOD("get_seed"), &NanoAccount::get_seed);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "seed", PROPERTY_HINT_TYPE_STRING, ""), "set_seed", "get_seed");

    ClassDB::bind_method(D_METHOD("get_private_key"), &NanoAccount::get_private_key);
    ClassDB::bind_method(D_METHOD("get_public_key"), &NanoAccount::get_public_key);
    ClassDB::bind_method(D_METHOD("get_index"), &NanoAccount::get_index);

    ClassDB::bind_method(D_METHOD("sign", "previous", "representative", "balance", "link"), &NanoAccount::sign);
}