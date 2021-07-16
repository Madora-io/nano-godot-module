#ifndef NANO_NANO_ACCOUNT_H_
#define NANO_NANO_ACCOUNT_H_

#include <vector>
#include <iostream>

namespace nano {

class NanoAccount {
    private:
        std::string private_key;
        std::string public_key;
        std::string address;
    public:
        NanoAccount();
        void set_address(std::string const &);
        std::string get_address() {return address; }
};

NanoAccount from_seed(std::string const &, int const &);

}

#endif