#ifndef NANO_WATCHER_H_
#define NANO_WATCHER_H_

#include "account.h"
#include "receiver.h"

#include "core/list.h"
#include "scene/main/node.h"
#include "modules/websocket/websocket_client.h"

class NanoWatcher : public Node {
    GDCLASS(NanoWatcher, Node);

    private:
        Ref<WebSocketClient> _client;

        Array watched_accounts;
        bool subscribed;

        bool auto_receive = true;
        String node_url;
        Ref<NanoAccount> default_rep;
        String auth_header;
        bool use_ssl;
        String work_url;
        bool use_peers;

        void _closed(bool was_clean = false);
        void _connected(String proto = "");
        void _on_data();
        void _auto_recieve_completed(Ref<NanoAccount> account, String message, int code);

        void send_accounts_to_node();

        Array receiver_pool;
        NanoReceiver * get_free_receiver();

        Ref<NanoAccount> lookup_watched_account(String address);

    protected:
        static void _bind_methods();
    public:
        void initialize_and_connect(String node_url, Ref<NanoAccount> default_representative, String auth_header = "", bool use_ssl = true, String work_url = "", bool use_peers = false);
        void add_watched_account(Ref<NanoAccount> account);
        void update_watched_accounts(Array accounts_add, Array accounts_del = Array());

        void _process(float delta);

        void set_auto_receive(bool receive) { this->auto_receive = receive; }
        bool get_auto_receive() { return auto_receive; }

        bool is_websocket_connected();

        NanoWatcher();
};

#endif