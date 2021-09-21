#include "account.h"

#include "../blake2/blake2.h"
#include "../duthomhas/csprng.hpp"
#include "../ed25519-donna/ed25519.h"
#include "../qrcode/QrCode.hpp"

#include "core/io/file_access_encrypted.h"

#include <boost/multiprecision/cpp_int.hpp>
#include <vector>

const char base32_characters[33] = "13456789abcdefghijkmnopqrstuwxyz";
const char hex_characters[17] = "0123456789ABCDEF";
char const * account_reverse ("~0~1234567~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~89:;<=>?@AB~CDEFGHIJK~LMNO~~~~~");

void key_string_to_bytes(String const & in, std::array<uint8_t, 32> & out){
    // Seed, Private Key, and Public Key should all be 64 Hexadecimal characters (representing 32 bytes of data)
    ERR_FAIL_COND_MSG(in.length() != 64, "String has incorrect length: " + itos(in.length()) + " String: " + in);
    for(int i = 0; i < in.length(); i+=2){
        out[i/2] = in.substr(i, 2).hex_to_int(false);
    }
}

void key_string_to_bytes16(String const & in, std::array<uint8_t, 16> & out){
    // Balance should be 32 Hexadecimal characters (representing 16 bytes of data)
    ERR_FAIL_COND_MSG(in.length() != 32, "String has incorrect length: " + itos(in.length()) + " String: " + in);
    for(int i = 0; i < in.length(); i+=2){
        out[i/2] = in.substr(i, 2).hex_to_int(false);
    }
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

uint8_t account_decode (char value)
{
    ERR_FAIL_COND_V_MSG(value < '0' || value >= '~', '~', "Invalid address: checksum value out of bounds with " + itos(value));
	auto result (account_reverse[value - 0x30]);
	if (result != '~')
	{
		result -= 0x30;
	}
	return result;
}

void NanoAccount::set_address(String const & a) {
    if(this->address.empty()){
        this->address = a;

        String encoded_val = "";
        if(address.begins_with("nano_")){
            ERR_FAIL_COND_MSG(address.length() != 65, "Invalid nano address");
            encoded_val = address.substr(5);
        } else if(address.begins_with("xrb_")){
            ERR_FAIL_COND_MSG(address.length() != 64, "Invalid xrb address");
            encoded_val = address.substr(4);
        } else ERR_FAIL_MSG("Address is invalid.")

        ERR_FAIL_COND_MSG(encoded_val[0] != '1' && encoded_val[0] != '3', "Invalid address");

        boost::multiprecision::uint512_t decoded;
        for(int i = 0; i < encoded_val.length(); i++){
            uint8_t byte = account_decode(encoded_val[i]);
            ERR_FAIL_COND_MSG(byte == '~', "Invalid address");
            decoded <<= 5;
            decoded += byte;
        }

        boost::multiprecision::uint256_t a = (decoded >> 40).convert_to<boost::multiprecision::uint256_t>();
        boost::multiprecision::export_bits(a, public_key.rbegin(), 8, false);
        uint64_t checksum (decoded & static_cast<uint64_t>(0xffffffffff));

        uint64_t validation (0);
        blake2b_state hash;
        blake2b_init (&hash, 5);
        blake2b_update (&hash, public_key.data(), public_key.size());
        blake2b_final (&hash, reinterpret_cast<uint8_t *> (&validation), 5);
        ERR_FAIL_COND_MSG(checksum != validation, "Checksum does not match");
    }
}

NanoAccount::NanoAccount() {
    boost::multiprecision::uint256_t preamble_num(6);

    for (auto i (preamble.rbegin ()), n (preamble.rend ()); i != n; ++i)
	{
		*i = static_cast<uint8_t> (preamble_num & static_cast<uint8_t> (0xff));
		preamble_num >>= 8;
	}
}

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

Ref<ImageTexture> get_qr_code_for_text(String text) {
    const char * c_text = text.utf8().ptr();
    qrcodegen::QrCode qr = qrcodegen::QrCode::encodeText(c_text, qrcodegen::QrCode::Ecc::MEDIUM);

    int size = qr.getSize();

    Ref<Image> image(memnew(Image));
    image->create(size, size, false, Image::Format::FORMAT_L8);
    image->lock();
    for(int x = 0; x < size; x++) {
        for(int y = 0; y < size; y++) {
            if(qr.getModule(x, y)) {
                image->set_pixel(x, y, Color(0, 0, 0));
            } else {
                image->set_pixel(x, y, Color(1, 1, 1));
            }
        }
    }
    image->unlock();

    Ref<ImageTexture> texture(memnew(ImageTexture));
    texture->create_from_image(image, 0); // 0 means no flags: no mipmap, filter, or repeat (none of which we would want on a qr code)
    return texture;
}

Ref<ImageTexture> NanoAccount::get_qr_code() {
    return get_qr_code_for_text("nano:" + address);
}

Ref<ImageTexture> NanoAccount::get_qr_code_with_amount(Ref<NanoAmount> amount) {
    String text = "nano:" + address;
    text += "?amount=" + amount->get_raw_amount();
    return get_qr_code_for_text(text);
}

std::array<uint8_t, 32> NanoAccount::internal_block_hash(String previous, String representative, String balance, String link) {
    Ref<NanoAmount> a(memnew(NanoAmount));
    a->set_amount(balance);
    String b = a->to_hex();
    
    String formatted_balance;
    if(b.length() == 32) formatted_balance = b;
    else if(b.length() < 32) {
        String zeroes;
        for(int i = 0; i < 32 - b.length(); i++) zeroes += '0';
        formatted_balance = zeroes + b;
    }
    else formatted_balance = b.substr(0, 32);
    std::array<uint8_t, 16> amount;
    key_string_to_bytes16(formatted_balance, amount);

    String formatted_previous;
    if(previous.length() == 64) formatted_previous = previous;
    else if(previous.length() < 64) {
        String zeroes;
        for(int i = 0; i < 64 - previous.length(); i++) zeroes += '0';
        formatted_previous = zeroes + previous;
    }
    else formatted_previous = previous.substr(0, 64);

    Ref<NanoAccount> rep_account(memnew(NanoAccount));
    rep_account->set_address(representative);

    print_line("Hash info: previous: " + formatted_previous + " representative: " + rep_account->get_public_key() + " balance: " + formatted_balance + " link: " + link);

    std::array<uint8_t, 32> pre_array;
    key_string_to_bytes(formatted_previous, pre_array);
    std::array<uint8_t, 32> rep_array;
    key_string_to_bytes(rep_account->get_public_key(), rep_array);
    std::array<uint8_t, 32> link_array;
    key_string_to_bytes(link, link_array);

    std::array<uint8_t, 32> result;
    blake2b_state hash_l;
    blake2b_init (&hash_l, sizeof (result));
    blake2b_update (&hash_l, preamble.data(), preamble.size());
    blake2b_update (&hash_l, public_key.data(), sizeof (public_key));
	blake2b_update (&hash_l, pre_array.data(), sizeof (pre_array));
	blake2b_update (&hash_l, rep_array.data(), sizeof (rep_array));
	blake2b_update (&hash_l, amount.data(), sizeof (amount));
	blake2b_update (&hash_l, link.c_str(), sizeof (link.c_str()));
    blake2b_final (&hash_l, result.data(), sizeof (result));

    return result;
}

String NanoAccount::block_hash(String previous, String representative, String balance, String link) {
    std::array<uint8_t, 32> hash_result = internal_block_hash(previous, representative, balance, link);
    return bytes_to_key_string(hash_result.begin(), hash_result.end());
}

String NanoAccount::sign(String previous, String representative, String balance, String link) {
    std::array<uint8_t, 32> signature_data = internal_block_hash(previous, representative, balance, link);
    print_line("Block hash is " + bytes_to_key_string(signature_data.begin(), signature_data.end()));
    print_line("Private key is " + get_private_key());
    std::array<uint8_t, 64> signature_result;
    ed25519_sign(signature_data.data(), sizeof(signature_data), private_key.data(), public_key.data(), signature_result.data());
    
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

    ClassDB::bind_method(D_METHOD("get_qr_code"), &NanoAccount::get_qr_code);
    ClassDB::bind_method(D_METHOD("get_qr_code_with_amount", "amount"), &NanoAccount::get_qr_code_with_amount);

    ClassDB::bind_method(D_METHOD("block_hash", "previous", "representative", "balance", "link"), &NanoAccount::block_hash);
    ClassDB::bind_method(D_METHOD("sign", "previous", "representative", "balance", "link"), &NanoAccount::sign);
}