// Copyright 2020 Wesley Shillingford. All rights reserved.
#include <nano/numbers.h>

#include "../blake2/blake2.h"
#include "../ed25519-donna/ed25519.h"

#include <iomanip>
#include <sstream>

namespace
{
const char hex_characters[17] = "0123456789ABCDEF";

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
}

nano::uint256_union::uint256_union (nano::uint256_t const & number_a)
{
	nano::uint256_t number_l (number_a);
	for (auto i (bytes.rbegin ()), n (bytes.rend ()); i != n; ++i)
	{
		*i = static_cast<uint8_t> ((number_l & static_cast<uint8_t> (0xff)));
		number_l >>= 8;
	}
}

bool nano::uint256_union::operator== (nano::uint256_union const & other_a) const
{
	return bytes == other_a.bytes;
}

bool nano::uint256_union::is_zero () const
{
	return qwords[0] == 0 && qwords[1] == 0 && qwords[2] == 0 && qwords[3] == 0;
}

String nano::uint256_union::to_string () const
{
	String result;
	encode_hex (result);
	return result;
}

bool nano::uint256_union::operator< (nano::uint256_union const & other_a) const
{
	return std::memcmp (bytes.data (), other_a.bytes.data (), 32) < 0;
}

nano::uint256_union & nano::uint256_union::operator^= (nano::uint256_union const & other_a)
{
	auto j (other_a.qwords.begin ());
	for (auto i (qwords.begin ()), n (qwords.end ()); i != n; ++i, ++j)
	{
		*i ^= *j;
	}
	return *this;
}

nano::uint256_union nano::uint256_union::operator^ (nano::uint256_union const & other_a) const
{
	nano::uint256_union result;
	auto k (result.qwords.begin ());
	for (auto i (qwords.begin ()), j (other_a.qwords.begin ()), n (qwords.end ()); i != n; ++i, ++j, ++k)
	{
		*k = *i ^ *j;
	}
	return result;
}

nano::uint256_union::uint256_union (String const & hex_a)
{
	decode_hex (hex_a);
}

void nano::uint256_union::clear ()
{
	qwords.fill (0);
}

nano::uint256_t nano::uint256_union::number () const
{
	nano::uint256_t result;
	auto shift (0);
	for (auto i (bytes.begin ()), n (bytes.end ()); i != n; ++i)
	{
		result <<= shift;
		result |= *i;
		shift = 8;
	}
	return result;
}

void nano::uint256_union::encode_hex (String & text) const
{
	text = bytes_to_key_string(bytes.begin(), bytes.end());
}

int nano::uint256_union::decode_hex (String const & text)
{
    ERR_FAIL_COND_V_MSG(text.empty() || text.length() > 64, 1, "Invalid text for uint256: " + text);
    const char * c_text = text.ascii();
    std::stringstream stream (c_text);
    stream << std::hex << std::noshowbase;
    nano::uint256_t number_l;

	stream >> number_l;
	*this = number_l;
	ERR_FAIL_COND_V_MSG(!stream.eof(), 1, "Invalid text for uint256 (likely too large): " + text);

    return 0;
}

void nano::uint256_union::encode_dec (String & text) const
{
	std::stringstream stream;
	stream << std::dec << std::noshowbase;
	stream << number();
	text = stream.str().c_str();
}

int nano::uint256_union::decode_dec (String const & text)
{
    ERR_FAIL_COND_V_MSG(text.size () > 78, 1, "Text too large");
    ERR_FAIL_COND_V_MSG((!text.empty () && text.begins_with("-")), 1, "uint256 cannot be negative");
    const char * c_text = text.ascii();
    std::stringstream stream (c_text);
    stream << std::dec << std::noshowbase;
    nano::uint256_t number_l;

	stream >> number_l;
	*this = number_l;
	ERR_FAIL_COND_V_MSG(!stream.eof(), 1, "Invalid text for uint256 (likely too large): " + text);

    return 0;
}

nano::uint256_union::uint256_union (uint64_t value0)
{
	*this = nano::uint256_t (value0);
}

