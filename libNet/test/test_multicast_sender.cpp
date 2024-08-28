//========================================================================
//[File Name]:test_multicast_sender.cpp
//[Description]: a test file for socket multicast sender.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../multicast/MultiCast.h"

using namespace Net::MultiCast;

#define SENDER_NUM 1000
#define ITERATONS  2000

void test_multicast_sender(asio::io_context* io_c, int send_port) {
    Sender sender(*io_c, "0.0.0.0", send_port);

    /*conn->AsyncSend("239.255.0.1", 29000, msg.c_str(), msg.length() + 1, [port](const udp::endpoint& endpoint, size_t size, uint8_t ec) {
            static unsigned count = 0;
            auto thd_id = std::this_thread::get_id();
            StreamWriter::Instance()->Write(std::cout, "[Test Connector] thd_id=%d, port=%d\
 count=%d, sent_size=%d, ec=%d", thd_id, port, count++, size, (int)ec);
            });*/
     //std::this_thread::sleep_for(std::chrono::milliseconds(1));

    int count = (send_port - 30000)* ITERATONS;
    while (count < (send_port - 30000 + 1)*ITERATONS) {
        std::string msg = std::to_string(count);
        uint8_t ec = 0;
        auto ret = sender.Send("239.255.0.1", 29000, msg.c_str(), msg.length() + 1, ec);
        StreamWriter::Instance()->Write(std::cout, "[Test MultiCast Sender] sender port=%d\
 count=%d, sent_size=%d, ec=%d", send_port, ++count, ret, (int)ec);
    }
}
int main(int argc, char* argv[])
{
    try
    {
        asio::io_context io_c;
        asio::io_context::work worker(io_c);
        std::vector<std::thread> v_threads;
        for (auto i = 0; i < 2; ++i) {
            v_threads.emplace_back(std::thread([&]() {
                io_c.run();
                }));
        }

        int port_start = 30000;
        std::vector<std::shared_ptr<std::thread>> v_thd;
        for (int i = port_start; i < (SENDER_NUM + port_start); i++) {
            std::shared_ptr<std::thread> thd = std::make_shared<std::thread>([&io_c, i] {
                try
                {
                    test_multicast_sender(&io_c, i);
                }
                catch (const std::exception& e)
                {
                    StreamWriter::Instance()->Write(std::cout, "[Test MultiCast Sender] raise ex:%s", e.what());
                }
            });
            v_thd.push_back(thd);            
        }

        for (auto var : v_thd) {
            if (var->joinable()) {
                var->join();
            }
        }
        printf("Main thread wil sleep for 300s!");
        std::this_thread::sleep_for(std::chrono::seconds(60 * 5));

        io_c.stop();
        for (auto& var : v_threads) {
            if (var.joinable())
                var.join();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "[Test MultiCast Sender] Exception: " << e.what() << "\n";
    }
    return 0;
}