// Copyright 2020 Wesley Shillingford. All rights reserved.
#ifdef _WIN32
#pragma warning (disable : 4804 ) /* '/': unsafe use of type 'bool' in operation warnings */
#endif
#include "../blake2/blake2.h"
#include "../duthomhas/csprng.hpp"
#include <array>

extern "C" {
#include "ed25519-hash-custom.h"
void ed25519_randombytes_unsafe (void * out, size_t outlen)
{
	duthomhas::csprng rng;
	rng(out, outlen);
}
void ed25519_hash_init (ed25519_hash_context * ctx)
{
	ctx->blake2 = new blake2b_state;
	blake2b_init (reinterpret_cast<blake2b_state *> (ctx->blake2), 64);
}

void ed25519_hash_update (ed25519_hash_context * ctx, uint8_t const * in, size_t inlen)
{
	blake2b_update (reinterpret_cast<blake2b_state *> (ctx->blake2), in, inlen);
}

void ed25519_hash_final (ed25519_hash_context * ctx, uint8_t * out)
{
	blake2b_final (reinterpret_cast<blake2b_state *> (ctx->blake2), out, 64);
	delete reinterpret_cast<blake2b_state *> (ctx->blake2);
}

void ed25519_hash (uint8_t * out, uint8_t const * in, size_t inlen)
{
	ed25519_hash_context ctx;
	ed25519_hash_init (&ctx);
	ed25519_hash_update (&ctx, in, inlen);
	ed25519_hash_final (&ctx, out);
}
}