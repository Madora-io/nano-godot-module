#ifndef NANO_RECEIVER_H_
#define NANO_RECEIVER_H_

#include "account.h"
#include "amount.h"
#include "requester.h"

#include "scene/main/node.h"

#include <atomic>

class NanoReceiver : public Node {
    GDCLASS(NanoReceiver, Node)

    private:
        std::atomic<NanoProcessorState> state;
        NanoRequest * requester;
        Ref<NanoAmount> sending_amount;
        String linked_send_block;
        Dictionary block;

        String node_url;
        Ref<NanoAccount> default_rep;
        String work_url;
        String auth;
        bool use_ssl;
        bool use_peers;

        void cancel_receive_request(String error_message, int error_code);

    protected:
        static void _bind_methods();
    public:
        void _nano_request_completed(int p_status, int p_code, const PoolStringArray &headers, const PoolByteArray &p_data);

        void set_connection_parameters(String node_url, Ref<NanoAccount> default_representative, String auth_header = "", bool use_ssl = true, String work_url = "", bool use_peers = false);

        void receive(Ref<NanoAccount> receiver, String linked_send_block, Ref<NanoAmount> amount, String override_url = "");
        bool is_ready() { return state.load() == READY; }

        NanoReceiver();
};

#endif