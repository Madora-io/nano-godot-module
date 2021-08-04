
#include "register_types.h"

#include "core/class_db.h"
#include "nano/account.h"
#include "nano/amount.h"
#include "nano/requester.h"
#include "nano/sender.h"

void register_nano_types() {
    ClassDB::register_class<NanoAccount>();
    ClassDB::register_class<NanoAmount>();
    ClassDB::register_class<NanoRequest>();
    ClassDB::register_class<NanoSender>();
}

void unregister_nano_types() {}