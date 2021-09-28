#include "watcher.h"

#include "core/io/json.h"
#include "core/method_bind_ext.gen.inc"

Array accountsToAddresses(Array accounts) {
    Array addresses;
    for(int i = 0; i < accounts.size(); i++) {
        Ref<NanoAccount> acc = accounts[i];
        addresses.append(acc->get_address());
    }
    return addresses;
}

NanoWatcher::NanoWatcher() {
    set_process(true);
    Ref<WebSocketClient> client(WebSocketClient::create());
    _client = client;
    ERR_FAIL_COND_MSG(_client.is_null(), "Client was null, connection not established");

    _client->connect("connection_closed", this, "_closed");
    _client->connect("connection_error", this, "_closed");
    _client->connect("connection_established", this, "_connected");

    _client->connect("data_received", this, "_on_data");

    timer = memnew(Timer);
    add_child(timer);
    timer->connect("timeout", this, "_on_timeout");
    timer->set_one_shot(false);
    timer->set_wait_time(30);
    timer->set_autostart(true);

    receiver = memnew(NanoReceiver);
    add_child(receiver);
    receiver->connect("nano_receive_completed", this, "_auto_receive_completed");
}

void NanoWatcher::_on_timeout() {
    Dictionary request;
    request["action"] = "ping";
    write_data(JSON::print(request));
    timer->start(30);
}

void NanoWatcher::_auto_receive_completed(Ref<NanoAccount> account, String message, int code) {
    process_next_receive();
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

Error NanoWatcher::initialize_and_connect(String websocket_url, Ref<NanoAccount> default_representative, String node_url, String auth_header, bool use_ssl, String work_url, bool use_peers) {
    print_line("Initializing Nano Watcher");
    this->websocket_url = websocket_url;
    this->node_url = node_url;
    this->default_rep = default_representative;
    if(work_url.empty()) this->work_url = node_url;
    else this->work_url = work_url;
    this->auth_header = auth_header;
    this->use_ssl = use_ssl;
    this->use_peers = use_peers;

    receiver->set_connection_parameters(node_url, default_rep, auth_header, use_ssl, this->work_url, use_peers);

    Vector<String> headers;
    if(!auth_header.empty())
        headers.push_back(auth_header);
    
    print_line("Connecting Nano Watcher");
    return _client->connect_to_url(websocket_url, Vector<String>(), false, headers);
}

void NanoWatcher::write_data(String data) {
    print_line("Sending data: ");

    CharString charstr = data.utf8();
    PoolByteArray packet;
    size_t len = charstr.length();
    packet.resize(len);
    PoolByteArray::Write w = packet.write();
    copymem(w.ptr(), charstr.ptr(), len);
    w.release();

    _client->get_peer(1)->set_write_mode(WebSocketPeer::WRITE_MODE_TEXT);
    Error err = _client->get_peer(1)->put_packet(packet.read().ptr(), packet.size());
    if(err) print_error("Failed to set up subscription with websocket, error: " + itos(err));
    else print_line("Data submitted");
}

void NanoWatcher::_connected(String proto) {
    print_line("Nano Watcher Connected");

    if(!watched_accounts.empty()){
        Dictionary request;
        request["topic"] = "confirmation";

        Dictionary options;
        request["action"] = "subscribe";
        options["accounts"] = accountsToAddresses(watched_accounts);

        request["options"] = options;
        String data = JSON::print(request);
        write_data(data);
    }
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
            options["accounts_add"] = accountsToAddresses(accounts_add);
            options["accounts_del"] = accountsToAddresses(accounts_del);
        } else {
            request["action"] = "subscribe";
            options["accounts"] = accountsToAddresses(watched_accounts);
        }
        request["options"] = options;
        String data = JSON::print(request);
        write_data(data);
    }
}

void NanoWatcher::_closed(bool was_clean) {
    print_line("Nano Watcher connection closed");
    emit_signal("disconnected", was_clean);
}

