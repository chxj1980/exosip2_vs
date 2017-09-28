//////////////////////////////////////////////////////////////////////////  
//  COPYRIGHT NOTICE  
//  Copyright (c) 2011, ���пƼ���ѧ ¬������Ȩ������  
//  All rights reserved.  
//   
/// @file    CortpServer.h  
/// @brief   ortp�������������ļ�  
///  
/// ʵ�ֺ��ṩortp�ķ�������Ӧ�ýӿ�  
///  
/// @version 1.0     
/// @author  ¬��  
/// @date    2011/11/03  
//  
//  
//  �޶�˵����  
//////////////////////////////////////////////////////////////////////////  

#ifndef CORTPSERVER_H_  
#define CORTPSERVER_H_  

#include <ortp/ortp.h>  

/**
*  COrtpServer RTP������
*
*  ����ʹ��RTPЭ��������ݵķ���
*/
class COrtpServer
{
public:

    /**  ���캯��
    *
    *  �ú���Ϊ����Ĺ��캯�����ڴ����������ʱ�Զ�����
    */
    COrtpServer();

    /** ��������
    *
    * �ú���ִ��������������ϵͳ�Զ�����
    */
    ~COrtpServer();

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
    *  @param:  const char * destIP Ŀ�ĵ�ַ��IP
    *  @param:  int dest_rtp_port Ŀ�ĵ�ַ��rtp�����˿ں�
    *  @param:  int dest_rtcp_port Ŀ�ĵ�ַ��rtcp�����˿ں�
    *  @return: bool  �Ƿ�ɹ�
    *  @note:
    *  @see:
    */
    bool create(const char * destIP, int dest_rtp_port, int dest_rtcp_port);

    /** ����RTP����
    *
    *  ��ָ����buffer�е����ݷ��͵��ͻ���
    *  @param:  unsigned char * buffer ��Ҫ���͵�����
    *  @param:  int len ��Ч�ֽ���
    *  @return: int ʵ�ʷ��͵��ֽ���
    *  @note:
    *  @see:
    */
    int send_data(unsigned char *buffer, int len);

private:

    RtpSession *m_pSession;     /** rtp�Ự��� */

    long        m_curTimeStamp; /** ��ǰʱ��� */
    int         m_timeStampInc; /** ʱ������� */

    char       *m_ssrc;         /** ����Դ��ʶ */
};

#endif // COrtpServer_H_  
