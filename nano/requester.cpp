#include "requester.h"

#include "core/crypto/crypto_core.h"
#include "core/io/json.h"

void NanoRequest::set_account(Ref<NanoAccount> a) {
    this->account = a;
}

void NanoRequest::set_connection_information(String node_url, String auth_header) {
    this->url = node_url;
    this->auth = auth_header;
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

Error NanoRequest::account_balance() {
    Dictionary data;
    data["action"] = "account_balance";
    data["account"] = account->get_address();
    String payload = JSON::print(data);

    return request(url, get_common_headers(), false, HTTPClient::METHOD_POST, payload);
}

void NanoRequest::_bind_methods() {
    ClassDB::bind_method(D_METHOD("set_account", "account"), &NanoRequest::set_account);
    ClassDB::bind_method(D_METHOD("set_connection_information", "node_url", "auth_header"), &NanoRequest::set_connection_information, DEFVAL(""));
    ClassDB::bind_method(D_METHOD("basic_auth_header", "username", "password"), &NanoRequest::basic_auth_header);

    ClassDB::bind_method(D_METHOD("account_balance"), &NanoRequest::account_balance);
}