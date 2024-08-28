//========================================================================
//[File Name]:test_multicast_receiver.cpp
//[Description]: a test file for socket multicast receiver.
//[Author]:Nico Hu
//[Date]:2020-08-10
//[Other]:Copyright (c) 2020-2050 Nico Hu
//========================================================================
#include "../multicast/MultiCast.h"


using namespace Net::MultiCast;

#define RECV_NUM 1

std::unordered_map<int, std::shared_ptr<Receiver>> v_R;
void do_receive(int recv_id) {
    uint8_t ec = 0;
    static int count = 0;

    /*while (1) {
        char* data = nullptr;
        auto len = v_R[recv_id]->Recieve(&data, ec);
        if (ec) {
            StreamWriter::Instance()->Write(std::cout, "[Test MultiCast Receiver] Recieve, ec=%d", (int)ec);
            return;
        }
        auto endpoint = v_R[recv_id]->GetNetPeer();
        StreamWriter::Instance()->Write(std::cout, "[Test MultiCast Receiver] endpoint ip=%s, port=%d, recv id=%d\
 count=%d, data=%s, size=%d, ec=%d", endpoint.address().to_string().c_str(), endpoint.port(), recv_id, ++count, data, len, (int)ec);
    }*/
    
    auto ret = v_R[recv_id]->AsyncRecieve([recv_id](const udp::endpoint& endpoint, char* data, unsigned len, uint8_t ec) {
        StreamWriter::Instance()->Write(std::cout, "[Test MultiCast Receiver] endpoint ip=%s, port=%d, recv id=%d\
 count=%d, data=%s, size=%d, ec=%d", endpoint.address().to_string().c_str(), endpoint.port(), recv_id, ++count, data, len, (int)ec);

        do_receive(recv_id);
        });
}
void test_multicast_receiver(asio::io_context* io_c, int recv_id) {
    do_receive(recv_id);
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
        std::vector<std::shared_ptr<std::thread>> v_thd;
        for (int i = 0; i < RECV_NUM; i++) {
            std::shared_ptr<std::thread> thd = std::make_shared<std::thread>([&io_c, i] {
                try
                {
                    std::shared_ptr<Receiver> receiver(new Receiver(io_c, "0.0.0.0", "239.255.0.1", 29000));
                    v_R.emplace(i, receiver);
                    test_multicast_receiver(&io_c, i);
                }
                catch (const std::exception& e)
                {
                    StreamWriter::Instance()->Write(std::cout, "[Test MultiCast Receiver] raise ex:%s", e.what());
                }               
                });
            v_thd.push_back(thd);
        }

        printf("Main thread will sleep for 300s!\n");
        std::this_thread::sleep_for(std::chrono::seconds(60 * 5));
        for (auto var : v_thd) {
            if (var->joinable())
                var->join();
        }
        v_thd.clear();

        io_c.stop();
        for (auto& var : v_threads) {
            if (var.joinable())
                var.join();
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "[Test MultiCast Receiver] Exception: " << e.what() << "\n";
    }
    return 0;
}