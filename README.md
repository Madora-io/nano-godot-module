# Godot Nano Module
A Godot module to simplify use of Nano currency with the game engine. This module provides easy classes for managing Nano accounts (seeds, private keys, public keys, and addresses), amounts of Nano (raw or nano, with conversions between them), and making requests to a Nano Node (both RPC and WebSocket). It has convenience classes and functions for sending and receiving Nano, along with tracking confirmations and auto-receiving amounts sent to tracked accounts.

# Getting Started
1. This is a C++ Godot Module, which means it requires recompiling the engine. Learn more about how to do this with the Godot documentation: https://docs.godotengine.org/en/stable/development/compiling/index.html
2. Once you can compile the engine, the next step is to include this module in the compilation. Simply clone this repository into your godot/modules directory, and recompile. You will also need to rename the folder to "nano", it will not work otherwise!
3. You are now ready to begin using the module. Make sure to start Godot using your compiled binaries. To check that everything is working correctly, ensure that documentation is available for classes such as NanoAccount, NanoRequest, etc.

For an example of how this module can be used, please check out a demo wallet written entirely in Godot here: https://github.com/Madora-io/nano-godot-demo.

# Features

## NanoAccount
NanoAccount is a helper class that holds information for interacting with seeds, private keys, public keys, and addresses. It allows generating new seeds, generating qr codes for an account, local block signing, and block hashing. This is used by the other Nano classes to help interact with the Nano network. Importantly, this class allows for local block signing, which means that games created with this module can hold Nano non-custodially, without ever sending a private key off of the user's device. For more information about managing accounts see https://docs.nano.org/integration-guides/the-basics/#account-key-seed-and-wallet-ids

## NanoAmount
Used to deal with the large sizes for raw amounts of Nano. Get and set functions always deal with a string representing the raw amount. The functions `get_nano_amount` and `set_nano_amount` can be used to get and set with nano amounts (10^30 raw).

## NanoRequest
This class functions similarly to the Godot class HTTPRequest (including using the same signals), with convenience functions for interacting with the Nano network. You must use `set_connection_parameters` to initialize the requester before any calls can be made. Additionally, if the requests involve an account (all inbuilt requests require this) the `set_account` function is required. This class also has a convenience function for sending any Nano RPC call, in addition to the build in helper functions.

## NanoSender
This class encapsulates the RPC calls account_info, block_create, work_generate, and process into one method and signal, to reduce complexity for sending nano.

## NanoReceiver
This class encapsulates the RPC calls account_info, block_create, work_generate, and process into one method and signal. Generally the flow will be a pending RPC call to gather pending transactions, followed by a call to receive with the information from the pending call. This class is also used internally by `NanoWatcher` to automatically create receive blocks. This class works for both existing accounts, and creating new ones, with no need to specify which.

## NanoWatcher
NanoWatcher uses a websocket connection to be notified of newly confirmed blocks on the network. It also allows automatic receives for watched accounts. For more information see https://docs.nano.org/integration-guides/websockets/.

