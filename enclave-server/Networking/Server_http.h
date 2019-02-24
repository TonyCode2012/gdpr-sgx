#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>
#include <boost/bind.hpp>

#include "../GeneralSettings.h"
#include "LogBase.h"
#include "UtilityFunctions.h"

using boost::asio::ip::tcp;
using namespace std;

typedef function<vector<string>(unsigned char*, int)> CallbackHandler;

class Session_http
  : public std::enable_shared_from_this<Session_http>
{
public:
    Session_http(boost::asio::ip::tcp::socket socket)
      : socket_(std::move(socket))
    {
    }

    void start();
    void setCallbackHandler(CallbackHandler cb);
    boost::asio::ip::tcp::socket socket_;

private:
    void do_read();
    void do_write(std::size_t length);

    CallbackHandler callback_handler = NULL;
    enum { max_length = 1024 };
    unsigned char data_[max_length];
};

class Server_http
{
public:
    Server_http(boost::asio::io_service& io_service, short port = Settings::rh_port)
      : acceptor_(io_service, boost::asio::ip::tcp::endpoint(boost::asio::ip::tcp::v4(), port)),
        socket_(io_service)
    { }

    void start_accept();
    void handle_accept(Session_http* new_session, const boost::system::error_code& error);
    void connectCallbackHandler(CallbackHandler cb);
    boost::asio::ip::tcp::socket socket_;

private:

    boost::asio::ip::tcp::acceptor acceptor_;
    CallbackHandler callback_handler;
};