bool nano::uint256_union::operator!= (nano::uint256_union const & other_a) const
{
	return !(*this == other_a);
}

bool nano::uint512_union::operator== (nano::uint512_union const & other_a) const
{
	return bytes == other_a.bytes;
}

nano::uint512_union::uint512_union (nano::uint256_union const & upper, nano::uint256_union const & lower)
{
	uint256s[0] = upper;
	uint256s[1] = lower;
}

nano::uint512_union::uint512_union (nano::uint512_t const & number_a)
{
	nano::uint512_t number_l (number_a);
	for (auto i (bytes.rbegin ()), n (bytes.rend ()); i != n; ++i)
	{
		*i = static_cast<uint8_t> ((number_l & static_cast<uint8_t> (0xff)));
		number_l >>= 8;
	}
}

bool nano::uint512_union::is_zero () const
{
	return qwords[0] == 0 && qwords[1] == 0 && qwords[2] == 0 && qwords[3] == 0
	&& qwords[4] == 0 && qwords[5] == 0 && qwords[6] == 0 && qwords[7] == 0;
}

void nano::uint512_union::clear ()
{
	bytes.fill (0);
}

nano::uint512_t nano::uint512_union::number () const
{
	nano::uint512_t result;
	auto shift (0);
	for (auto i (bytes.begin ()), n (bytes.end ()); i != n; ++i)
	{
		result <<= shift;
		result |= *i;
		shift = 8;
	}
	return result;
}

void nano::uint512_union::encode_hex (String & text) const
{
	text = bytes_to_key_string(bytes.begin(), bytes.end());
}

int nano::uint512_union::decode_hex (String const & text)
{
	ERR_FAIL_COND_V_MSG(text.length () > 128, 1, "Value too large");
	
    const char * c_text = text.ascii();
    std::stringstream stream (c_text);
    stream << std::hex << std::noshowbase;
    nano::uint512_t number_l;

	stream >> number_l;
	*this = number_l;
	ERR_FAIL_COND_V_MSG(!stream.eof(), 1, "Invalid text for uint512 (likely too large): " + text);

    return 0;
}

bool nano::uint512_union::operator!= (nano::uint512_union const & other_a) const
{
	return !(*this == other_a);
}

nano::uint512_union & nano::uint512_union::operator^= (nano::uint512_union const & other_a)
{
	uint256s[0] ^= other_a.uint256s[0];
	uint256s[1] ^= other_a.uint256s[1];
	return *this;
}

String nano::uint512_union::to_string () const
{
	String result;
	encode_hex (result);
	return result;
}

nano::raw_key::~raw_key ()
{
	data.clear ();
}

bool nano::raw_key::operator== (nano::raw_key const & other_a) const
{
	return data == other_a.data;
}

bool nano::raw_key::operator!= (nano::raw_key const & other_a) const
{
	return !(*this == other_a);
}

nano::uint512_union nano::sign_message (nano::raw_key const & private_key, nano::public_key const & public_key, nano::uint256_union const & message)
{
	nano::uint512_union result;
	ed25519_sign (message.bytes.data (), sizeof (message.bytes), private_key.data.bytes.data (), public_key.bytes.data (), result.bytes.data ());
	return result;
}

void nano::deterministic_key (nano::uint256_union const & seed_a, uint32_t index_a, nano::uint256_union & prv_a)
{
	blake2b_state hash;
	blake2b_init (&hash, prv_a.bytes.size ());
	blake2b_update (&hash, seed_a.bytes.data (), seed_a.bytes.size ());
	nano::uint256_union index (index_a);
	blake2b_update (&hash, reinterpret_cast<uint8_t *> (&index.dwords[7]), sizeof (uint32_t));
	blake2b_final (&hash, prv_a.bytes.data (), prv_a.bytes.size ());
}

nano::public_key nano::pub_key (nano::private_key const & privatekey_a)
{
	nano::uint256_union result;
	ed25519_publickey (privatekey_a.bytes.data (), result.bytes.data ());
	return result;
}

