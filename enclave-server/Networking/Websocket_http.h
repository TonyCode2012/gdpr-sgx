//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket server, asynchronous
//
//------------------------------------------------------------------------------

#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/asio/bind_executor.hpp>
#include <boost/asio/strand.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <algorithm>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include "../GeneralSettings.h"
#include "LogBase.h"
#include "UtilityFunctions.h"

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>
using namespace std;

//------------------------------------------------------------------------------

typedef function<vector<string>(unsigned char*, int)> CallbackHandler;

// Echoes back all received WebSocket messages
class Session_http : public std::enable_shared_from_this<Session_http>
{
    websocket::stream<tcp::socket> ws_;
    boost::asio::strand<
        boost::asio::io_context::executor_type> strand_;
    boost::beast::multi_buffer buffer_;

public:
    // Take ownership of the socket
    explicit Session_http(tcp::socket socket)
        : ws_(std::move(socket))
        , strand_(ws_.get_executor())
    {
    }

    // Start the asynchronous operation
    void run();

    void on_accept(boost::system::error_code ec);

    void do_read();

    void on_read(
        boost::system::error_code ec,
        std::size_t bytes_transferred);

    void on_write(
        boost::system::error_code ec,
        std::size_t bytes_transferred);

    void setCallbackHandler(CallbackHandler cb);

private:
    CallbackHandler callback_handler = NULL;
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the Session_https
class Server_http : public std::enable_shared_from_this<Server_http>
{
    tcp::acceptor acceptor_;
    tcp::socket socket_;

public:
    Server_http(
        boost::asio::io_context& ioc,
        tcp::endpoint endpoint);
    /*
    {
        boost::system::error_code ec;

        // Open the acceptor
        acceptor_.open(endpoint.protocol(), ec);
        if(ec)
        {
            boost_fail(ec, "open");
            return;
        }

        // Allow address reuse
        acceptor_.set_option(boost::asio::socket_base::reuse_address(true), ec);
        if(ec)
        {
            boost_fail(ec, "set_option");
            return;
        }

        // Bind to the server address
        acceptor_.bind(endpoint, ec);
        if(ec)
        {
            boost_fail(ec, "bind");
            return;
        }

        // Start listening for connections
        acceptor_.listen(
            boost::asio::socket_base::max_listen_connections, ec);
        if(ec)
        {
            boost_fail(ec, "listen");
            return;
        }
    }
    */

    // Start accepting incoming connections
    void run();

    void do_accept();

    void on_accept(boost::system::error_code ec);

    void connectCallbackHandler(CallbackHandler cb);

private:
    CallbackHandler callback_handler;
};
