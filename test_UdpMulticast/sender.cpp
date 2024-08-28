#include <iostream>
#include <sstream>
#include <string>
#include "../libNet/asio/asio.hpp"

constexpr short multicast_port = 29000;
constexpr int max_message_count = 1000;

class sender
{
public:
    sender(asio::io_context& io_context,
        const asio::ip::address& multicast_address)
        : endpoint_(multicast_address, multicast_port),
        socket_(io_context, endpoint_.protocol()),
        timer_(io_context),
        message_count_(0)
    {
        /*auto endpoint = asio::ip::udp::endpoint(asio::ip::make_address("0.0.0.0"), 29000);
        socket_.open(endpoint.protocol());
        socket_.set_option(asio::ip::udp::socket::reuse_address(false));
        socket_.bind(endpoint);*/
        do_send();
    }

private:
    void do_send()
    {
        std::ostringstream os;
        os << "Message " << message_count_++;
        message_ = os.str();

       /* std::error_code ec;
        auto ret = socket_.send_to(asio::buffer(message_), endpoint_, 0, ec);

        if (ec) {
            std::cout << "ec=" << ec.message() << std::endl;
        }
        else {
            std::cout << "ret=" << ret << std::endl;
        }*/
        socket_.async_send_to(
            asio::buffer(message_), endpoint_,
            [this](std::error_code ec, std::size_t /*length*/)
            {
                if (!ec && message_count_ < max_message_count)
                    do_timeout();
            });
    }

    void do_timeout()
    {
        timer_.expires_after(std::chrono::seconds(1));
        timer_.async_wait(
            [this](std::error_code ec)
            {
                if (!ec)
                    do_send();
            });
    }

private:
    asio::ip::udp::endpoint endpoint_;
    asio::ip::udp::socket socket_;
    asio::steady_timer timer_;
    int message_count_;
    std::string message_;
};

int main(int argc, char* argv[])
{
    try
    {
        /*if (argc != 2)
        {
            std::cerr << "Usage: sender <multicast_address>\n";
            std::cerr << "  For IPv4, try:\n";
            std::cerr << "    sender 239.255.0.1\n";
            std::cerr << "  For IPv6, try:\n";
            std::cerr << "    sender ff31::8000:1234\n";
            return 1;
        }*/

        asio::io_context io_context;
        sender s(io_context, asio::ip::make_address("239.255.0.1"/*argv[1]*/));
        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}