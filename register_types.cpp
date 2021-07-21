
#include "register_types.h"

#include "core/class_db.h"
#include "nano.h"

void register_nano_types() {
    ClassDB::register_class<nano::NanoAccount>();
}

void unregister_nano_types() {
   // Nothing to do here in this example.
}