//
// Created by tl on 18.03.19.
//

//
// Copyright (c) 2016-2019 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

//------------------------------------------------------------------------------
//
// Example: WebSocket SSL client, asynchronous
//
//------------------------------------------------------------------------------


//------------------------------------------------------------------------------

#include "websocketclient.hpp"

bool accepting = true;
bool writing = false;
int num_read, num_sent;

std::queue <std::string> requests;

// Report a failure
void
fail(beast::error_code ec, char const *what) {
#ifdef DEBUG_PRINT
    std::clog << what << ": " << ec.message() << "\n";
#endif
}

// Sends a WebSocket message and prints the response
session::session(net::io_context &ioc, ssl::context &ctx)
        : resolver_(net::make_strand(ioc)), ws_(net::make_strand(ioc), ctx) {
}

// Start the asynchronous operation
void
session::run(
        const std::string& host,
        const std::string& port) {
    // Save these for later
    host_ = host;

    // Look up the domain name
    resolver_.async_resolve(
            host,
            port,
            beast::bind_front_handler(
                    &session::on_resolve,
                    shared_from_this()));
}

void
session::stop() {
    accepting = false;
}

void
sendTXT(const std::string &message_) {
    num_sent++;
    requests.push(message_);
#ifdef DEBUG_PRINT
    std::clog << "> Pushed " << num_sent << "-th request to the queue" << std::endl;
#endif
}

void
session::on_resolve(
        beast::error_code ec,
        const tcp::resolver::results_type &results) {
    if (ec)
        return fail(ec, "resolve");

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Make the connection on the IP address we get from a lookup
    beast::get_lowest_layer(ws_).async_connect(
            results,
            beast::bind_front_handler(
                    &session::on_connect,
                    shared_from_this()));
}

void
session::on_connect(beast::error_code ec, const tcp::resolver::results_type::endpoint_type &) {
    if (ec)
        return fail(ec, "connect");

    // Set a timeout on the operation
    beast::get_lowest_layer(ws_).expires_after(std::chrono::seconds(30));

    // Perform the SSL handshake
    ws_.next_layer().async_handshake(
            ssl::stream_base::client,
            beast::bind_front_handler(
                    &session::on_ssl_handshake,
                    shared_from_this()));
}

void
session::on_ssl_handshake(beast::error_code ec) {
    if (ec)
        return fail(ec, "ssl_handshake");

    // Turn off the timeout on the tcp_stream, because
    // the websocket stream has its own timeout system.
    beast::get_lowest_layer(ws_).expires_never();

    // Set suggested timeout settings for the websocket
    ws_.set_option(
            websocket::stream_base::timeout::suggested(
                    beast::role_type::client));

    // Set a decorator to change the User-Agent of the handshake
    ws_.set_option(websocket::stream_base::decorator(
            [](websocket::request_type &req) {
                req.set(http::field::user_agent,
                        std::string(BOOST_BEAST_VERSION_STRING) +
                        " websocket-client-async-ssl");
            }));

    // Perform the websocket handshake
    ws_.async_handshake(host_, "/",
                        beast::bind_front_handler(
                                &session::on_handshake,
                                shared_from_this()));
}

void
session::on_handshake(beast::error_code ec) {
    if (ec)
        return fail(ec, "handshake");

    session::LoopRead();
    requests.push("Initialising write");
    session::LoopWrite();

    ////// accept requests!!!!!
}

void
session::on_close(beast::error_code ec) {
    if (ec)
        return fail(ec, "close");

    // If we get here then the connection is closed gracefully

    // The make_printable() function helps print a ConstBufferSequence
#ifdef DEBUG_PRINT
    std::cout << "> Closed" << std::endl;
#endif
}

void
session::on_read(
        beast::error_code ec,
        std::size_t size
) {
#ifdef DEBUG_PRINT
    std::clog << "> End read" << std::endl;
#endif
    if (ec) {
#ifdef DEBUG_PRINT
        std::clog << "Error while reading" << std::endl;
        std::clog << ec.message() << std::endl;
#endif
        ws_.async_close(websocket::close_code::normal,
                        beast::bind_front_handler(
                                &session::on_close,
                                shared_from_this()));
        return;
    } else {
#ifdef DEBUG_PRINT
        std::clog << "> Received \"" << beast::make_printable(buffer_.data()) << "\"" << std::endl;
#endif
        buffer_.consume(size);
        num_read++;
    }
    // Enqueue another read.
    if (accepting || num_sent > num_read)
        session::LoopRead();
    else {
        // Close the WebSocket connection
        ws_.async_close(websocket::close_code::normal,
                        beast::bind_front_handler(
                                &session::on_close,
                                shared_from_this()));
    }
}

void
session::LoopRead() {
#ifdef DEBUG_PRINT
    std::clog << "> Read" << std::endl;
#endif
    ws_.async_read(
            buffer_,
            beast::bind_front_handler(
                    &session::on_read,
                    shared_from_this()));
}

void
session::on_write(
        beast::error_code ec,
        std::size_t size
) {
#ifdef DEBUG_PRINT
    std::clog << "> End write" << std::endl;
#endif
    if (ec) {
#ifdef DEBUG_PRINT
        std::clog << "Error while writing:" << std::endl;
        std::clog << ec.message() << std::endl;
#endif
    } else {
#ifdef DEBUG_PRINT
        std::clog << "Written \"" << message << "\"" << std::endl;
#endif
        writing = false;
        // Enqueue another write.
        session::LoopWrite();
    }
}

void
session::LoopWrite() {
    if (!accepting && requests.empty()) {
#ifdef DEBUG_PRINT
        std::clog << "Not accepting!" << std::endl;
#endif
        return;
    }

    std::string WSmessage;

    try {
        WSmessage = requests.front();
        requests.pop();
#ifdef DEBUG_PRINT
        std::clog << "> Write \"" << WSmessage << "\"" << std::endl;
#endif
    } catch (...) {
        WSmessage = "";
#ifdef DEBUG_PRINT
        std::clog << "> Write queue is empty" << std::endl;
#endif
    }

    writing = true;

    ws_.async_write(
            net::buffer(WSmessage),
            beast::bind_front_handler(
                    &session::on_write,
                    shared_from_this()));
}

net::io_context
WSInitiate(net::io_context &ioc, const std::string &host, const int &port) {
    const std::string s_port = std::to_string(port);

    // The SSL context is required, and holds certificates
    ssl::context ctx{ssl::context::tlsv12_client};

    // This holds the root certificate used for verification
    load_root_certificates(ctx);

    // Launch the asynchronous operation
    std::make_shared<session>(ioc, ctx)->run(host, s_port);

    // Run the I/O service. The call will return when
    // the socket is closed.
    ioc.run();
}
