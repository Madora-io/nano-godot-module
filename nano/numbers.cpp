// Copyright 2020 Wesley Shillingford. All rights reserved.
#include <nano/numbers.h>

#include "../blake2/blake2.h"
#include "../ed25519-donna/ed25519.h"

#include <iomanip>
#include <sstream>

namespace
{
const char hex_characters[17] = "0123456789ABCDEF";

char const * account_lookup ("13456789abcdefghijkmnopqrstuwxyz");
char const * account_reverse ("~0~1234567~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~89:;<=>?@AB~CDEFGHIJK~LMNO~~~~~");
char account_encode (uint8_t value)
{
	ERR_FAIL_COND(value >= 32);
	auto result (account_lookup[value]);
	return result;
}
uint8_t account_decode (char value)
{
	ERR_FAIL_COND(value < '0');
	ERR_FAIL_COND(value > '~');
	auto result (account_reverse[value - 0x30]);
	if (result != '~')
	{
		result -= 0x30;
	}
	return result;
}
}

// void nano::uint256_union::encode_account (String & destination_a) const
// {
// 	ERR_FAIL_COND_MSG(!destination_a.empty(), "String not empty");

// 	uint64_t check1 (0);
// 	blake2b_state hash;
// 	blake2b_init (&hash, 5);
// 	blake2b_update (&hash, bytes.data (), bytes.size ());
// 	blake2b_final (&hash, reinterpret_cast<uint8_t *> (&check1), 5);
// 	nano::uint512_t number_l;
// 	number_l.Parse (number ().ToString ());
// 	number_l <<= 40;
// 	number_l |= nano::uint512_t (check1);
// 	for (auto i (0); i < 60; ++i)
// 	{
// 		uint8_t r ((number_l & static_cast<uint8_t> (0x1f)).ToInt ());
// 		number_l >>= 5;
// 		destination_a.push_back (account_encode (r));
// 	}
// 	destination_a.append ("_onan"); // nano_
// 	std::reverse (destination_a.begin (), destination_a.end ());
// }

// String nano::uint256_union::to_account () const
// {
// 	String result;
// 	encode_account (result);
// 	return result;
// }

// bool nano::uint256_union::decode_account (String const & source_a)
// {
// 	auto error (source_a.size () < 5);
// 	if (!error)
// 	{
// 		auto xrb_prefix (source_a[0] == 'x' && source_a[1] == 'r' && source_a[2] == 'b' && (source_a[3] == '_' || source_a[3] == '-'));
// 		auto nano_prefix (source_a[0] == 'n' && source_a[1] == 'a' && source_a[2] == 'n' && source_a[3] == 'o' && (source_a[4] == '_' || source_a[4] == '-'));
// 		error = (xrb_prefix && source_a.size () != 64) || (nano_prefix && source_a.size () != 65);
// 		if (!error)
// 		{
// 			if (xrb_prefix || nano_prefix)
// 			{
// 				auto i (source_a.begin () + (xrb_prefix ? 4 : 5));
// 				if (*i == '1' || *i == '3')
// 				{
// 					nano::uint512_t number_l;
// 					for (auto j (source_a.end ()); !error && i != j; ++i)
// 					{
// 						uint8_t character (*i);
// 						error = character < 0x30 || character >= 0x80;
// 						if (!error)
// 						{
// 							uint8_t byte (account_decode (character));
// 							error = byte == '~';
// 							if (!error)
// 							{
// 								number_l <<= 5;
// 								number_l += byte;
// 							}
// 						}
// 					}
// 					if (!error)
// 					{
// 						FString str = (number_l >> 40).ToString ();
// 						str.RemoveFromStart ("0x");
// 						*this = String (TCHAR_TO_UTF8 (*str));
// 						uint64_t check1 ((number_l & static_cast<uint64_t> (0xffffffffff)).ToInt ());
// 						uint64_t validation (0);
// 						blake2b_state hash;
// 						blake2b_init (&hash, 5);
// 						blake2b_update (&hash, bytes.data (), bytes.size ());
// 						blake2b_final (&hash, reinterpret_cast<uint8_t *> (&validation), 5);
// 						error = check1 != validation;
// 					}
// 				}
// 				else
// 				{
// 					error = true;
// 				}
// 			}
// 			else
// 			{
// 				error = true;
// 			}
// 		}
// 	}
// 	return error;
// }

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
	auto error (decode_hex (hex_a));

	ERR_FAIL_COND(error);
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

bool nano::uint256_union::decode_hex (String const & text)
{
	auto error (false);
	if (!text.empty () && text.size () <= 64)
	{
        const char * c_text = text.ascii();
		std::stringstream stream (c_text);
		stream << std::hex << std::noshowbase;
		nano::uint256_t number_l;
		try
		{
			stream >> number_l;
			*this = number_l;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	else
	{
		error = true;
	}
	return error;
}

void nano::uint256_union::encode_dec (String & text) const
{
	std::stringstream stream;
	stream << std::dec << std::noshowbase;
	stream << number();
	text = stream.str().c_str();
}

bool nano::uint256_union::decode_dec (String const & text)
{
	auto error (text.size () > 78 || (text.size () > 1 && text.begins_with("0")) || (!text.empty () && text.begins_with("-")));
	if (!error)
	{
        const char * c_text = text.ascii();
		std::stringstream stream (c_text);
		stream << std::dec << std::noshowbase;
		nano::uint256_t number_l;
		try
		{
			stream >> number_l;
			*this = number_l;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	return error;
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

bool nano::uint512_union::decode_hex (String const & text)
{
	auto error (text.size () > 128);
	if (!error)
	{
        const char * c_text = text.ascii();
		std::stringstream stream (c_text);
		stream << std::hex << std::noshowbase;
		nano::uint512_t number_l;
		try
		{
			stream >> number_l;
			*this = number_l;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	return error;
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
	auto error (decode_hex (string_a));
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

bool nano::uint128_union::decode_hex (String const & text)
{
	auto error (text.size () > 32);
	if (!error)
	{
        const char * c_text = text.ascii();
		std::stringstream stream (c_text);
		stream << std::hex << std::noshowbase;
		nano::uint128_t number_l;
		try
		{
			stream >> number_l;
			*this = number_l;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	return error;
}

void nano::uint128_union::encode_dec (String & text) const
{
	std::stringstream stream;
	stream << std::dec << std::noshowbase;
	stream << number ();
	text = stream.str().c_str();
}

bool nano::uint128_union::decode_dec (String const & text, bool decimal)
{
	auto error (text.size () > 39 || (text.size () > 1 && text.begins_with("0") && !decimal) || (!text.empty () && text.begins_with("-")));
	if (!error)
	{
        const char * c_text = text.ascii();
		std::stringstream stream (c_text);
		stream << std::dec << std::noshowbase;
		boost::multiprecision::checked_uint128_t number_l;
		try
		{
			stream >> number_l;
			nano::uint128_t unchecked (number_l);
			*this = unchecked;
			if (!stream.eof ())
			{
				error = true;
			}
		}
		catch (std::runtime_error &)
		{
			error = true;
		}
	}
	return error;
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