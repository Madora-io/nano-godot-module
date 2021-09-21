#include "sender.h"

#include "core/bind/core_bind.h"
#include "core/io/json.h"

NanoSender::NanoSender() {
    state = READY;
    requester = memnew(NanoRequest);
    add_child(requester);
    requester->connect("request_completed", this, "_nano_send_completed");
}

void NanoSender::cancel_send_request(String error_message, int error_code) {
    state = READY;
    emit_signal("nano_send_completed", requester->get_account(), error_message, error_code);
    ERR_FAIL_MSG(error_message);
}

void NanoSender::_nano_send_completed(int p_status, int p_code, const PoolStringArray &headers, const PoolByteArray &p_data) {
    if(p_status) return cancel_send_request("Could not communicate with node, see Result error.", p_status);
    
    String json_string;
    json_string.parse_utf8((const char *) p_data.read().ptr(), p_data.size());
    if(p_code < 200 || p_code >= 300) return cancel_send_request("Node url returned failed response, response body: " + json_string, p_code);

    Variant json_result;
    String err_string;
    int err_line;
    Error json_error = JSON::parse(json_string, json_result, err_string, err_line);
    if(json_error) return cancel_send_request("JSON Parsing failed at line " + itos(err_line) + " with message: " + err_string, json_error);
    Dictionary json = json_result;
    // if(json_ptr == NULL) cancel_send_request("JSON response not in expected format: " + json_string, 1);

    switch (state.load())
    {
    case ACCOUNT:
    {
        String previous = json.get("frontier", "");
        String representative = json.get("representative", "");
        String current_balance = json.get("balance", "");
        Ref<NanoAmount> balance;
        balance->set_amount(current_balance);
        balance->sub(sending_amount);

        if(previous.empty() || representative.empty() || sending_amount.is_null() || destination->get_public_key().empty()) return cancel_send_request("Unexpected account state", 1);

        block = requester->block_create(previous, representative, balance, destination->get_public_key());
        state = WORK;
        requester->work_generate(block["hash"], use_peers);
        break;
    }
    case WORK:
    {
        String work = json.get("work", "");
        String hash = json.get("hash", "");

        if(block["hash"] != hash) return cancel_send_request("Work hash does not match current block", 1);
        Dictionary subblock = block["block"];
        subblock["work"] = work;
        block["block"] = subblock;

        state = PROCESS;
        requester->process("send", block);
        break;
    }
    case PROCESS:
    {
        String hash = json.get("hash", "");
        state = READY;
        emit_signal("nano_send_completed", requester->get_account(), hash, 0);
        break;
    }
    default:
        ERR_FAIL_MSG("Unexpected State on return from Nano Sender");
        break;
    }
}

void NanoSender::set_connection_parameters(String node_url, String auth_header, bool use_ssl, String work_url, bool use_peers) {
    this->node_url = node_url;
    if(work_url.empty()) this->work_url = node_url;
    else this->work_url = work_url;
    this->auth = auth_header;
    this->use_ssl = use_ssl;
    this->use_peers = use_peers;
}

void NanoSender::send(Ref<NanoAccount> sender, Ref<NanoAccount> destination, Ref<NanoAmount> amount, String override_url) {
    ERR_FAIL_COND_MSG(sender->get_private_key().empty(), "Sender private key not set");
    ERR_FAIL_COND_MSG(destination->get_public_key().empty(), "Destination public key not set");
    ERR_FAIL_COND_MSG(amount->get_raw_amount().empty(), "Amount not set");

    String url = (override_url.empty()) ? node_url : override_url;
    String w_url = (work_url.empty()) ? override_url : work_url;
    ERR_FAIL_COND_MSG(url.empty(), "Url not set");
    ERR_FAIL_COND_MSG(w_url.empty(), "Work url not set");
    
    ERR_FAIL_COND_MSG(state, "Already in use, only one send can happen per Sender.");
    state = ACCOUNT;

    requester->set_connection_parameters(url, auth, use_ssl, w_url);
    requester->set_account(sender);

    this->destination = destination;
    this->sending_amount = amount;

    requester->account_info();
}

void NanoSender::_bind_methods() {
    ClassDB::bind_method(D_METHOD("is_ready"), &NanoSender::is_ready);
    ClassDB::bind_method(D_METHOD("send", "sender", "destination", "amount", "url"), &NanoSender::send, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("set_connection_parameters", "node_url", "auth_header", "use_ssl", "work_url", "use_peers"), &NanoSender::set_connection_parameters, DEFVAL(false), DEFVAL(""), DEFVAL(true), DEFVAL(""));

    ClassDB::bind_method(D_METHOD("_nano_send_completed", "p_status", "p_code", "headers", "p_data"), &NanoSender::_nano_send_completed);
    ADD_SIGNAL(MethodInfo("nano_send_completed", PropertyInfo(Variant::OBJECT, "account"), PropertyInfo(Variant::STRING, "message"), PropertyInfo(Variant::INT, "response_code")));
}