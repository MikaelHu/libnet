#include <Winsock2.h>
#include <stdio.h>

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
    printf("��������ʼ����SOCKET��\n");
    struct sockaddr_in local;//������ַ��ؽṹ��  
    struct sockaddr_in from;//�ͻ��˵�ַ��ؽṹ��  
    int fromlen = sizeof(from);
    local.sin_family = AF_INET;
    local.sin_port = htons(27015); ///�����˿�   
    local.sin_addr.s_addr = INADDR_ANY; ///����   
    socket1 = socket(AF_INET, SOCK_DGRAM, 0);
    bind(socket1, (struct sockaddr*)&local, sizeof(local));//��SOCKET���˲��ؼ�  
    char buffer1[1024] = "\0";
    char buffer[1024] = "ha\0hnc00\0hj";
    if (recvfrom(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, &fromlen) != SOCKET_ERROR)//�������ܿͻ��˵�����  
    {
        printf("���ӳɹ����ͻ��˷��͵����ݣ�%s\n", buffer);
        printf("����˿�ʼ��������\n");
    }
    while (1)
    {
        int ret = sendto(socket1, buffer, sizeof(buffer), 0, (struct sockaddr*)&from, fromlen);//�����ݸ��ͻ��ˣ�������  
        printf("����˿�ʼ��������:%s, size=%d\n", buffer, ret);
        Sleep(200);
    }
    closesocket(socket1);
    WSACleanup();
}