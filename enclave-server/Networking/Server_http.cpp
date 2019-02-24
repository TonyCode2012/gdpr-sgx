#include <cstdlib>
#include <iostream>
#include <memory>
#include <utility>
#include <boost/asio.hpp>

#include "Server_http.h"
#include "LogBase.h"

using boost::asio::ip::tcp;
using namespace util;

/*************** Session http ***************/
void Session_http::start()
{
  do_read();
}

void Session_http::do_read()
{
  auto self(shared_from_this());
  socket_.async_read_some(boost::asio::buffer(data_, max_length),
      [this, self](boost::system::error_code ec, std::size_t length)
      {
        Log("\tlength:%d",length);
        Log("\tdata_:%s",ByteArrayToString(data_,max_length));
        auto msg = this->callback_handler(data_, 8);
        //if (!ec)
        //{
        //  do_write(length);
        //}
      });
}

void Session_http::do_write(std::size_t length)
{
  auto self(shared_from_this());
  boost::asio::async_write(socket_, boost::asio::buffer(data_, length),
      [this, self](boost::system::error_code ec, std::size_t /*length*/)
      {
        if (!ec)
        {
          do_read();
        }
      });
}

void Session_http::setCallbackHandler(CallbackHandler cb) {
    this->callback_handler = cb;
}


/*************** Server http ***************/
void Server_http::start_accept()
{
    acceptor_.async_accept(socket_, [this](boost::system::error_code ec){
        if(!ec) {
            auto new_session = std::make_shared<Session_http>(std::move(socket_));
            new_session->setCallbackHandler(this->callback_handler);
            new_session->start();
        }
        start_accept();
    });
    /*
    Session_http *new_session = new Session_http(socket_);
    new_session->setCallbackHandler(this->callback_handler);
    acceptor_.async_accept(new_session->socket_, boost::bind(&Server_http::handle_accept, this, new_session, boost::asio::placeholders::error));
    */
}

void Server_http::handle_accept(Session_http* new_session, const boost::system::error_code& error) {
    if (!error) {
        Log("New accept request, starting new session");
        new_session->start();
    } else {
        delete new_session;
    }

    start_accept();
}

void Server_http::connectCallbackHandler(CallbackHandler cb) {
    this->callback_handler = cb;
}
