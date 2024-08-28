#include <stdio.h>   
#include <Winsock2.h>   

#pragma comment(lib,"Ws2_32.lib")//连接Sockets相关库  

void main()
{
    SOCKET socket1;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 1), &wsaData)) //初始化  
    {
        printf("Winsock无法初始化!\n");
        WSACleanup();
        return;
    }
    printf("客户端开始创建SOCKET。\n");
    struct sockaddr_in server;
    int len = sizeof(server);
    server.sin_family = AF_INET;
    server.sin_port = htons(9900); ///server的监听端口   
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); ///server的地址   
    socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    char buffer[32] = "ha1hnc\0ha\0";
    
    while (1)
    {
        auto ret = sendto(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&server, len);
        if (ret != SOCKET_ERROR)//发送信息给服务器，发送完进入等待，代表服务器在客户端启动前必须是等待状态  
        {
            printf("客户端发送数据:%s, ret= %d\n", buffer, ret);
        }
        Sleep(100);
        /*auto size = recvfrom(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&server, &len);
        if (size != SOCKET_ERROR)
            printf("从服务端接收到的数据:%s\n, size=%d\n", buffer, size);*/
    }
    closesocket(socket1);
    WSACleanup();

}