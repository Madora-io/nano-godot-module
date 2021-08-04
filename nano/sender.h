#ifndef NANO_SENDER_H_
#define NANO_SENDER_H_

#include "account.h"
#include "amount.h"
#include "requester.h"

#include "scene/main/node.h"

#include <atomic>

enum NanoSendingState { READY, ACCOUNT, WORK, PROCESS };

class NanoSender : public Node {
    GDCLASS(NanoSender, Node)

    private:
        std::atomic<NanoSendingState> state;
        NanoRequest requester;
        Ref<NanoAmount> sending_amount;
        Ref<NanoAccount> destination;
        Dictionary block;

        String node_url;
        String work_url;
        String auth;
        bool use_ssl;
        bool use_peers;

        void cancel_send_request(String error_message, int error_code);
        void _nano_send_completed(int p_status, int p_code, const PoolStringArray &headers, const PoolByteArray &p_data);

    protected:
        static void _bind_methods();
    public:
        void _init();
        void set_connection_parameters(String node_url, String auth_header = "", bool use_ssl = true, String work_url = "", bool use_peers = false);

        void send(Ref<NanoAccount> sender, Ref<NanoAccount> destination, Ref<NanoAmount> amount, String override_url = "");

};

#endif