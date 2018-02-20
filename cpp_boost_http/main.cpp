//
// Copyright (c) 2016-2017 Vinnie Falco (vinnie dot falco at gmail dot com)
//
// Distributed under the Boost Software License, Version 1.0. (See accompanying
// file LICENSE_1_0.txt or copy at http://www.boost.org/LICENSE_1_0.txt)
//
// Official repository: https://github.com/boostorg/beast
//

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <thread>
#include <vector>

#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/strand.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>
#include <boost/beast/version.hpp>
#include <boost/config.hpp>

using tcp = boost::asio::ip::tcp;    // from <boost/asio/ip/tcp.hpp>
namespace http = boost::beast::http; // from <boost/beast/http.hpp>

// This function produces an HTTP response for the given
// request. The type of the response object depends on the
// contents of the request, so the interface requires the
// caller to pass a generic lambda for receiving the response.
template <class Body, class Allocator, class Send>
void handle_request(http::request<Body, http::basic_fields<Allocator>>&& req, Send&& send) {
  // Returns a bad request response
  auto const bad_request = [&req](boost::beast::string_view why) {
    http::response<http::string_body> res{http::status::bad_request, req.version()};
    res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
    res.set(http::field::content_type, "text/html");
    res.keep_alive(req.keep_alive());
    res.body() = why.to_string();
    res.prepare_payload();
    return res;
  };

  // Make sure we can handle the method
  if(req.method() != http::verb::get) return send(bad_request("Unknown HTTP-method"));

  // Request path must be absolute and not contain "..".
  if(req.target().empty() || req.target()[0] != '/' ||
     req.target().find("..") != boost::beast::string_view::npos)
    return send(bad_request("Illegal request-target"));

  http::response<http::string_body> res{http::status::ok, req.version()};
  res.body() = "hello";
  res.set(http::field::server, BOOST_BEAST_VERSION_STRING);
  res.set(http::field::content_type, "text/plain");
  res.prepare_payload();
  res.keep_alive(req.keep_alive());
  return send(std::move(res));
}

//------------------------------------------------------------------------------

// Report a failure
void fail(boost::system::error_code ec, char const* what) {
  std::cerr << what << ": " << ec.message() << "\n";
}

// Handles an HTTP server connection
class session : public std::enable_shared_from_this<session> {
  // This is the C++11 equivalent of a generic lambda.
  // The function object is used to send an HTTP message.
  struct send_lambda {
    session& self_;

    explicit send_lambda(session& self) : self_(self) {}

    template <bool isRequest, class Body, class Fields>
    void operator()(http::message<isRequest, Body, Fields>&& msg) const {
      // The lifetime of the message has to extend
      // for the duration of the async operation so
      // we use a shared_ptr to manage it.
      auto sp = std::make_shared<http::message<isRequest, Body, Fields>>(std::move(msg));

      // Store a type-erased version of the shared
      // pointer in the class to keep it alive.
      self_.res_ = sp;

      // Write the response
      http::async_write(self_.socket_,
                        *sp,
                        self_.strand_.wrap(std::bind(&session::on_write,
                                                     self_.shared_from_this(),
                                                     std::placeholders::_1,
                                                     std::placeholders::_2)));
    }
  };

  tcp::socket socket_;
  boost::asio::io_service::strand strand_;
  boost::beast::flat_buffer buffer_;
  http::request<http::string_body> req_;
  std::shared_ptr<void> res_;
  send_lambda lambda_;

public:
  // Take ownership of the socket
  explicit session(tcp::socket socket)
      : socket_(std::move(socket)), strand_(socket_.get_io_service()), lambda_(*this) {}

  // Start the asynchronous operation
  void run() { do_read(); }

  void do_read() {
    // Read a request
    http::async_read(
        socket_,
        buffer_,
        req_,
        strand_.wrap(std::bind(
            &session::on_read, shared_from_this(), std::placeholders::_1, std::placeholders::_2)));
  }

  void on_read(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    // This means they closed the connection
    if(ec == http::error::end_of_stream) return do_close();

    if(ec) return fail(ec, "read");

    // Send the response
    handle_request(std::move(req_), lambda_);
  }

  void on_write(boost::system::error_code ec, std::size_t bytes_transferred) {
    boost::ignore_unused(bytes_transferred);

    if(ec == http::error::end_of_stream) {
      // This means we should close the connection, usually because
      // the response indicated the "Connection: close" semantic.
      return do_close();
    }

    if(ec) return fail(ec, "write");

    // We're done with the response so delete it
    res_ = nullptr;

    // Read another request
    do_read();
  }

  void do_close() {
    // Send a TCP shutdown
    boost::system::error_code ec;
    socket_.shutdown(tcp::socket::shutdown_send, ec);

    // At this point the connection is closed gracefully
  }
};

//------------------------------------------------------------------------------

// Accepts incoming connections and launches the sessions
class listener : public std::enable_shared_from_this<listener> {
  boost::asio::io_service::strand strand_;
  tcp::acceptor acceptor_;
  tcp::socket socket_;

public:
  listener(boost::asio::io_service& ios, tcp::endpoint endpoint)
      : strand_(ios), acceptor_(ios), socket_(ios) {
    boost::system::error_code ec;

    // Open the acceptor
    acceptor_.open(endpoint.protocol(), ec);
    if(ec) {
      fail(ec, "open");
      return;
    }

    // Bind to the server address
    acceptor_.bind(endpoint, ec);
    if(ec) {
      fail(ec, "bind");
      return;
    }

    // Start listening for connections
    acceptor_.listen(boost::asio::socket_base::max_connections, ec);
    if(ec) {
      fail(ec, "listen");
      return;
    }
  }

  // Start accepting incoming connections
  void run() {
    if(!acceptor_.is_open()) return;
    do_accept();
  }

  void do_accept() {
    acceptor_.async_accept(
        socket_, std::bind(&listener::on_accept, shared_from_this(), std::placeholders::_1));
  }

  void on_accept(boost::system::error_code ec) {
    if(ec) {
      fail(ec, "accept");
    } else {
      // Create the session and run it
      std::make_shared<session>(std::move(socket_))->run();
    }

    // Accept another connection
    do_accept();
  }
};

//------------------------------------------------------------------------------

int main(int argc, char* argv[]) {
  // Check command line arguments.
  if(argc != 4) {
    std::cerr << "Usage: http-server-async <address> <port> <threads>\n"
              << "Example:\n"
              << "    http-server-async 0.0.0.0 8080 1\n";
    return EXIT_FAILURE;
  }

  auto const address = boost::asio::ip::address::from_string(argv[1]);
  auto const port = static_cast<unsigned short>(std::atoi(argv[2]));
  auto const threads = std::max<std::size_t>(1, std::atoi(argv[3]));

  // The io_service is required for all I/O
  boost::asio::io_service ios{threads};

  // Create and launch a listening port
  std::make_shared<listener>(ios, tcp::endpoint{address, port})->run();

  // Run the I/O service on the requested number of threads
  std::vector<std::thread> v;
  v.reserve(threads - 1);
  for(auto i = threads - 1; i > 0; --i) v.emplace_back([&ios] { ios.run(); });
  ios.run();
}
