#ifndef NANO_AMOUNT_H_
#define NANO_AMOUNT_H_

#include "numbers.h"

using namespace nano;

class NanoAmount : public Reference {
    GDCLASS(NanoAmount, Reference);

    private:
        uint128_union amount;

    protected:
        static void _bind_methods();
    public:
        String get_raw_amount() { return amount.to_string_dec(); };
        String get_nano_amount();
        String get_friendly_amount(int);
        int set_amount(String a);
        int set_nano_amount(String a);

        void add(Ref<NanoAmount> a);
        void sub(Ref<NanoAmount> a);

        bool equals(Ref<NanoAmount> a);
        bool greater_than(Ref<NanoAmount> a);
        bool greater_than_or_equal(Ref<NanoAmount> a);
        bool less_than(Ref<NanoAmount> a);
        bool less_than_or_equal(Ref<NanoAmount> a);

        uint128_union get_amount() { return amount; }
        String to_hex() { return amount.to_string(); }

};

#endif