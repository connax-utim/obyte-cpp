//
// Created by tl on 18.03.19.
//

#ifndef OBYTE_CPP_WEBSOCKETCLIENT_HPP
#define OBYTE_CPP_WEBSOCKETCLIENT_HPP

#include "root_certificates.hpp"

#include <boost/beast/core.hpp>
#include <boost/beast/ssl.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>
#include <boost/asio/strand.hpp>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <memory>
#include <string>
#include <queue>

namespace beast = boost::beast;         // from <boost/beast.hpp>
namespace http = beast::http;           // from <boost/beast/http.hpp>
namespace websocket = beast::websocket; // from <boost/beast/websocket.hpp>
namespace net = boost::asio;            // from <boost/asio.hpp>
namespace ssl = boost::asio::ssl;       // from <boost/asio/ssl.hpp>
using tcp = boost::asio::ip::tcp;       // from <boost/asio/ip/tcp.hpp>

#ifndef WS_VARIABLES
#define WS_VARIABLES

class session : public std::enable_shared_from_this<session>
{
    tcp::resolver resolver_;
    websocket::stream<
            beast::ssl_stream<beast::tcp_stream>> ws_;
    beast::flat_buffer buffer_;
    std::string host_;

public:
    // Resolver and socket require an io_context
    explicit session(net::io_context&, ssl::context&);

    // Start the asynchronous operation
    void run(const std::string&, const std::string&);
    void stop();

private:
    void on_resolve(beast::error_code, const tcp::resolver::results_type&);
    void on_connect(beast::error_code, const tcp::resolver::results_type::endpoint_type&);
    void on_ssl_handshake(beast::error_code);
    void on_handshake(beast::error_code);
    void on_close(beast::error_code);
    void on_read(beast::error_code, std::size_t);
    void LoopRead();
    void on_write(beast::error_code, std::size_t);
    void LoopWrite();
};
#endif

void fail(beast::error_code, char const*);
//net::io_context
void WSInitiate(net::io_context&, const std::string&, const int&);
void sendTXT(const std::string&);

#endif //OBYTE_CPP_WEBSOCKETCLIENT_HPP