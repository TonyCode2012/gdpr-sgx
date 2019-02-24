#include "Websocket_http.h"

using tcp = boost::asio::ip::tcp;               // from <boost/asio/ip/tcp.hpp>
namespace websocket = boost::beast::websocket;  // from <boost/beast/websocket.hpp>
using namespace std;
using namespace util;

//------------------------------------------------------------------------------


// Report a boost_failure
void boost_fail(boost::system::error_code ec, char const* what)
{
    std::cerr << what << ": " << ec.message() << "\n";
}

// Start the asynchronous operation
void Session_http::run()
{
    // Accept the websocket handshake
    ws_.async_accept(
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &Session_http::on_accept,
                shared_from_this(),
                std::placeholders::_1)));
}

void Session_http::on_accept(boost::system::error_code ec)
{
    if(ec)
        return boost_fail(ec, "accept");
    // Read a message
    do_read();
}

void Session_http::do_read()
{
    // Read a message into our buffer
    ws_.async_read(
        buffer_,
        boost::asio::bind_executor(
            strand_,
            std::bind(
                &Session_http::on_read,
                shared_from_this(),
                std::placeholders::_1,
                std::placeholders::_2)));
}

void Session_http::on_read(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    // This indicates that the Session_httpwas closed
    if(ec == websocket::error::closed)
        return;
    if(ec)
        boost_fail(ec, "read");
    // Echo the message
    std::ostringstream os;
    os << boost::beast::buffers(buffer_.data());
    string tmpstr = os.str();
    int tmpsize = tmpstr.size();
    unsigned char *pdata = (unsigned char*) malloc(tmpsize);
    memcpy(pdata,tmpstr.c_str(),tmpsize);
    auto msg = this->callback_handler(pdata,tmpsize);

    // send handle result to Client
    if(msg.size() != 0) {
        ws_.text(ws_.got_text());
        ws_.async_write(
            //buffer_.data(),
            boost::asio::buffer(msg[0].c_str(),msg[0].size()),
            boost::asio::bind_executor(
                strand_,
                std::bind(
                    &Session_http::on_write,
                    shared_from_this(),
                    std::placeholders::_1,
                    std::placeholders::_2)));
    } else {
        Log("No response from handler");
    }
}

void Session_http::on_write(
    boost::system::error_code ec,
    std::size_t bytes_transferred)
{
    boost::ignore_unused(bytes_transferred);
    if(ec)
        return boost_fail(ec, "write");
    // Clear the buffer
    buffer_.consume(buffer_.size());
    // Do another read
    do_read();
}

void Session_http::setCallbackHandler(CallbackHandler cb) {
    this->callback_handler = cb;
}

//------------------------------------------------------------------------------

Listener::Listener(
    boost::asio::io_context& ioc,
    tcp::endpoint endpoint)
    : acceptor_(ioc)
    , socket_(ioc)
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

// Start accepting incoming connections
void Listener::run()
{
    if(! acceptor_.is_open())
        return;
    do_accept();
}

void Listener::do_accept()
{
    acceptor_.async_accept(
        socket_,
        std::bind(
            &Listener::on_accept,
            shared_from_this(),
            std::placeholders::_1));
}

void Listener::on_accept(boost::system::error_code ec)
{
    if(ec)
    {
        boost_fail(ec, "accept");
    }
    else
    {
        // Create the Session_httpand run it
        auto new_session = std::make_shared<Session_http>(std::move(socket_));
        new_session->setCallbackHandler(this->callback_handler);
        new_session->run();
    }
    // Accept another connection
    do_accept();
}

void Listener::connectCallbackHandler(CallbackHandler cb) {
    this->callback_handler = cb;
}

//------------------------------------------------------------------------------

/*
int main(int argc, char* argv[])
{
    // Check command line arguments.
    if (argc != 4)
    {
        std::cerr <<
            "Usage: websocket-server-async <address> <port> <threads>\n" <<
            "Example:\n" <<
            "    websocket-server-async 0.0.0.0 8080 1\n";
        return EXIT_boost_failURE;
    }
    auto const address = boost::asio::ip::make_address(argv[1]);
    auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
    auto const threads = std::max<int>(1, std::atoi(argv[3]));

    // The io_context is required for all I/O
    boost::asio::io_context ioc{threads};

    // Create and launch a listening port
    std::make_shared<listener>(ioc, tcp::endpoint{address, port})->run();

    // Run the I/O service on the requested number of threads
    std::vector<std::thread> v;
    v.reserve(threads - 1);
    for(auto i = threads - 1; i > 0; --i)
        v.emplace_back(
        [&ioc]
        {
            ioc.run();
        });
    ioc.run();

    return EXIT_SUCCESS;
}
*/
