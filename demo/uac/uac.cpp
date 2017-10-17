/*
===============================================================
GBT28181 ����eXosip2,osip��ʵ��ע��UAC����
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
#include <process.h>

//���ؼ���IP
const char* LISTEN_ADDR = "192.168.6.80";
//���ؼ����˿�
const char* UACPORT = "5061";
//��UAC��ַ����
const char* UACCODE = "100110000201000000";
//����UAC����
const char* UACPWD = "12345";

//Զ��UAS IP
const char* UAS_ADDR = "192.168.6.80";
//Զ��UAS �˿�
const char* UAS_PORT = "5060";
//��ʱ
const int EXPIS = 300;

char *source_call = "sip:300@192.168.6.80";
char *dest_call = "sip:200@192.168.6.80:5060";

struct eXosip_t *excontext = NULL;
//��ǰ����״̬ 1 �Ѿ�ע�� 0 δע��
static int iCurrentStatus;
//ע��ɹ�HANDLE, ���ͦ��Ҫ
static int iHandle = -1;

int call_id, dialog_id;


#define snprintf _snprintf

//SIP From/To ͷ��
class CSipFromToHeader
{
public:
    void SetHeader(std::string addrCod, std::string addrI, std::string addrPor) {
        addrCode = addrCod;
        addrIp = addrI;
        addrPort = addrPor;
    }
    std::string GetFormatHeader() {
        std::stringstream stream;
        stream << "sip:" << addrCode << "@" << addrIp << ":" << addrPort;
        return stream.str();
    }
    //��������
    std::string GetCode() {
        std::stringstream stream;
        stream << addrCode;
        return stream.str();
    }
    //������ַ
    std::string GetAddr() {
        std::stringstream stream;
        stream << addrIp;
        return stream.str();
    }
    //�˿�
    std::string GetPort() {
        std::stringstream stream;
        stream << addrPort;
        return stream.str();
    }

private:
    std::string addrCode;
    std::string addrIp;
    std::string addrPort;
};

//SIP Contractͷ��
class CContractHeader : public CSipFromToHeader
{
public:
    void SetContractHeader(std::string addrCod, std::string addrI, std::string addrPor) {
        SetHeader(addrCod, addrI, addrPor);
    }
    std::string GetContractFormatHeader() {
        std::stringstream stream;
        stream << "<sip:" << GetCode() << "@" << GetAddr() << ":" << GetPort() << ">";
        return stream.str();
    }
};


unsigned __stdcall serverHandle(void *pUser);
void help();
void UnRegister();
void RefreshRegister();
void Register();
int SendRegister(int& registerId, CSipFromToHeader &from, CSipFromToHeader &to,
    CContractHeader &contact, const std::string& userName, const std::string& pwd,
    const int expires, int iType);
void Invate_Call();


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
    iCurrentStatus = 0;
    //�⴦����
    int result = OSIP_SUCCESS;

    //��ʼ����
    excontext = eXosip_malloc();

    result = eXosip_init(excontext);
    assert(OSIP_SUCCESS == result);
    std::cout << "eXosip_init success." << std::endl;

    eXosip_set_cbsip_message(excontext, MyCbSipCallback);
    eXosip_set_user_agent(excontext, NULL);

    //����
    result = eXosip_listen_addr(excontext, IPPROTO_UDP, NULL, atoi(UACPORT), AF_INET, 0);
    assert(OSIP_SUCCESS == result);

    //���ü�������
    result = eXosip_set_option(excontext, EXOSIP_OPT_SET_IPV4_FOR_GATEWAY, LISTEN_ADDR);
    assert(OSIP_SUCCESS == result);

    //���������߳�
    _beginthreadex(NULL, 0, serverHandle, NULL, 0, 0);

    //�����¼�ѭ��
    while (true)
    {
        //�ȴ��¼� 0�ĵ�λ���룬500�Ǻ���
        eXosip_event_t* je = ::eXosip_event_wait(excontext, 0, 200);
        //�¼��ռ����ȴ�
        if (NULL == je) {
            continue;
        }

        //eXosip��Ĭ�ϴ���
        {
            //һ�㴦��401/407���ÿ�Ĭ�ϴ���
            eXosip_lock(excontext);
            eXosip_default_action(excontext, je);
            eXosip_unlock(excontext);
        }


        // �����¼�
        {
            switch (je->type)
            {
                //��Ҫ������֤REGISTER��ʲô����
            case EXOSIP_REGISTRATION_SUCCESS:
            case EXOSIP_REGISTRATION_FAILURE:
            {
                std::cout << "�յ�״̬��:" << je->response->status_code << "����" << std::endl;
                if (je->response->status_code == 401) {
                    std::cout << "���ͼ�Ȩ����" << std::endl;
                }
                else if (je->response->status_code == 200) {
                    std::cout << "���ճɹ�" << std::endl;
                }
                else {
                }
            } break;
                // INVITE
            case EXOSIP_CALL_INVITE:
                printf("a new invite reveived!\n");
                break;
            case EXOSIP_CALL_PROCEEDING:
                printf("proceeding!\n");
                break;
            case EXOSIP_CALL_RINGING:
            {
                printf("ringing!\n");
                printf("\ncall_id is %d, dialog_id is %d \n", je->cid, je->did);
            } break;
            case EXOSIP_CALL_ANSWERED:
            {
                printf("ok! connected!\n");
                call_id = je->cid;
                dialog_id = je->did;
                printf("call_id is %d, dialog_id is %d \n", je->cid, je->did);

                osip_message_t *ack = NULL;
                eXosip_call_build_ack(excontext, je->did, &ack);
                eXosip_call_send_ack(excontext, je->did, ack);
            } break;
            case EXOSIP_CALL_CLOSED:
                printf("the other sid closed!\n");
                break;
            case EXOSIP_CALL_ACK:
                printf("ACK received!\n");
                break;
            default:
                std::cout << "The sip event type that not be precessed.the event " "type is : " << je->type << std::endl;
                break;
            }
        }

        eXosip_event_free(je);
    }
}

//�������߳�
unsigned __stdcall serverHandle(void *pUser)
{
    while (true)
    {
        help();
        char ch[256] = { 0 };
        scanf("%s", ch);

        switch (ch[0])
        {
        case '0':
        {
            Register(); //ע��
        } break;
        case '1':
        {
            RefreshRegister(); //ˢ��ע��
        } break;
        case '2':
        {
            UnRegister(); //ע��
        } break;
        case  '3':
        {
            Invate_Call();
        } break;
        case 's':
        {
            // ���� INFO ����
            osip_message_t* info = NULL;;
            eXosip_call_build_info(excontext, dialog_id, &info);

            char tmp[4096] = { 0 };
            snprintf(tmp, 4096, "hello, bluesea");
            osip_message_set_body(info, tmp, strlen(tmp));

            // ��ʽ���������趨, text/plain �����ı���Ϣ
            osip_message_set_content_type(info, "text/plain");
            eXosip_call_send_request(excontext, dialog_id, info);
            break;
        } break;
        case 'm':
        {
            // ���� MESSAGE����,Ҳ���Ǽ�ʱ��Ϣ��
            // �� INFO ������ȣ���Ҫ������ MESSAGE ���ý������ӣ�ֱ�Ӵ�����Ϣ��
            // �� INFO �����ڽ��� INVITE �Ļ����ϴ��䡣
            std::cout << "\n\t--> the mothed :MESSAGE \n" << std::endl;
            osip_message_t* message = NULL;
            eXosip_message_build_request(excontext, &message,
                "MESSAGE",
                dest_call,
                source_call,
                NULL);
            char* strMsg = "message: hello bluesea!";
            osip_message_set_body(message, strMsg, strlen(strMsg));

            // �����ʽ��xml
            osip_message_set_content_type(message, "text/xml");
            eXosip_message_send_request(excontext, message);
            break;
        }break;
        case '4':
        {
            if (system("clear") < 0) {
                std::cout << "clear scream error" << std::endl;
                exit(1);
            }
        } break;
        case '5':
        {
            std::cout << "exit sipserver......" << std::endl;
            system("pause");
            exit(0);
        } break;
        default:
        {
            std::cout << "select error" << std::endl;
        } break;
        }

        Sleep(5000);
    }
    return NULL;
}






//����ע����Ϣ
int SendRegister(int& registerId, CSipFromToHeader &from, CSipFromToHeader &to,
    CContractHeader &contact, const std::string& userName, const std::string& pwd,
    const int expires, int iType)
{
    std::cout << "=============================================" << std::endl;
    if (iType == 0) {
        std::cout << "ע��������Ϣ��" << std::endl;
    }
    else if (iType == 1) {
        std::cout << "ˢ��ע����Ϣ��" << std::endl;
    }
    else {
        std::cout << "ע����Ϣ:" << std::endl;
    }

    std::cout << "registerId " << registerId << std::endl;
    std::cout << "from " << from.GetFormatHeader() << std::endl;
    std::cout << "to " << to.GetFormatHeader() << std::endl;
    std::cout << "contact " << contact.GetContractFormatHeader() << std::endl;
    std::cout << "username " << userName << std::endl;
    std::cout << "pwd " << pwd << std::endl;
    std::cout << "expires " << expires << std::endl;
    std::cout << "=============================================" << std::endl;

    //������ע��
    static osip_message_t *regMsg = 0;
    int ret;

    ::eXosip_add_authentication_info(excontext, userName.c_str(), userName.c_str(), pwd.c_str(), "MD5", NULL);
    eXosip_lock(excontext);
    {
        //����ע����Ϣ 401��Ӧ��eXosip2���Զ�����
        if (0 == registerId)
        {
            // ע����Ϣ�ĳ�ʼ��
            registerId = ::eXosip_register_build_initial_register(excontext,
                from.GetFormatHeader().c_str(),
                to.GetFormatHeader().c_str(),
                contact.GetContractFormatHeader().c_str(),
                expires,
                &regMsg);
            if (registerId <= 0) {
                return -1;
            }
        }
        else
        {
            // ����ע����Ϣ
            ret = ::eXosip_register_build_register(excontext, registerId, expires, &regMsg);
            if (ret != OSIP_SUCCESS) {
                return ret;
            }

            //���ע��ԭ��
            if (expires == 0)
            {
                osip_contact_t *contact = NULL;
                char tmp[128];

                osip_message_get_contact(regMsg, 0, &contact);
                {
                    sprintf(tmp, "<sip:%s@%s:%s>;expires=0", contact->url->username, contact->url->host, contact->url->port);
                }
                //osip_contact_free(contact);
                //reset contact header
                osip_list_remove(&regMsg->contacts, 0);
                osip_message_set_contact(regMsg, tmp);
                osip_message_set_header(regMsg, "Logout-Reason", "logout");
            }
        }

        // ����ע����Ϣ
        ret = ::eXosip_register_send_register(excontext, registerId, regMsg);
        if (ret != OSIP_SUCCESS) {
            registerId = 0;
        }
    }
    eXosip_unlock(excontext);

    return ret;
}

//ע��
void Register()
{
    if (iCurrentStatus == 1) {
        std::cout << "��ǰ�Ѿ�ע��" << std::endl;
        return;
    }

    CSipFromToHeader stFrom;
    stFrom.SetHeader(UACCODE, UAS_ADDR, UAS_PORT);
    CSipFromToHeader stTo;
    stTo.SetHeader(UACCODE, UAS_ADDR, UAS_PORT);
    CContractHeader stContract;
    stContract.SetContractHeader(UACCODE, LISTEN_ADDR, UACPORT);

    //����ע����Ϣ
    int registerId = 0;
    if (0 > SendRegister(registerId, stFrom, stTo, stContract, UACCODE, UACPWD, 3000, 0)) {
        std::cout << "����ע��ʧ��" << std::endl;
        return;
    }
    iCurrentStatus = 1;
    iHandle = registerId;
}
//ˢ��ע��
void RefreshRegister()
{
    if (iCurrentStatus == 0) {
        std::cout << "��ǰδע�ᣬ������ˢ��" << std::endl;
        return;
    }

    CSipFromToHeader stFrom;
    stFrom.SetHeader(UACCODE, UAS_ADDR, UAS_PORT);
    CSipFromToHeader stTo;
    stTo.SetHeader(UACCODE, UAS_ADDR, UAS_PORT);
    CContractHeader stContract;
    stContract.SetContractHeader(UACCODE, LISTEN_ADDR, UACPORT);
    //����ע����Ϣ
    if (0 > SendRegister(iHandle, stFrom, stTo, stContract, UACCODE, UACPWD, 3000, 1))
    {
        std::cout << "����ˢ��ע��ʧ��" << std::endl;
        return;
    }
}

//ע��
void UnRegister()
{
    if (iCurrentStatus == 0) {
        std::cout << "��ǰδע�ᣬ������ע��" << std::endl;
        return;
    }

    CSipFromToHeader stFrom;
    stFrom.SetHeader(UACCODE, UAS_ADDR, UAS_PORT);
    CSipFromToHeader stTo;
    stTo.SetHeader(UACCODE, UAS_ADDR, UAS_PORT);
    CContractHeader stContract;
    stContract.SetContractHeader(UACCODE, LISTEN_ADDR, UACPORT);
    //����ע����Ϣ
    if (0 > SendRegister(iHandle, stFrom, stTo, stContract, UACCODE, UACPWD, 0, 2)) {
        std::cout << "����ע��ʧ��" << std::endl;
        return;
    }

    iCurrentStatus = 0;
    iHandle = -1;
}

// ��������
void Invate_Call()
{
    osip_message_t *invite = NULL;
    int result = eXosip_call_build_initial_invite(excontext, &invite, dest_call, source_call, NULL, "This si a call for a conversation");
    if (result != OSIP_SUCCESS) {
        printf("Intial INVITE failed!\n");
        return;
    }

    char tmp[4096];
    snprintf(tmp, 4096,
        "v=0\r\n"
        "o=anonymous 0 0 IN IP4 0.0.0.0\r\n"
        "t=1 10\r\n"
        "a=username:300\r\n"
        "a=password:300\r\n");
    osip_message_set_body(invite, tmp, strlen(tmp));
    osip_message_set_content_type(invite, "application/sdp");

    eXosip_lock(excontext);
    result = eXosip_call_send_initial_invite(excontext, invite);
    eXosip_unlock(excontext);
}


void help()
{
    const char *b =
        "\n\n\n-------------------------------------------------------------------------------\n"
        "SIP UAC�� ע��,ˢ��ע��,ע��ʵ��\n\n"
        "-------------------------------------------------------------------------------\n"
        "\n"
        "              0:Register\n"
        "              1:RefreshRegister\n"
        "              2:UnRegister\n"
        "              3:invate call\n"
        "              s:INFO  method\n"
        "              m:MESSAGE method\n"
        "              4:clear scream\n"
        "              5:exit\n"
        "-------------------------------------------------------------------------------\n"
        "\n";
    fprintf(stderr, b, strlen(b));
    std::cout << "please select method :";
}