/*
===============================================================
GBT28181 SIP���libGBT28181SipComponent.soע��ʵ��
���ߣ���������
���͵�ַ��http://blog.csdn.net/hiwubihe
QQ��1269122125
ע��������ԭ�����Ͷ��ɹ�������ѧϰʹ�ã�������ã�Υ�߱ؾ���
================================================================
*/

#include <iostream>
#include <string>
#include <sstream>
#include <assert.h>
#include <osipparser2/osip_message.h>
#include <osipparser2/osip_parser.h>
#include <osipparser2/osip_port.h>

#include <eXosip2/eXosip.h>
#include <eXosip2/eX_setup.h>
#include <eXosip2/eX_register.h>
#include <eXosip2/eX_options.h>
#include <eXosip2/eX_message.h>
#include <sys/types.h>
//#include <arpa/inet.h>
//#include <sys/socket.h>
#include <WinSock2.h>
#include <Windows.h>


const char* LISTEN_ADDR = "192.168.6.80";
const int UASPORT = 5060;

#define snprintf _snprintf

//��ϵ������UASά���ģ�UAS�ڽ��յ�UAC��δ��Ȩ���ĺ󣬸�UAC�ظ�401���ڸñ����б���Ҫ�������֤ϵ������֤���� ��UAS��ֵ����֤�������
const char* NONCE = "9bd055";
//UASĬ�ϼ����㷨
const char* ALGORITHTHM = "MD5";

struct eXosip_t *excontext = NULL;
//SIP Fromͷ��
class CSipFromHeader
{
public:
    void SetHeader(std::string addrCod, std::string addrI, std::string addrPor) {
        addrCode = addrCod;
        addrIp = addrI;
        addrPort = addrPor;
    }

    std::string GetFormatHeader() {
        std::stringstream stream;
        stream << "<sip:" << addrCode << "@" << addrIp << ":" << addrPort << ">";
        return stream.str();
    }
    //��������
    std::string GetRealName() {
        std::stringstream stream;
        stream << addrIp;
        return stream.str();
    }
private:
    std::string addrCode;
    std::string addrIp;
    std::string addrPort;
};

//SIP Contractͷ��
class CContractHeader : public CSipFromHeader
{
public:
    void SetContractHeader(std::string addrCod, std::string addrI, std::string addrPor, int expire) {
        SetHeader(addrCod, addrI, addrPor);
        expires = expire;
    }
    std::string GetContractFormatHeader(bool bExpires) {
        if (!bExpires) {
            return GetFormatHeader();
        }
        else {
            std::string sTmp = GetFormatHeader();
            std::stringstream stream;
            stream << ";" << "expires=" << expires;
            sTmp += stream.str();
            return sTmp;
        }

    }
private:
    int expires;
};

struct SipContextInfo
{
    //Sip�㷵�ص�����ı�־ ��Ӧʱ���ؼ���
    int sipRequestId;
    //ά��һ��ע��
    std::string callId;
    //��Ϣ�����Ĺ��ܷ������ַ���
    std::string method;
    //��ַ����@������IP��ַ:���Ӷ˿ڣ�����sip:1111@127.0.0.1:5060
    CSipFromHeader from;
    //��ַ����@������IP��ַ:���Ӷ˿ڣ�����sip:1111@127.0.0.1:5060
    CSipFromHeader proxy;
    //��ַ����@������IP��ַ:���Ӷ˿ڣ�����sip:1111@127.0.0.1:5060
    CContractHeader contact;
    //��Ϣ����,һ��ΪDDCP��Ϣ��XML�ĵ�,���߾���Э��֡Ҫ��������ַ����ı�
    std::string content;
    //��Ӧ״̬��Ϣ
    std::string status;
    //��ʱ,ʱ�䵥λΪ��
    int expires;
};

struct SipAuthInfo
{
    //ƽ̨������
    std::string digestRealm;
    //ƽ̨�ṩ�������
    std::string nonce;
    //�û���
    std::string userName;
    //����
    std::string response;
    //��sip:ƽ̨��ַ��,����Ҫuac��ֵ
    std::string uri;
    //�����㷨MD5
    std::string algorithm;
};

struct sipRegisterInfo
{
    SipContextInfo baseInfo;
    SipAuthInfo authInfo;
    bool isAuthNull;
};

void OnRegister(eXosip_event_t *osipEvent);
void Register_SendAnswer(sipRegisterInfo&info);
void printRegisterPkt(sipRegisterInfo&info);
void parserRegisterInfo(osip_message_t*request, int iReqId, sipRegisterInfo &regInfo);

