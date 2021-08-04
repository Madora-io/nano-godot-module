#include "requester.h"

#include "core/crypto/crypto_core.h"
#include "core/io/json.h"

void NanoRequest::set_account(Ref<NanoAccount> a) {
    this->account = a;
}

void NanoRequest::set_connection_parameters(String node_url, String auth_header, bool use_ssl, String work_url) {
    this->node_url = node_url;
    if(work_url.empty()) this->work_url = node_url;
    else this->work_url = work_url;
    this->auth = auth_header;
    this->use_ssl = use_ssl;
}

String NanoRequest::basic_auth_header(String username, String password) {
    String basic = username + ":" + password;
    CharString cstr = basic.utf8();
	String ret = CryptoCore::b64_encode_str((unsigned char *)cstr.get_data(), cstr.length());
	ERR_FAIL_COND_V(ret == "", ret);
	return "Authorization: Basic " + ret;
}

Vector<String> NanoRequest::get_common_headers() {
    Vector<String> headers;
    if(!auth.empty())
        headers.push_back(auth);

    headers.push_back("Content-Type: application/json");

    return headers;
}

Error NanoRequest::nano_request(Dictionary body, bool is_work) {
    String url = (is_work) ? work_url : node_url;
    if(url.empty()) return Error::ERR_UNCONFIGURED;

    String action = body["action"];
    if(!action.empty()) return ERR_INVALID_PARAMETER;

    String data = JSON::print(body);
    Error r = request(url, get_common_headers(), use_ssl, HTTPClient::METHOD_POST, data);

    if(r) return r;
    current_action = action;
    last_reply.clear();

    return r;
}

Error NanoRequest::account_balance() {
    Dictionary data;
    data["action"] = "account_balance";
    data["account"] = account->get_address();
    return nano_request(data);
}

Error NanoRequest::account_info(bool include_confirmed) {
    Dictionary data;
    data["action"] = "account_info";
    data["account"] = account->get_address();
    data["include_confirmed"] = include_confirmed;
    data["representative"] = true;

    return nano_request(data);
}

Dictionary NanoRequest::block_create(String previous, String representative, Ref<NanoAmount> balance, String link, String work) {
    String signature = account->sign(previous, representative, balance->get_raw_amount(), link);

    Dictionary block;
    block["type"] = "state";
    block["account"] = account->get_address();
    block["previous"] = previous;
    block["representative"] = representative;
    block["balance"] = balance->get_raw_amount();
    block["link"] = link;
    block["signature"] = signature;
    if(!work.empty()) block["work"] = work;

    Dictionary dict;
    dict["block"] = block;
    dict["hash"] = account->block_hash(previous, representative, balance->get_raw_amount(), link);
    return dict;
}

Error NanoRequest::pending(int count, String threshold) {
    Dictionary data;
    data["action"] = "pending";
    data["account"] = account->get_address();
    if(count) data["count"] = count;
    if(!threshold.empty()) {
        // Validate amount is in proper format
        NanoAmount amount;
        amount.set_amount(threshold);
        data["threshold"] = threshold;
    }

    return nano_request(data);
}

Error NanoRequest::process(String subtype, Dictionary block) {
    Dictionary data;
    data["action"] = "process";
    data["json_block"] = true;
    data["subtype"] = subtype;
    data["block"] = block;

    return nano_request(data);
}

Error NanoRequest::work_generate(String hash, bool use_peers) {
    Dictionary data;
    data["action"] = "work_generate";
    data["hash"] = hash;
    return nano_request(data);
}

void NanoRequest::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_account", "account"), &NanoRequest::set_account);
    ClassDB::bind_method(D_METHOD("set_connection_parameters", "node_url", "use_ssl", "auth_header", "work_url"), &NanoRequest::set_connection_parameters, DEFVAL(""), DEFVAL(""),  DEFVAL(true));
    ClassDB::bind_method(D_METHOD("basic_auth_header", "username", "password"), &NanoRequest::basic_auth_header);

    ClassDB::bind_method(D_METHOD("account_balance"), &NanoRequest::account_balance);
    ClassDB::bind_method(D_METHOD("account_info", "include_confirmed"), &NanoRequest::account_info, DEFVAL(true));
    ClassDB::bind_method(D_METHOD("block_create", "previous", "representative", "balance", "link", "work"), &NanoRequest::block_create, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("pending", "count", "threshold"), &NanoRequest::pending, DEFVAL(0), DEFVAL(""));
    ClassDB::bind_method(D_METHOD("process", "subtype", "block"), &NanoRequest::process);
    ClassDB::bind_method(D_METHOD("work_generate", "hash", "use_peers"), &NanoRequest::work_generate);
}