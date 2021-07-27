#include "scene/main/http_request.h"
#include "account.h"

class NanoRequest : public HTTPRequest {
    GDCLASS(NanoRequest, HTTPRequest)

    private:
        Ref<NanoAccount> account;
        String url;
        String auth;

        Vector<String> get_common_headers();
        
    protected:
        static void _bind_methods();
    public:
        void set_account(Ref<NanoAccount> a);
        void set_connection_information(String node_url, String auth_header = "");
        String basic_auth_header(String username, String password);

        Error account_balance();

};