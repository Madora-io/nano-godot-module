#ifndef NANO_ACCOUNT_H_
#define NANO_ACCOUNT_H_

#include <array>
#include <iostream>
#include "core/reference.h"
#include "core/script_language.h"

class NanoAccount : public Reference {
    GDCLASS(NanoAccount, Reference);

    private:
        std::array<uint8_t, 32> seed;
        std::array<uint8_t, 32> private_key;
        std::array<uint8_t, 32> public_key;
        String address;
        uint32_t index;

        void generate_keys_and_address();
    protected:
        static void _bind_methods();
    public:
        NanoAccount();
        void initialize_with_new_seed(); // Create an account with a newly generated seed
        void set_seed(String const &); // Create first account from seed
        void set_seed_and_index(String const &, uint32_t); // Create account from seed and index

        String get_seed(); // Return the seed as a hex value
        String get_private_key();
        String get_public_key();
        int get_index() { return index; }
        void set_address(String const & a);
        String get_address() { return address; }

        String block_hash(String previous, String representative, String balance, String link);
    
        String sign(String previous, String representative, String balance, String link);
};

#endif