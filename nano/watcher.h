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
        Timer * timer;

        bool auto_receive = true;
        String websocket_url;
        String node_url;
        Ref<NanoAccount> default_rep;
        String auth_header;
        bool use_ssl;
        String work_url;
        bool use_peers;

        void _closed(bool was_clean = false);
        void _connected(String proto = "");
        void _on_data();

        void write_data(String data);

        Array pending_receives;
        NanoReceiver * receiver;
        void process_next_receive();

        Ref<NanoAccount> lookup_watched_account(String address);

    protected:
        static void _bind_methods();
    public:
        Error initialize_and_connect(String websocket_url, Ref<NanoAccount> default_representative, String node_url = "", String auth_header = "", bool use_ssl = true, String work_url = "", bool use_peers = false);
        void add_watched_account(Ref<NanoAccount> account);
        void update_watched_accounts(Array accounts_add, Array accounts_del = Array());

        void _notification(int what);
        void _on_timeout();
        void _auto_receive_completed(Ref<NanoAccount> account, String message, int code);

        void set_auto_receive(bool receive) { this->auto_receive = receive; }
        bool get_auto_receive() { return auto_receive; }

        bool is_websocket_connected();

        NanoWatcher();
};

#endif