
//////////////////////////////////////////////////////////////////////////  
//  COPYRIGHT NOTICE  
//  Copyright (c) 2011, ���пƼ���ѧ ¬������Ȩ������  
//  All rights reserved.  
//   
/// @file    CortpClient.cpp    
/// @brief   ortp�ͻ�����ʵ���ļ�  
///  
/// ʵ�ֺ��ṩortp�Ŀͻ���Ӧ�ýӿ�  
///  
/// @version 1.0     
/// @author  lujun   
/// @date    2011/11/03  
//  
//  
//  �޶�˵����  
//////////////////////////////////////////////////////////////////////////  

#include "CortpClient.h"
#include <assert.h>

/* the payload type define */
#define PAYLOAD_TYPE_VIDEO 34  

/* RTP video Send time stamp increase */
#define VIDEO_TIME_STAMP_INC  3600  

/** ��rtp���ջ�����һ�ζ�ȡ���ֽ��� */
#define READ_RECV_PER_TIME    128  

COrtpClient::COrtpClient()
{
    m_pSession = NULL;
    m_timeStampInc = 0;
    m_curTimeStamp = 0;
}

COrtpClient::~COrtpClient()
{
    if (!m_pSession) {
        rtp_session_destroy(m_pSession);
    }
}

bool COrtpClient::init()
{
    WSADATA wsaData;

    /** ��ʼ��winsocket */
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
        return false;
    }

    ortp_init();
    ortp_scheduler_init();

    return true;
}

bool COrtpClient::deInit()
{
    ortp_exit();

    if (WSACleanup() == SOCKET_ERROR) {
        return false;
    }

    return true;
}

bool COrtpClient::create(const char * localip, int local_rtp_port, int local_rtcp_port)
{
    int rc = 0;
    if (m_pSession != NULL) {
        return false;
    }

    /** �����»Ự */
    m_pSession = rtp_session_new(RTP_SESSION_RECVONLY);
    assert(m_pSession != NULL);

    /** ������ز��� */
    rtp_session_set_scheduling_mode(m_pSession, 1);
    rtp_session_set_blocking_mode(m_pSession, 1);
    rc = rtp_session_set_local_addr(m_pSession, localip, local_rtp_port, local_rtcp_port);
    if (rc != 0) {
        printf("rtp_session_set_local_addr: %d\n", GetLastError());
    }

    rtp_session_enable_adaptive_jitter_compensation(m_pSession, 1);
    rtp_session_set_jitter_compensation(m_pSession, 40);

    rtp_session_set_payload_type(m_pSession, PAYLOAD_TYPE_VIDEO);
    m_timeStampInc = VIDEO_TIME_STAMP_INC;

    return true;
}

int COrtpClient::get_recv_data(char *pBuffer, int &len)
{
    int recvBytes = 0;
    int totalBytes = 0;
    int have_more = 1;

    while (have_more)
    {
        if (totalBytes + READ_RECV_PER_TIME > len) {
            /** ��������С���� */
            printf("recv buffer small!\n");
            return -1;
        }

        recvBytes = rtp_session_recv_with_ts(m_pSession, (uint8_t *)pBuffer + totalBytes, READ_RECV_PER_TIME, m_curTimeStamp, &have_more);
        if (recvBytes <= 0) {
            break;
        }
        totalBytes += recvBytes;
    }

    /** �ж��Ƿ��ȡ������ */
    if (totalBytes == 0) {
        return false;
    }

    /** ��¼��Ч�ֽ��� */
    len = totalBytes;

    /** ʱ������� */
    m_curTimeStamp += m_timeStampInc;

    return totalBytes;
}
