#include <stdio.h>   
#include <Winsock2.h>   

#pragma comment(lib,"Ws2_32.lib")//����Sockets��ؿ�  

void main()
{
    SOCKET socket1;
    WSADATA wsaData;
    if (WSAStartup(MAKEWORD(2, 1), &wsaData)) //��ʼ��  
    {
        printf("Winsock�޷���ʼ��!\n");
        WSACleanup();
        return;
    }
    printf("�ͻ��˿�ʼ����SOCKET��\n");
    struct sockaddr_in server;
    int len = sizeof(server);
    server.sin_family = AF_INET;
    server.sin_port = htons(9900); ///server�ļ����˿�   
    server.sin_addr.s_addr = inet_addr("127.0.0.1"); ///server�ĵ�ַ   
    socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    char buffer[32] = "ha1hnc\0ha\0";
    
    while (1)
    {
        auto ret = sendto(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&server, len);
        if (ret != SOCKET_ERROR)//������Ϣ�������������������ȴ�������������ڿͻ�������ǰ�����ǵȴ�״̬  
        {
            printf("�ͻ��˷�������:%s, ret= %d\n", buffer, ret);
        }
        Sleep(100);
        /*auto size = recvfrom(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&server, &len);
        if (size != SOCKET_ERROR)
            printf("�ӷ���˽��յ�������:%s\n, size=%d\n", buffer, size);*/
    }
    closesocket(socket1);
    WSACleanup();

}