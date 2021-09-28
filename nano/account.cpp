#include "account.h"

#include "numbers.h"
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

int key_string_to_bytes(String const & in, std::array<uint8_t, 32> & out){
    // Seed, Private Key, and Public Key should all be 64 Hexadecimal characters (representing 32 bytes of data)
    ERR_FAIL_COND_V_MSG(in.length() != 64, 1, "String has incorrect length: " + itos(in.length()) + " String: " + in);
    for(int i = 0; i < in.length(); i+=2){
        out[i/2] = in.substr(i, 2).hex_to_int(false);
    }
    return 0;
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

int NanoAccount::set_address(String const & a) {
    if(this->address.empty()){
        this->address = a;

        String encoded_val = "";
        if(address.begins_with("nano_")){
            ERR_FAIL_COND_V_MSG(address.length() != 65, 1, "Invalid nano address");
            encoded_val = address.substr(5);
        } else if(address.begins_with("xrb_")){
            ERR_FAIL_COND_V_MSG(address.length() != 64, 1, "Invalid xrb address");
            encoded_val = address.substr(4);
        } else ERR_FAIL_V_MSG(1, "Address is invalid.")

        ERR_FAIL_COND_V_MSG(encoded_val[0] != '1' && encoded_val[0] != '3', 1, "Invalid address");

        boost::multiprecision::uint512_t decoded;
        for(int i = 0; i < encoded_val.length(); i++){
            uint8_t byte = account_decode(encoded_val[i]);
            ERR_FAIL_COND_V_MSG(byte == '~', 1, "Invalid address");
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
        ERR_FAIL_COND_V_MSG(checksum != validation, 1, "Checksum does not match");
    }
    return 0;
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

int NanoAccount::set_seed(String const & s) {
    return set_seed_and_index(s, 0);
}

int NanoAccount::set_seed_and_index(String const & s, uint32_t index) {
    if(key_string_to_bytes(s, seed)) return 1;
    this->index = index;

    generate_keys_and_address();
    return 0;
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

uint256_union NanoAccount::internal_block_hash(String previous, Ref<NanoAccount> representative, Ref<NanoAmount> balance, String link) {
    nano::uint256_union prev_u(previous);
    nano::uint256_union link_u(link);
    
    nano::uint256_union result;
	blake2b_state hash_l;
	auto status (blake2b_init (&hash_l, sizeof (result.bytes)));
    nano::uint256_union preamble (6);
	blake2b_update (&hash_l, preamble.bytes.data (), preamble.bytes.size ());
    blake2b_update (&hash_l, public_key.data (), sizeof (public_key));
	blake2b_update (&hash_l, prev_u.bytes.data (), sizeof (prev_u.bytes));
	blake2b_update (&hash_l, representative->public_key.data (), sizeof (representative->public_key));
	blake2b_update (&hash_l, balance->get_amount().bytes.data (), sizeof (balance->get_amount().bytes));
	blake2b_update (&hash_l, link_u.bytes.data (), sizeof (link_u.bytes));
    status = blake2b_final (&hash_l, result.bytes.data (), sizeof (result.bytes));
    ERR_FAIL_COND_V_MSG(status, uint256_union(), "Block hashing failed");
	return result;
}

String NanoAccount::block_hash(String previous, Ref<NanoAccount> representative, Ref<NanoAmount> balance, String link) {
    return internal_block_hash(previous, representative, balance, link).to_string();
}

String NanoAccount::sign(String previous, Ref<NanoAccount> representative, Ref<NanoAmount> balance, String link) {
    uint256_union message = internal_block_hash(previous, representative, balance, link);

    uint512_union result;
	ed25519_sign (message.bytes.data (), sizeof (message.bytes), private_key.data (), public_key.data (), result.bytes.data ());
	return result.to_string();
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