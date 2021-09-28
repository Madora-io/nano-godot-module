#include "receiver.h"

#include "core/bind/core_bind.h"
#include "core/method_bind_ext.gen.inc"
#include "core/io/json.h"

NanoReceiver::NanoReceiver() {
    state = READY;
    requester = memnew(NanoRequest);
    add_child(requester);
    requester->connect("request_completed", this, "_nano_request_completed");
}

void NanoReceiver::cancel_receive_request(String error_message, int error_code) {
    state = READY;
    emit_signal("nano_receive_completed", requester->get_account(), error_message, error_code);
    ERR_FAIL_MSG(error_message);
}

void NanoReceiver::_nano_request_completed(int p_status, int p_code, const PoolStringArray &headers, const PoolByteArray &p_data) {
    if(p_status) return cancel_receive_request("Could not communicate with node, see Result error.", p_status);
    
    String json_string;
    json_string.parse_utf8((const char *) p_data.read().ptr(), p_data.size());

    Variant json_result;
    String err_string;
    int err_line;
    Error json_error = JSON::parse(json_string, json_result, err_string, err_line);
    Dictionary json;
    if(json_error) json["error"] = json_string;
    else json = json_result;

    switch (state.load())
    {
    case ACCOUNT:
    {
        String error = json.get("error", "");

        if(error.empty()){ // This means the account already exists and has transactions
            String previous = json.get("frontier", "");
            String representative = json.get("representative", "");
            String current_balance = json.get("balance", "");
            Ref<NanoAmount> balance(memnew(NanoAmount));
            balance->set_amount(current_balance);
            balance->add(sending_amount);

            if(previous.empty() || representative.empty() || sending_amount.is_null()) return cancel_receive_request("Unexpected account state", 1);
            Ref<NanoAccount> rep(memnew(NanoAccount));
            rep->set_address(representative);
            
            block = requester->block_create(previous, rep, balance, linked_send_block);
            state = WORK;
            requester->work_generate(previous, use_peers, "fffffe0000000000");
        } else { // This account hasn't been opened, so this must be the first receive
            if(error != "Account not found") return cancel_receive_request("JSON Parsing failed at line " + itos(err_line) + " with message: " + err_string, json_error);
            block = requester->block_create("0", default_rep, sending_amount, linked_send_block);
            state = WORK;
            requester->work_generate(requester->get_account()->get_public_key(), use_peers, "fffffe0000000000");
        }
        break;
    }
    case WORK:
    {
        String error = json.get("error", "");
        if(!error.empty()) return cancel_receive_request("Error on work generation: " + error, 1);

        String work = json.get("work", "");
        String hash = json.get("hash", "");
        String current_hash = block["hash"];

        Dictionary subblock = block["block"];
        subblock["work"] = work;

        state = PROCESS;
        String subtype;
        if(subblock.get("previous", "0") == "0") subtype = "open";
        else subtype = "receive";
        requester->process(subtype, subblock);
        break;
    }
    case PROCESS:
    {
        String error = json.get("error", "");
        if(!error.empty()) return cancel_receive_request("Error on process call: " + error, 1);
        String hash = json.get("hash", "");
        state = READY;
        emit_signal("nano_receive_completed", requester->get_account(), hash, 0);
        break;
    }
    default:
        ERR_FAIL_MSG("Unexpected State on return from Nano Receiver for response " + json_string);
        break;
    }
}

void NanoReceiver::set_connection_parameters(String node_url, Ref<NanoAccount> default_representative, String auth_header, bool use_ssl, String work_url, bool use_peers) {
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

    requester->set_connection_parameters(url, auth, use_ssl, w_url);
    requester->set_account(receiver);

    this->linked_send_block = linked_send_block;
    this->sending_amount = amount;

    requester->account_info();
}

void NanoReceiver::_bind_methods() {
    ClassDB::bind_method(D_METHOD("is_ready"), &NanoReceiver::is_ready);
    ClassDB::bind_method(D_METHOD("receive", "receiver", "linked_send_block", "amount", "url"), &NanoReceiver::receive, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("set_connection_parameters", "node_url", "default_representative", "auth_header", "use_ssl", "work_url", "use_peers"), &NanoReceiver::set_connection_parameters, DEFVAL(false), DEFVAL(""), DEFVAL(true), DEFVAL(""));

    ClassDB::bind_method(D_METHOD("_nano_request_completed", "p_status", "p_code", "headers", "p_data"), &NanoReceiver::_nano_request_completed);
    ADD_SIGNAL(MethodInfo("nano_receive_completed", PropertyInfo(Variant::OBJECT, "account"), PropertyInfo(Variant::STRING, "message"), PropertyInfo(Variant::INT, "response_code")));
}