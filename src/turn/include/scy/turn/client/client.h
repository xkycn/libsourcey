///
//
// LibSourcey
// Copyright (c) 2005, Sourcey <http://sourcey.com>
//
// SPDX-License-Identifier:	LGPL-2.1+
//
/// @addtogroup turn
/// @{


#ifndef SCY_TURN_Client_H
#define SCY_TURN_Client_H


#include "scy/net/udpsocket.h"
#include "scy/stateful.h"
#include "scy/stun/transaction.h"
#include "scy/turn/fivetuple.h"
#include "scy/turn/iallocation.h"
#include "scy/turn/turn.h"
#include "scy/turn/turn.h"
#include "scy/turn/types.h"

#include <deque>


namespace scy {
namespace turn {


struct ClientState : public State
{
    enum Type
    {
        None = 0x00,
        Allocating = 0x02,
        Authorizing = 0x04,
        Success = 0x08,
        // Terminated      = 0x10,
        Failed = 0x10
    };

    std::string toString() const
    {
        switch (id()) {
            case None:
                return "None";
            case Allocating:
                return "Allocating";
            case Authorizing:
                return "Authorizing";
            // case Terminated:      return "Terminated";
            case Success:
                return "Success";

            case Failed:
                return "Failed";
        }
        return "undefined";
    };
};


class Client;


struct ClientObserver
{
    virtual void onClientStateChange(Client& client, ClientState& state,
                                     const ClientState& oldState) = 0;

    virtual void onRelayDataReceived(Client& client, const char* data,
                                     std::size_t size,
                                     const net::Address& peerAddress) = 0;

    virtual void onAllocationCreated(Client& client,
                                     const stun::Transaction& transaction){};
    virtual void onAllocationFailed(Client& client, int errorCode,
                                    const std::string& reason){};
    virtual void onAllocationDeleted(Client& client,
                                     const stun::Transaction& transaction){};
    virtual void
    onAllocationPermissionsCreated(Client& client,
                                   const PermissionList& permissions){};

    /// All received transaction responses will be routed here after local
    /// processing so the observer can easily implement extra functionality.
    virtual void onTransactionResponse(Client& client,
                                       const stun::Transaction& transaction){};

    /// Fires after the client's internal timer callback.
    /// Handy for performing extra async cleanup tasks.
    virtual void onTimer(Client& client){};
};


class Client : public Stateful<ClientState>, protected IAllocation
{
public:
    struct Options
    {
        std::string software;
        std::string username;
        // std::string realm;
        std::string password;

        long timeout;
        std::int64_t lifetime;
        std::int64_t timerInterval;
        net::Address serverAddr;
        Options()
        {
            software = "Sourcey STUN/TURN Client [rfc5766]";
            username = util::randomString(4);
            password = util::randomString(22);
            // realm                    = "sourcey.com";
            lifetime = 5 * 60 * 1000; // 5 minutes
            timeout = 10 * 1000;
            timerInterval = 30 * 1000; // 30 seconds
            serverAddr = net::Address("127.0.0.1", 3478);
        }
    };

public:
    Client(ClientObserver& observer,
           const Options& options = Options()); // net::Socket* socket,
    virtual ~Client();

    /// Initiates the allocation sequence.
    virtual void initiate();

    /// Shutdown the client and destroy the active allocation.
    virtual void shutdown();

    /// Sends the allocation request.
    virtual void sendAllocate();

    virtual void addPermission(const IPList& peerIPs);

    /// Peer permissions should be added/created before we kick
    /// off the allocation sequence, but may be added later.
    virtual void addPermission(const std::string& ip);

    /// Sends a CreatePermission request including all hosts
    // added via addPermission();

    /// A CreatePermission request will be sent as soon as the
    /// Allocation is created, and at timer x intervals.
    virtual void sendCreatePermission();

    virtual void sendChannelBind(const std::string& peerIP);
    virtual void sendRefresh();
    virtual void sendData(const char* data, std::size_t size,
                          const net::Address& peerAddress);

    virtual bool handleResponse(const stun::Message& response);
    virtual void handleAllocateResponse(const stun::Message& response);
    virtual void handleAllocateErrorResponse(const stun::Message& response);
    virtual void handleCreatePermissionResponse(const stun::Message& response);
    virtual void
    handleCreatePermissionErrorResponse(const stun::Message& response);
    virtual void handleRefreshResponse(const stun::Message& response);
    virtual void handleDataIndication(const stun::Message& response);

    virtual int transportProtocol();
    virtual stun::Transaction*
    createTransaction(const net::Socket::Ptr& socket = nullptr);
    virtual void authenticateRequest(stun::Message& request);
    virtual bool sendAuthenticatedTransaction(stun::Transaction* transaction);
    virtual bool removeTransaction(stun::Transaction* transaction);

    net::Address mappedAddress() const;
    net::Address relayedAddress() const;

    bool closed() const;

    ClientObserver& observer();
    Options& options();

protected:
    virtual void setError(const scy::Error& error);

    virtual void onSocketConnect(net::Socket& socket);
    virtual void onSocketRecv(net::Socket& socket, const MutableBuffer& buffer,
                              const net::Address& peerAddress);
    virtual void onSocketClose(net::Socket& socket);
    virtual void onTransactionProgress(void* sender, TransactionState& state,
                                       const TransactionState&);
    virtual void onStateChange(void* sender, ClientState& state,
                               const ClientState& oldState);
    virtual void onTimer();

protected:
    // mutable std::mutex _mutex;
    ClientObserver& _observer;
    Options _options;
    net::Socket::Ptr _socket;
    Timer _timer;
    scy::Error _error;
    net::Address _mappedAddress;
    net::Address _relayedAddress;

    std::string _realm;
    std::string _nonce;

    /// A list of queued Send indication packets awaiting server permissions
    std::deque<stun::Message> _pendingIndications;

    /// A list containing currently active transactions
    std::vector<stun::Transaction*> _transactions;
};
}
} //  namespace scy::turn


#endif // SCY_TURN_Client_H


/// @\}
