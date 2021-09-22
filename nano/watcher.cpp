#include "watcher.h"

#include "core/io/json.h"
#include "core/method_bind_ext.gen.inc"

NanoWatcher::NanoWatcher() {
    Ref<WebSocketClient> client(WebSocketClient::create());
    _client = client;
    ERR_FAIL_COND_MSG(_client.is_null(), "Client was null, connection not established");

    _client->connect("connection_closed", this, "_closed");
    _client->connect("connection_error", this, "_closed");
    _client->connect("connection_established", this, "_connected");

    _client->connect("data_received", this, "_on_data");
}

NanoReceiver * NanoWatcher::get_free_receiver() {
    for(int i = 0; i < receiver_pool.size(); i++){
        NanoReceiver *r = Object::cast_to<NanoReceiver>(receiver_pool[i]);
        if(r->is_ready()) return r;
    }

    // No ready receiver found, have to make a new one
    NanoReceiver * new_receiver = memnew(NanoReceiver);
    add_child(new_receiver);
    new_receiver->set_connection_parameters(node_url, default_rep, auth_header, use_ssl, work_url, use_peers);
    new_receiver->connect("nano_receive_completed", this, "_auto_recieve_completed");
    receiver_pool.push_back(new_receiver);
    return new_receiver;
}

void NanoWatcher::_auto_recieve_completed(Ref<NanoAccount> account, String message, int code) {
    emit_signal("nano_receive_completed", account, message, code);
}

Ref<NanoAccount> NanoWatcher::lookup_watched_account(String address) {
    for(int i = 0; i < watched_accounts.size(); i++) {
        Ref<NanoAccount> acc = watched_accounts[i];
        if(acc->get_address() == address) {
            return acc;
        }
    }
    return NULL;
}

void NanoWatcher::initialize_and_connect(String node_url, Ref<NanoAccount> default_representative, String auth_header, bool use_ssl, String work_url, bool use_peers) {
    this->node_url = node_url;
    this->default_rep = default_representative;
    if(work_url.empty()) this->work_url = node_url;
    else this->work_url = work_url;
    this->auth_header = auth_header;
    this->use_ssl = use_ssl;
    this->use_peers = use_peers;

    Vector<String> headers;
    if(!auth_header.empty())
        headers.push_back(auth_header);

    ERR_FAIL_COND_MSG(_client.is_null(), "Client was null, connection not established");
    _client->connect_to_url(node_url, Vector<String>(), false, headers);
}

void NanoWatcher::_connected(String proto) {
    Dictionary request;
    request["topic"] = "confirmation";

    Dictionary options;
    request["action"] = "subscribe";
    options["accounts"] = watched_accounts;
    
    request["options"] = options;
    String data = JSON::print(request);

    _client->get_peer(1)->set_write_mode(WebSocketPeer::WRITE_MODE_BINARY);
    _client->get_peer(1)->put_packet(reinterpret_cast<const uint8_t *>(data.c_str()), data.size());
}

bool NanoWatcher::is_websocket_connected() { return _client->get_connection_status() == WebSocketClient::CONNECTION_CONNECTED; }

void NanoWatcher::add_watched_account(Ref<NanoAccount> account) {
    Array add_accounts;
    add_accounts.push_back(account);
    update_watched_accounts(add_accounts);
}

void NanoWatcher::update_watched_accounts(Array accounts_add, Array accounts_del) {
    for(int i = 0; i < accounts_del.size(); i++){
        watched_accounts.erase(accounts_del[i]);
    }

    watched_accounts.append_array(accounts_add);

    if(is_websocket_connected()) {
        Dictionary request;
        request["topic"] = "confirmation";

        Dictionary options;
        if(subscribed){
            request["action"] = "update";
            options["accounts_add"] = accounts_add;
            options["accounts_del"] = accounts_del;
        } else {
            request["action"] = "subscribe";
            options["accounts"] = watched_accounts;
        }
        request["options"] = options;
        String data = JSON::print(request);

        _client->get_peer(1)->set_write_mode(WebSocketPeer::WRITE_MODE_BINARY);
        _client->get_peer(1)->put_packet(reinterpret_cast<const uint8_t *>(data.c_str()), data.size());
    }
}

void NanoWatcher::_closed(bool was_clean) {
    emit_signal("disconnected", was_clean);
}

void NanoWatcher::_on_data() {
    const uint8_t * data;
    int buffer_size;
    _client->get_peer(1)->get_packet(&data, buffer_size);

    String packet;
    packet.parse_utf8(reinterpret_cast<const char *>(data), buffer_size);

    Variant json_result;
    String err_string;
    int err_line;
    Error json_error = JSON::parse(packet, json_result, err_string, err_line);
    ERR_FAIL_COND_MSG(json_error, "JSON Parsing failed at line " + itos(err_line) + " with message: " + err_string);
    Dictionary *json_ptr = Object::cast_to<Dictionary>(json_result);
    ERR_FAIL_COND_MSG(json_ptr == NULL, "JSON response not in expected format: " + packet);

    Dictionary message = json_ptr->get("message", Dictionary());
    Ref<NanoAccount> account = lookup_watched_account(message.get("account", ""));
    Dictionary block = message.get("block", "");
    Ref<NanoAccount> link = lookup_watched_account(block.get("link_as_account", ""));
    ERR_FAIL_COND_MSG(account == NULL && link == NULL, "Received notification for non-watched account");
    String subtype = block.get("subtype", "");
    if(auto_receive && subtype == "send" && link != NULL && !link->get_private_key().empty()) {
        NanoReceiver * receiver = get_free_receiver();
        Ref<NanoAmount> amount(memnew(NanoAmount));
        amount->set_amount(message.get("amount", ""));
        receiver->receive(link, message.get("hash", ""), amount);
    } else {
        emit_signal("confirmation_received", *json_ptr);
    }
}

void NanoWatcher::_process(float delta) {
    _client->poll();
}

void NanoWatcher::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_process", "delta"), &NanoWatcher::_process);

    ClassDB::bind_method(D_METHOD("initialize_and_connect", "node_url", "default_representative", "auth_header", "use_ssl", "work_url", "use_peers"),
        &NanoWatcher::initialize_and_connect, DEFVAL(false), DEFVAL(""), DEFVAL(true), DEFVAL(""));
    ClassDB::bind_method(D_METHOD("add_watched_account", "account"), &NanoWatcher::add_watched_account);
    ClassDB::bind_method(D_METHOD("update_watched_accounts", "accounts_add", "accounts_del"), &NanoWatcher::update_watched_accounts, DEFVAL(Array()));

    ClassDB::bind_method(D_METHOD("set_auto_receive", "auto_receive"), &NanoWatcher::set_auto_receive);
    ClassDB::bind_method(D_METHOD("get_auto_receive"), &NanoWatcher::get_auto_receive);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_receive", PROPERTY_HINT_PROPERTY_OF_BASE_TYPE, ""), "set_auto_receive", "get_auto_receive");

    ClassDB::bind_method(D_METHOD("is_websocket_connected"), &NanoWatcher::is_websocket_connected);

    ADD_SIGNAL(MethodInfo("nano_receive_completed", PropertyInfo(Variant::OBJECT, "account"), PropertyInfo(Variant::STRING, "message"), PropertyInfo(Variant::INT, "response_code")));
    ADD_SIGNAL(MethodInfo("confirmation_received", PropertyInfo(Variant::DICTIONARY, "json")));
}