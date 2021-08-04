#ifndef NANO_REQUESTER_H_
#define NANO_REQUESTER_H_

#include "scene/main/http_request.h"
#include "account.h"
#include "amount.h"

#include <atomic>

class NanoRequest : public HTTPRequest {
    GDCLASS(NanoRequest, HTTPRequest)

    private:
        Ref<NanoAccount> account;
        String node_url;
        String work_url;
        String auth;
        bool use_ssl;

        String current_action;
        Dictionary last_reply;

        Vector<String> get_common_headers();
        
    protected:
        static void _bind_methods();
    public:
        void set_account(Ref<NanoAccount> a);
        void set_connection_parameters(String node_url, String auth_header = "", bool use_ssl = true, String work_url = "");
        String basic_auth_header(String username, String password);

        Error nano_request(Dictionary body, bool is_work = false);

        Error account_balance();
        Error account_info(bool include_confirmed = true);
        Dictionary block_create(String previous, String representative, Ref<NanoAmount> balance, String link, String work = ""); // This does not make a request to node, but instead signs locally.
        Error pending(int count = 0, String threshold = "");
        Error process(String subtype, Dictionary block);
        // Error send(String destination_address, Ref<NanoAmount> amount); // Encapsulates account_info, block_create (local to avoid sending pk), and process into one call
        Error work_generate(String hash, bool use_peers = false);

};

#endif