#include <Winsock2.h>
#include <stdio.h>

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
    printf("服务器开始创建SOCKET。\n");
    struct sockaddr_in local;//本机地址相关结构体  
    struct sockaddr_in from;//客户端地址相关结构体  
    int fromlen = sizeof(from);
    local.sin_family = AF_INET;
    local.sin_port = htons(27015); ///监听端口   
    local.sin_addr.s_addr = INADDR_ANY; ///本机   
    socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    bind(socket1, (struct sockaddr*)&local, sizeof(local));//绑定SOCKET，此步关键  
    char buffer1[1024] = "\0";
    char buffer[1024] = "ha\0hnc00\0hj";
    if (recvfrom(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromlen) != SOCKET_ERROR)//阻塞接受客户端的请求  
    {
        printf("连接成功，客户端发送的数据：%s\n", buffer);
        printf("服务端开始发送数据\n");
    }
    while (1)
    {
        int ret = sendto(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, fromlen);//发数据给客户端，由于是  
        printf("服务端开始发送数据:%s, size=%d\n", buffer, ret);
        Sleep(200);
    }
    closesocket(socket1);
    WSACleanup();
}