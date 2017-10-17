/** jrtplib ע��:
        1. jrtplib����ʹ�û����˿�, �����˿���������RTCP
        2. jrtplib���õ�Ĭ��MUT��1400������ʵ��ÿ�����ݲ��ܷ���1400�ֽڣ���Ϊ������RTPͷ���� */
#include <WinSock2.h>
#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

int main_recv(uint16_t rtp_port, void(*OnRecvPacket)(char* buffer, int len));
int main_send(uint8_t rtp_ip[], uint16_t rtp_port);

#pragma comment(lib, "ws2_32.lib")


void RTP_OnRecvPacket(char* buffer, int len)
{
    static char tmp[1024];
    memset(tmp, 0, sizeof(tmp));

    memcpy(tmp, buffer, len);
    printf("\t\t\t\t\tdata: %s; len: %4d\n", tmp, len);
}

int main(int argc, char* argv[]) 
{
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);


    int cmd = 0;
    printf("1: recv\n2: send\n>>");
    scanf("%d", &cmd);

    uint8_t rtp_ip[4] = { 192, 168, 6, 80 };
    uint16_t rtp_port = 6664;

    if (1 == cmd) {
        main_recv(rtp_port, RTP_OnRecvPacket);
    }
    else if (2 == cmd) {
        main_send(rtp_ip, rtp_port);
    }

    system("pause");
    WSACleanup();
    return 0;
}