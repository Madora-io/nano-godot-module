#ifndef NANO_SENDER_H_
#define NANO_SENDER_H_

#include "account.h"
#include "amount.h"
#include "requester.h"

#include "scene/main/node.h"

#include <atomic>

class NanoSender : public Node {
    GDCLASS(NanoSender, Node)

    private:
        std::atomic<NanoProcessorState> state;
        NanoRequest * requester;
        Ref<NanoAmount> sending_amount;
        Ref<NanoAccount> destination;
        Dictionary block;

        String node_url;
        String work_url;
        String auth;
        bool use_ssl;
        bool use_peers;

        void cancel_send_request(String error_message, int error_code);

    protected:
        static void _bind_methods();
    public:
        void set_connection_parameters(String node_url, String auth_header = "", bool use_ssl = true, String work_url = "", bool use_peers = false);
        void _nano_send_completed(int p_status, int p_code, const PoolStringArray &headers, const PoolByteArray &p_data);

        void send(Ref<NanoAccount> sender, Ref<NanoAccount> destination, Ref<NanoAmount> amount, String override_url = "");
        bool is_ready() { return state.load() == READY; }

        NanoSender();
};

#endif