void NanoWatcher::process_next_receive() {
    if(pending_receives.empty()) return;
    Dictionary b = pending_receives.pop_front();
    receiver->receive(b.get("account", memnew(NanoAccount)), b.get("hash", ""), b.get("amount", memnew(NanoAmount)));
}

void NanoWatcher::_on_data() {
    print_line("Data recived from Websocket connection");
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
    Dictionary json = json_result;
    print_line("Info from websocket: " + packet);
    String ack = json.get("ack", "");
    if(!ack.empty()) return; // This is just a keepalive response

    Dictionary message = json.get("message", Dictionary());
    Ref<NanoAccount> account = lookup_watched_account(message.get("account", ""));
    Dictionary block = message.get("block", "");
    Ref<NanoAccount> link = lookup_watched_account(block.get("link_as_account", ""));
    ERR_FAIL_COND_MSG(account == NULL && link == NULL, "Received notification for non-watched account");
    String subtype = block.get("subtype", "");
    if(auto_receive && subtype == "send" && link != NULL && !link->get_private_key().empty()) {
        Ref<NanoAmount> amount(memnew(NanoAmount));
        amount->set_amount(message.get("amount", ""));

        Dictionary info;
        info["account"] = link;
        info["hash"] = message.get("hash", "");
        info["amount"] = amount;
        pending_receives.append(info);
        if(receiver->is_ready() && pending_receives.size() == 1){
            process_next_receive();
        }
    } else {
        emit_signal("confirmation_received", json);
    }
}

void NanoWatcher::_notification(int what) {
    if(what == NOTIFICATION_PROCESS){
        _client->poll();
    }
}

void NanoWatcher::_bind_methods() {
    ClassDB::bind_method(D_METHOD("_notification", "what"), &NanoWatcher::_notification);
    ClassDB::bind_method(D_METHOD("_on_timeout"), &NanoWatcher::_on_timeout);
    ClassDB::bind_method(D_METHOD("_closed", "was_clean"), &NanoWatcher::_closed);
    ClassDB::bind_method(D_METHOD("_on_data"), &NanoWatcher::_on_data);
    ClassDB::bind_method(D_METHOD("_connected", "proto"), &NanoWatcher::_connected);
    ClassDB::bind_method(D_METHOD("_auto_receive_completed", "account", "message", "code"), &NanoWatcher::_auto_receive_completed);

    ClassDB::bind_method(D_METHOD("initialize_and_connect", "websocket_url", "default_representative", "node_url", "auth_header", "use_ssl", "work_url", "use_peers"),
        &NanoWatcher::initialize_and_connect, DEFVAL(false), DEFVAL(""), DEFVAL(true), DEFVAL(""));
    ClassDB::bind_method(D_METHOD("add_watched_account", "account"), &NanoWatcher::add_watched_account);
    ClassDB::bind_method(D_METHOD("update_watched_accounts", "accounts_add", "accounts_del"), &NanoWatcher::update_watched_accounts, DEFVAL(Array()));

    ClassDB::bind_method(D_METHOD("set_auto_receive", "auto_receive"), &NanoWatcher::set_auto_receive);
    ClassDB::bind_method(D_METHOD("get_auto_receive"), &NanoWatcher::get_auto_receive);
    ADD_PROPERTY(PropertyInfo(Variant::BOOL, "auto_receive", PROPERTY_HINT_PROPERTY_OF_BASE_TYPE, ""), "set_auto_receive", "get_auto_receive");

    ClassDB::bind_method(D_METHOD("is_websocket_connected"), &NanoWatcher::is_websocket_connected);

    ADD_SIGNAL(MethodInfo("nano_receive_completed", PropertyInfo(Variant::OBJECT, "account"), PropertyInfo(Variant::STRING, "message"), PropertyInfo(Variant::INT, "response_code")));
    ADD_SIGNAL(MethodInfo("confirmation_received", PropertyInfo(Variant::DICTIONARY, "json")));
    ADD_SIGNAL(MethodInfo("disconnected", PropertyInfo(Variant::BOOL, "was_clean")));
}