#include "receiver.h"

#include "core/bind/core_bind.h"
#include "core/method_bind_ext.gen.inc"
#include "core/io/json.h"

void NanoReceiver::_init() {
    state = READY;
    this->connect("request_completed", &requester, "_nano_receive_completed");
}

void NanoReceiver::cancel_receive_request(String error_message, int error_code) {
    state = READY;
    emit_signal("nano_receive_completed", requester.get_account(), error_code, error_message);
    ERR_FAIL_MSG(error_message);
}

void NanoReceiver::_nano_receive_completed(int p_status, int p_code, const PoolStringArray &headers, const PoolByteArray &p_data) {
    if(p_status) cancel_receive_request("Could not communicate with node, see Result error.", p_status);
    
    String json_string;
    json_string.parse_utf8((const char *) p_data.read().ptr(), p_data.size());
    if(p_code < 200 || p_code >= 300) cancel_receive_request("Node url returned failed response, response body: " + json_string, p_code);

    Variant json_result;
    String err_string;
    int err_line;
    Error json_error = JSON::parse(json_string, json_result, err_string, err_line);
    if(json_error) cancel_receive_request("JSON Parsing failed at line " + itos(err_line) + " with message: " + err_string, json_error);
    Dictionary *json_ptr = Object::cast_to<Dictionary>(json_result);
    if(json_ptr == NULL) cancel_receive_request("JSON response not in expected format: " + json_string, 1);

    switch (state.load())
    {
    case ACCOUNT:
    {
        String error = json_ptr->get("error", "");

        if(error.empty()){ // This means the account already exists and has transactions
            String previous = json_ptr->get("frontier", "");
            String representative = json_ptr->get("representative", "");
            String current_balance = json_ptr->get("balance", "");
            Ref<NanoAmount> balance;
            balance->set_amount(current_balance);
            balance->add(sending_amount);

            block = requester.block_create(previous, representative, balance, linked_send_block);
            state = WORK;
            requester.work_generate(block["hash"], use_peers);
        } else { // This account hasn't been opened, so this must be the first receive
            if(error != "Account not found") cancel_receive_request("Received unexpected error: " + error, 1);
            block = requester.block_create("0", default_rep, sending_amount, linked_send_block);
            state = WORK;
            requester.work_generate(block["hash"], use_peers);
        }
        break;
    }
    case WORK:
    {
        String work = json_ptr->get("work", "");
        String hash = json_ptr->get("hash", "");

        if(block["hash"] != hash) cancel_receive_request("Work hash does not match current block", 1);
        Dictionary subblock = block["block"];
        subblock["work"] = work;
        block["block"] = subblock;

        state = PROCESS;
        requester.process("send", block);
        break;
    }
    case PROCESS:
    {
        String hash = json_ptr->get("hash", "");
        state = READY;
        emit_signal("nano_receive_completed", requester.get_account(), hash, 0);
        break;
    }
    default:
        ERR_FAIL_MSG("Unexpected State on return from Nano Sender");
        break;
    }
}

void NanoReceiver::set_connection_parameters(String node_url, String default_representative, String auth_header, bool use_ssl, String work_url, bool use_peers) {
    this->node_url = node_url;
    this->default_rep = default_representative;
    if(work_url.empty()) this->work_url = node_url;
    else this->work_url = work_url;
    this->auth = auth_header;
    this->use_ssl = use_ssl;
    this->use_peers = use_peers;
}

void NanoReceiver::receive(Ref<NanoAccount> receiver, String linked_send_block, Ref<NanoAmount> amount, String override_url) {
    ERR_FAIL_COND_MSG(receiver->get_private_key().empty(), "Receiver private key not set");
    ERR_FAIL_COND_MSG(linked_send_block.empty(), "Linked send block not set");
    ERR_FAIL_COND_MSG(amount->get_raw_amount().empty(), "Amount not set");

    String url = (override_url.empty()) ? node_url : override_url;
    String w_url = (work_url.empty()) ? override_url : work_url;
    ERR_FAIL_COND_MSG(url.empty(), "Url not set");
    ERR_FAIL_COND_MSG(w_url.empty(), "Work url not set");
    
    ERR_FAIL_COND_MSG(state, "Already in use, only one send can happen per Receiver.");
    state = ACCOUNT;

    requester.set_connection_parameters(url, auth, use_ssl, w_url);
    requester.set_account(receiver);

    this->linked_send_block = linked_send_block;
    this->sending_amount = amount;

    requester.account_info();
}

void NanoReceiver::_bind_methods() {
    ClassDB::bind_method(D_METHOD("is_ready"), &NanoReceiver::is_ready);
    ClassDB::bind_method(D_METHOD("receive", "receiver", "linked_send_block", "amount", "url"), &NanoReceiver::receive, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("set_connection_parameters", "node_url", "default_representative", "auth_header", "use_ssl", "work_url", "use_peers"), &NanoReceiver::set_connection_parameters, DEFVAL(false), DEFVAL(""), DEFVAL(true), DEFVAL(""));

    ADD_SIGNAL(MethodInfo("nano_receive_completed", PropertyInfo(Variant::OBJECT, "account"), PropertyInfo(Variant::STRING, "message"), PropertyInfo(Variant::INT, "response_code")));
}