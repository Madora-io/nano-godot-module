#ifndef NANO_AMOUNT_H_
#define NANO_AMOUNT_H_

#include "../bigint/bigint.h"

using namespace Dodecahedron;

class NanoAmount : public Reference {
    GDCLASS(NanoAmount, Reference);

    private:
        Bigint amount;
        void _set_amount(Bigint a);

    protected:
        static void _bind_methods();
    public:
        String get_raw_amount() { return amount.to_string(); };
        String get_nano_amount();
        String get_friendly_amount(int);
        void set_amount(String a);
        void set_nano_amount(String a);

        void add(Ref<NanoAmount> a);
        void sub(Ref<NanoAmount> a);

        bool equals(Ref<NanoAmount> a);
        bool greater_than(Ref<NanoAmount> a);
        bool greater_than_or_equal(Ref<NanoAmount> a);
        bool less_than(Ref<NanoAmount> a);
        bool less_than_or_equal(Ref<NanoAmount> a);

};

#endif