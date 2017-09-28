//////////////////////////////////////////////////////////////////////////  
//  COPYRIGHT NOTICE  
//  Copyright (c) 2011, ���пƼ���ѧ ¬������Ȩ������  
//  All rights reserved.  
//   
/// @file    CortpClient.h    
/// @brief   ortp�ͻ����������ļ�  
///  
/// ʵ�ֺ��ṩortp�Ŀͻ���Ӧ�ýӿ�  
///  
/// @version 1.0     
/// @author  ¬��   
/// @date    2011/11/03  
//  
//  
//  �޶�˵����  
//////////////////////////////////////////////////////////////////////////  

#ifndef CORTPCLIENT_H_  
#define CORTPCLIENT_H_  

#include <ortp/ortp.h>  
#include <string>  

/**
*  COrtpClient ortp�ͻ��˹�����
*
*  �����װ���ṩortp��ؽӿ�
*/
class COrtpClient
{
public:

    /**  ���캯��/��������
    *
    *  �ڴ���/���ٸ������ʱ�Զ�����
    */
    COrtpClient();
    ~COrtpClient();

    /** ORTPģ��ĳ�ʼ��
    *
    *  ������ϵͳ�ʼ���ã�����ORTP��ĳ�ʼ��
    *  @return: bool  �Ƿ�ɹ�
    *  @note:
    *  @see:
    */
    static bool init();

    /** ORTPģ������ʼ��
    *
    *  ��ϵͳ�˳�ǰ���ã�����ORTP����ͷ�
    *  @return: bool  �Ƿ�ɹ�
    *  @note:
    *  @see:
    */
    static bool deInit();

    /** ����RTP���ջỰ
    *
    *  �������RTP���ն˻Ự�������������˵�����
    *  @param:  const char * localip ����ip��ַ
    *  @param:  int local_rtp_port   ����rtp�����˿�
    *  @param:  int local_rtcp_port  ����rtcp�����˿�
    *  @return: bool  �Ƿ�ɹ�
    *  @note:
    *  @see:
    */
    bool create(const char * localip, int local_rtp_port, int local_rtcp_port);

    /** ��ȡ���յ���rtp��
    *
    *  �����յ���rtp���ݰ�ȡ��
    *  @param:  char * pBuffer
    *  @param:  int & len
    *  @return: bool  �Ƿ�ɹ�
    *  @note:
    *  @see:
    */
    int get_recv_data(char *pBuffer, int &len);

private:

    RtpSession *m_pSession;     /** rtp�Ự��� */

    long        m_curTimeStamp; /** ��ǰʱ��� */
    int         m_timeStampInc; /** ʱ������� */

};

#endif // CortpClient_H_  