void __stdcall MyCbSipCallback(osip_message_t * msg, int received)
{
    char* dest;
    size_t length;
    osip_message_to_str(msg, &dest, &length);

    printf("\n\t\t\t\t\t <#######[%s] sip_method: %s, status_code: %d  %s \n%s\n#######>\n",
        (received == 1) ? "recv" : "send", msg->sip_method, msg->status_code, msg->reason_phrase,
        dest);
}

int main()
{
    int result = OSIP_SUCCESS;
    int call_id, dialog_id;
    // ����
    excontext = eXosip_malloc();

    // ��ʼ��
    result = eXosip_init(excontext);
    assert(OSIP_SUCCESS == result);

    // ����SIP��Ϣ�ص�(����+����)
    result = eXosip_set_cbsip_message(excontext, MyCbSipCallback);
    assert(OSIP_SUCCESS == result);

    eXosip_set_user_agent(excontext, NULL);

    // ����
    result = eXosip_listen_addr(excontext, IPPROTO_UDP, NULL, UASPORT, AF_INET, 0);
    assert(OSIP_SUCCESS == result);
    printf("listen on port %d success\n", UASPORT);

    result = eXosip_set_option(excontext, EXOSIP_OPT_SET_IPV4_FOR_GATEWAY, LISTEN_ADDR);
    assert(OSIP_SUCCESS == result);

    //����ѭ����Ϣ��ʵ��Ӧ���п��Կ������߳�ͬʱ�����ź�
    while (true)
    {
        // �ȴ�sip�¼�
        eXosip_event_t* je = ::eXosip_event_wait(excontext, 0, 200);// 0�ĵ�λ���룬200�Ǻ���  If get nothing osip event,then continue the loop.
        if (NULL == je) {
            continue;
        }

        {
            eXosip_lock(excontext);
            eXosip_default_action(excontext, je);
            eXosip_unlock(excontext);
        }


        // �¼�����
        switch (je->type)
        {
        case EXOSIP_MESSAGE_NEW: // �µ���Ϣ����
        {
            if (MSG_IS_REGISTER(je->request)) {
                OnRegister(je);
            }
            else if (MSG_IS_MESSAGE(je->request))
            {
                {
                    osip_body_t *body = NULL;
                    osip_message_get_body(je->request, 0, &body);
                    std::cout << "I get the msg is: " << body->body << std::endl;
                }

                // ���չ�����Ҫ�ظ� OK ��Ϣ
                osip_message_t* answer = NULL;
                eXosip_message_build_answer(excontext, je->tid, 200, &answer);
                eXosip_message_send_answer(excontext, je->tid, 200, answer);
            }
        } break;
        case EXOSIP_CALL_INVITE: // INVITE ������Ϣ
        {
            // �õ����յ���Ϣ�ľ�����Ϣ
            std::cout << "\n\tReceived a INVITE msg from " << je->request->req_uri->host
                << " : " << je->request->req_uri->port
                << ", username is " << je->request->req_uri->username << std::endl;

            // �õ���Ϣ��,��Ϊ����Ϣ���� SDP ��ʽ.
            sdp_message_t* remote_sdp = eXosip_get_remote_sdp(excontext, je->did);
            call_id = je->cid;
            dialog_id = je->did;

            eXosip_lock(excontext);

            osip_message_t* answer;
            eXosip_call_send_answer(excontext, je->tid, 180, NULL);
            int result = eXosip_call_build_answer(excontext, je->tid, 200, &answer);
            if (result != OSIP_SUCCESS)
            {
                std::cout << "\n\t--> This request msg is invalid! Cann't response!\n" << std::endl;
                eXosip_call_send_answer(excontext, je->tid, 400, NULL);
            }
            else
            {
                char tmp[4096] = { 0 };
                snprintf(tmp, 4096,
                    "v=0\r\n"
                    "o=anonymous 0 0 IN IP4 0.0.0.0\r\n"
                    "t=1 10\r\n"
                    "a=username:rainfish\r\n"
                    "a=password:123\r\n");

                // ���ûظ���SDP��Ϣ��,��һ���ƻ�������Ϣ��
                // û�з�����Ϣ�壬ֱ�ӻظ�ԭ������Ϣ����һ�����Ĳ��á�
                osip_message_set_body(answer, tmp, strlen(tmp));
                osip_message_set_content_type(answer, "application/sdp");

                eXosip_call_send_answer(excontext, je->tid, 200, answer);
                std::cout << "\n\t--> send 200 over!" << std::endl;
            }

            eXosip_unlock(excontext);

            // ��ʾ���� sdp ��Ϣ���е� attribute ������,����ƻ�������ǵ���Ϣ
            std::cout << "\n\t--> The INFO is :\n";
            int pos = 0;
            while (!osip_list_eol(&(remote_sdp->a_attributes), pos))
            {
                sdp_attribute_t *at;

                //���������Ϊʲô��SDP��Ϣ��������a�����ű���������
                at = (sdp_attribute_t *)osip_list_get(&remote_sdp->a_attributes, pos);
                std::cout << "\n\t" << at->a_att_field
                    << " : " << at->a_att_value << std::endl;

                pos++;
            }
            break;

        } break;
        case EXOSIP_CALL_ACK:
            std::cout << "\n\t--> ACK recieved!\n" << std::endl;
            // printf ("the cid is %s, did is %s\n", je->did, je->cid); 
            break;
        case EXOSIP_CALL_CLOSED:
        {
            std::cout << "\n\t--> the remote hold the session!\n" << std::endl;
            // eXosip_call_build_ack(dialog_id, &ack);
            // eXosip_call_send_ack(dialog_id, ack); 
            osip_message_t* answer = NULL;
            int result = eXosip_call_build_answer(excontext, je->tid, 200, &answer);
            if (result != 0)
            {
                printf("This request msg is invalid!Cann't response!\n");
                eXosip_call_send_answer(excontext, je->tid, 400, NULL);
            }
            else
            {
                eXosip_call_send_answer(excontext, je->tid, 200, answer);
                std::cout << "\n\t--> bye send 200 over!\n";
            }
        } break;
        case EXOSIP_CALL_MESSAGE_NEW:
        {
            std::cout << "\n\t*** EXOSIP_CALL_MESSAGE_NEW\n" << std::endl;
            if (MSG_IS_INFO(je->request)) // ���������� INFO ����
            {
                eXosip_lock(excontext);
                {
                    osip_message_t* answer = NULL;
                    int result = eXosip_call_build_answer(excontext, je->tid, 200, &answer);
                    if (result == OSIP_SUCCESS) {
                        eXosip_call_send_answer(excontext, je->tid, 200, answer);
                    }
                }
                eXosip_unlock(excontext);

                {
                    osip_body_t *body;
                    osip_message_get_body(je->request, 0, &body);
                    std::cout << "the body is " << body->body << std::endl;
                }
            }
        } break;

        default:
        {
            std::cout << "The sip event type that not be precessed.the event type is : " << je->type << std::endl;
        } break;
        }

        eXosip_event_free(je);
    }

    return 0;
}