bool nano::validate_message (nano::public_key const & public_key, nano::uint256_union const & message, nano::uint512_union const & signature)
{
	auto result (0 != ed25519_sign_open (message.bytes.data (), sizeof (message.bytes), public_key.bytes.data (), signature.bytes.data ()));
	return result;
}

bool nano::validate_message_batch (const unsigned char ** m, size_t * mlen, const unsigned char ** pk, const unsigned char ** RS, size_t num, int * valid)
{
	bool result (0 == ed25519_sign_open_batch (m, mlen, pk, RS, num, valid));
	return result;
}

nano::uint128_union::uint128_union (String const & string_a)
{
	decode_hex (string_a);
}

nano::uint128_union::uint128_union (uint64_t value_a)
{
	*this = nano::uint128_t (value_a);
}

nano::uint128_union::uint128_union (nano::uint128_t const & number_a)
{
	nano::uint128_t number_l (number_a);
	for (auto i (bytes.rbegin ()), n (bytes.rend ()); i != n; ++i)
	{
		*i = static_cast<uint8_t> ((number_l & static_cast<uint8_t> (0xff)));
		number_l >>= 8;
	}
}

bool nano::uint128_union::operator== (nano::uint128_union const & other_a) const
{
	return qwords[0] == other_a.qwords[0] && qwords[1] == other_a.qwords[1];
}

bool nano::uint128_union::operator!= (nano::uint128_union const & other_a) const
{
	return !(*this == other_a);
}

bool nano::uint128_union::operator< (nano::uint128_union const & other_a) const
{
	return std::memcmp (bytes.data (), other_a.bytes.data (), 16) < 0;
}

bool nano::uint128_union::operator> (nano::uint128_union const & other_a) const
{
	return std::memcmp (bytes.data (), other_a.bytes.data (), 16) > 0;
}

nano::uint128_t nano::uint128_union::number () const
{
	nano::uint128_t result;
	auto shift (0);
	for (auto i (bytes.begin ()), n (bytes.end ()); i != n; ++i)
	{
		result <<= shift;
		result |= *i;
		shift = 8;
	}
	return result;
}

void nano::uint128_union::encode_hex (String & text) const
{
	text = bytes_to_key_string(bytes.begin(), bytes.end());
}

int nano::uint128_union::decode_hex (String const & text)
{
    ERR_FAIL_COND_V_MSG(text.length () > 32, 1, "Value too large");


    const char * c_text = text.ascii();
    std::stringstream stream (c_text);
    stream << std::hex << std::noshowbase;
    nano::uint128_t number_l;

	stream >> number_l;
	*this = number_l;
	ERR_FAIL_COND_V_MSG(!stream.eof(), 1, "Invalid text for uint128 (likely too large): " + text);

    return 0;
}

void nano::uint128_union::encode_dec (String & text) const
{
	std::stringstream stream;
	stream << std::dec << std::noshowbase;
	stream << number ();
	text = stream.str().c_str();
}

int nano::uint128_union::decode_dec (String const & text, bool decimal)
{
    ERR_FAIL_COND_V_MSG(text.size () > 39, 1, "Text too large");
    ERR_FAIL_COND_V_MSG((!text.empty () && text.begins_with("-")), 1, "uint128 cannot be negative");

    const char * c_text = text.ascii();
    std::stringstream stream (c_text);
    stream << std::dec << std::noshowbase;
    boost::multiprecision::checked_uint128_t number_l;

	stream >> number_l;
	nano::uint128_t unchecked (number_l);
	*this = unchecked;
	ERR_FAIL_COND_V_MSG(!stream.eof(), 1, "Invalid text for uint128 (likely too large): " + text);

    return 0;
}

void nano::uint128_union::clear ()
{
	qwords.fill (0);
}

bool nano::uint128_union::is_zero () const
{
	return qwords[0] == 0 && qwords[1] == 0;
}

String nano::uint128_union::to_string () const
{
	String result;
	encode_hex (result);
	return result;
}

String nano::uint128_union::to_string_dec () const
{
	String result;
	encode_dec (result);
	return result;
}