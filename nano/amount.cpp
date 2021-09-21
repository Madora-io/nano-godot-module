#include "amount.h"

String NanoAmount::get_nano_amount() {
    String str_amount = amount.to_string();
    int length = str_amount.length();
    if(length > 30)
        str_amount = str_amount.insert(length - 30, ".");
    else if(length == 30)
        str_amount = "0." + str_amount;
    else {
        int zeros_needed = 30 - length;
        char zero_arr[zeros_needed + 1];
        for(int i = 0; i < zeros_needed; i++) zero_arr[i] = '0';
        zero_arr[zeros_needed] = '\0';
        String zeros(zero_arr);
        str_amount = "0." + zeros + str_amount;
    }
    return str_amount;
}

String NanoAmount::get_friendly_amount(int decimal_places = 6) {
    String amount = get_nano_amount();
    int decimal_place = amount.find_char('.');
    return amount.substr(0, decimal_place + decimal_places);
}

void NanoAmount::set_amount(String a) {
    ERR_FAIL_INDEX_MSG(a.length(), 39, vformat("Nano amount is larger than maximum possible value."))
    ERR_FAIL_COND_MSG(!a.is_valid_integer(), vformat("Amount is not an integer."))
    ERR_FAIL_COND_MSG(a[0] == '-', vformat("Amount cannot be negative."))
    amount = a;
}

void NanoAmount::set_nano_amount(String a) {
    ERR_FAIL_INDEX_MSG(a.length(), 40, vformat("Nano amount is larger than maximum possible value."))
    ERR_FAIL_COND_MSG(!a.is_numeric(), vformat("Amount is not in valid format."))
    ERR_FAIL_COND_MSG(a[0] == '-', vformat("Amount cannot be negative."))
    int decimal_place = a.find_char('.');
    if(decimal_place == -1) amount = a + "000000000000000000000000000000";
    else {
        int zeros_needed = 30 - (a.length() - decimal_place);
        char zero_arr[zeros_needed + 1];
        for(int i = 0; i < zeros_needed; i++) zero_arr[i] = '0';
        zero_arr[zeros_needed] = '\0';

        a.remove(decimal_place);
        amount = a + zero_arr;
    }
}

void NanoAmount::add(Ref<NanoAmount> a) { amount += a->amount; }
void NanoAmount::sub(Ref<NanoAmount> a) { amount -= a->amount; }

bool NanoAmount::equals(Ref<NanoAmount> a) { return amount == a->amount; }
bool NanoAmount::greater_than(Ref<NanoAmount> a) { return amount > a->amount; }
bool NanoAmount::greater_than_or_equal(Ref<NanoAmount> a) { return amount >= a->amount; }
bool NanoAmount::less_than(Ref<NanoAmount> a) { return amount < a->amount; }
bool NanoAmount::less_than_or_equal(Ref<NanoAmount> a) { return amount <= a->amount; }

void NanoAmount::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_amount", "amount"), &NanoAmount::set_amount);
    ClassDB::bind_method(D_METHOD("get_raw_amount"), &NanoAmount::get_raw_amount);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "amount", PROPERTY_HINT_TYPE_STRING, ""), "set_amount", "get_raw_amount");

    ClassDB::bind_method(D_METHOD("get_nano_amount"), &NanoAmount::get_nano_amount);
    ClassDB::bind_method(D_METHOD("set_nano_amount", "amount"), &NanoAmount::set_nano_amount);
    ADD_PROPERTY(PropertyInfo(Variant::STRING, "nano_amount", PROPERTY_HINT_TYPE_STRING, ""), "set_nano_amount", "get_nano_amount");

    ClassDB::bind_method(D_METHOD("get_friendly_amount", "decimal_places"), &NanoAmount::get_friendly_amount, DEFVAL(6));

    ClassDB::bind_method(D_METHOD("add", "a"), &NanoAmount::add);
    ClassDB::bind_method(D_METHOD("sub", "a"), &NanoAmount::sub);

    ClassDB::bind_method(D_METHOD("equals", "a"), &NanoAmount::equals);
    ClassDB::bind_method(D_METHOD("greater_than", "a"), &NanoAmount::greater_than);
    ClassDB::bind_method(D_METHOD("greater_than_or_equal", "a"), &NanoAmount::greater_than_or_equal);
    ClassDB::bind_method(D_METHOD("less_than", "a"), &NanoAmount::less_than);
    ClassDB::bind_method(D_METHOD("less_than_or_equal", "a"), &NanoAmount::less_than_or_equal);
}