// ����ע����Ϣ
void OnRegister(eXosip_event_t *osipEvent)
{
    sipRegisterInfo regInfo;

    parserRegisterInfo(osipEvent->request, osipEvent->tid, regInfo);

    //printRegisterPkt(regInfo); //��ӡ����

    //����Ӧ����
    Register_SendAnswer(regInfo);
}



// ����sip��Ϣ
void parserRegisterInfo(osip_message_t*request, int iReqId, sipRegisterInfo &regInfo)
{
    std::stringstream stream;
    regInfo.baseInfo.method = request->sip_method;
    regInfo.baseInfo.from.SetHeader(request->from->url->username, request->from->url->host, request->from->url->port);
    regInfo.baseInfo.proxy.SetHeader(request->to->url->username, request->to->url->host, request->to->url->port);

    //��ȡexpires
    osip_header_t* header = NULL;
    {
        osip_message_header_get_byname(request, "expires", 0, &header);
        if (NULL != header && NULL != header->hvalue) {
            regInfo.baseInfo.expires = atoi(header->hvalue);
        }
    }

    //contact�ֶ�
    osip_contact_t* contact = NULL;
    osip_message_get_contact(request, 0, &contact);
    if (NULL != contact) {
        regInfo.baseInfo.contact.SetContractHeader(contact->url->username, contact->url->host, contact->url->port, regInfo.baseInfo.expires);
    }

    //ע�᷵�� �ɷ��ͷ�ά��������ID ���շ����պ�ԭ�����ؼ���
    regInfo.baseInfo.sipRequestId = iReqId;
    //CALL_ID
    {
        stream.str("");
        stream << request->call_id->number;
        regInfo.baseInfo.callId = stream.str();
    }

    //����content��Ϣ
    osip_body_t * body = NULL;
    osip_message_get_body(request, 0, &body);
    if (body != NULL)
    {
        stream.str("");
        stream << body->body;
        regInfo.baseInfo.content = stream.str();
    }

    //��Ȩ��Ϣ
    osip_authorization_t* authentication = NULL;
    {
        osip_message_get_authorization(request, 0, &authentication);
        if (NULL == authentication) {
            regInfo.isAuthNull = true;
        }
        else
        {
            regInfo.isAuthNull = false;
            stream.str("");
            stream << authentication->username;
            regInfo.authInfo.userName = stream.str();
            stream.str("");
            stream << authentication->algorithm;
            regInfo.authInfo.algorithm = stream.str();
            stream.str("");
            stream << authentication->realm;
            regInfo.authInfo.digestRealm = stream.str();
            stream.str("");
            stream << authentication->nonce;
            regInfo.authInfo.nonce = stream.str();
            stream.str("");
            stream << authentication->response;
            regInfo.authInfo.response = stream.str();
            stream.str("");
            stream << authentication->uri;
            regInfo.authInfo.uri = stream.str();
        }
    }
    authentication = NULL;
}

//����Ӧ����
void Register_SendAnswer(sipRegisterInfo& info)
{
    osip_message_t* answer = NULL;
    int iStatus;
    if (info.isAuthNull) {
        iStatus = 401;
    }
    else {
        iStatus = 200;
    }

    eXosip_lock(excontext);
    {
        int result = ::eXosip_message_build_answer(excontext, info.baseInfo.sipRequestId, iStatus, &answer);
        if (iStatus == 401)
        {
            //��SIP��������֤��������֤�������Ϳͻ���
            std::stringstream stream;
            std::string nonce = NONCE;
            std::string algorithm = ALGORITHTHM;
            stream << "Digest realm=\"" << info.baseInfo.from.GetRealName()
                << "\",nonce=\"" << nonce
                << "\",algorithm=" << algorithm;

            osip_message_set_header(answer, "WWW-Authenticate", stream.str().c_str());
            osip_message_set_header(answer, "MyHead", "this is my head ��Ҫ�������� �鿴WWW-Authenticate");
            osip_message_set_header(answer, "MyHead", stream.str().c_str());
            std::cout << "����401����" << std::endl;
        }
        else if (iStatus == 200)
        {
            osip_message_set_header(answer, "Contact", info.baseInfo.contact.GetContractFormatHeader(true).c_str());
            std::cout << "����200����" << std::endl;
            // std::string_t b = "<sip: 100110000101000000@192.168.31.18:5061>;expires=600";
            //osip_message_set_header(answer, "Contact", b.c_str());
        }
        else
        {
            //Do nothing
        }

        if (OSIP_SUCCESS != result) {
            ::eXosip_message_send_answer(excontext, info.baseInfo.sipRequestId, 400, NULL);
        }
        else {
            //������Ϣ��
            ::eXosip_message_send_answer(excontext, info.baseInfo.sipRequestId, iStatus, answer);
        }

        if (0 == info.baseInfo.expires) {
            eXosip_register_remove(excontext, info.baseInfo.sipRequestId);
        }
    }
    eXosip_unlock(excontext);
}


//��ӡ���յ�����Ӧ����
void printRegisterPkt(sipRegisterInfo& info)
{
    std::cout << "���յ����ģ�" << std::endl;
    std::cout << "================================================================" << std::endl;
    std::cout << "method:" << info.baseInfo.method << std::endl;
    std::cout << "from:    " << info.baseInfo.from.GetFormatHeader() << std::endl;
    std::cout << "to:" << info.baseInfo.proxy.GetFormatHeader() << std::endl;
    std::cout << "contact:" << info.baseInfo.contact.GetContractFormatHeader(false) << std::endl;

    //ע�᷵�� �ɷ��ͷ�ά��������ID ���շ����պ�ԭ�����ؼ���
    std::cout << "sipRequestId:" << info.baseInfo.sipRequestId << std::endl;
    //CALL_ID
    std::cout << "CallId:" << info.baseInfo.callId << std::endl;
    //����content��Ϣ
    std::cout << "body:" << info.baseInfo.content << std::endl;
    //��ȡexpires
    std::cout << "expires:" << info.baseInfo.expires << std::endl;
    //��Ȩ��Ϣ
    if (info.isAuthNull) {
        std::cout << "��ǰ����δ�ṩ��Ȩ��Ϣ!!!" << std::endl;
    }
    else {
        std::cout << "��ǰ���ļ�Ȩ��Ϣ����:" << std::endl;
        std::cout << "username:" << info.authInfo.userName << std::endl;
        std::cout << "algorithm:" << info.authInfo.algorithm << std::endl;
        std::cout << "Realm:" << info.authInfo.digestRealm << std::endl;
        std::cout << "nonce:" << info.authInfo.nonce << std::endl;
        std::cout << "response:" << info.authInfo.response << std::endl;
        std::cout << "uri:" << info.authInfo.uri << std::endl;
    }
    std::cout << "================================================================" << std::endl;
    return;
}
