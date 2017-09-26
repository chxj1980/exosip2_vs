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
#define LISTEN_ADDR ("192.168.6.80")
//���ؼ����˿�
#define UACPORT ("5061")
#define UACPORTINT (5061)
//��UAC��ַ����
#define UACCODE ("100110000201000000")
//����UAC����
#define UACPWD ("12345")

//Զ��UAS IP
#define UAS_ADDR ("192.168.6.80")
//Զ��UAS �˿�
#define UAS_PORT ("5060")
//��ʱ
#define EXPIS 300


struct eXosip_t *excontext = NULL;
//��ǰ����״̬ 1 �Ѿ�ע�� 0 δע��
static int iCurrentStatus;
//ע��ɹ�HANDLE
static int iHandle = -1;

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
        stream << "sip: " << addrCode << "@" << addrIp << ":" << addrPort;
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


unsigned __stdcall eventHandle(void *pUser);
unsigned __stdcall serverHandle(void *pUser);
void help();
void UnRegister();
void RefreshRegister();
void Register();
int SendRegister(int& registerId, CSipFromToHeader &from, CSipFromToHeader &to,
    CContractHeader &contact, const std::string& userName, const std::string& pwd,
    const int expires, int iType);


int main()
{
    iCurrentStatus = 0;
    //�⴦����
    int result = OSIP_SUCCESS;
    //��ʼ����
    excontext = eXosip_malloc();
    if (OSIP_SUCCESS != (result = eXosip_init(excontext))) {
        printf("eXosip_init failure.\n");
        return 1;
    }
    std::cout << "eXosip_init success." << std::endl;
    eXosip_set_user_agent(excontext, NULL);
    //����
    if (OSIP_SUCCESS != eXosip_listen_addr(excontext, IPPROTO_UDP, NULL, UACPORTINT, AF_INET, 0)) {
        printf("eXosip_listen_addr failure.\n");
        return 1;
    }
    //���ü�������
    if (OSIP_SUCCESS != eXosip_set_option(excontext, EXOSIP_OPT_SET_IPV4_FOR_GATEWAY, LISTEN_ADDR)) {
        return -1;
    }

    //���������߳�
    _beginthreadex(NULL, 0, serverHandle, NULL, 0, 0);

    //�¼����ڵȴ�
    eXosip_event_t* osipEventPtr = NULL;
    //�����¼�ѭ��
    while (true)
    {
        //�ȴ��¼� 0�ĵ�λ���룬500�Ǻ���
        osipEventPtr = ::eXosip_event_wait(excontext, 0, 200);
        //����eXosip��Ĭ�ϴ���
        {
            Sleep(500);
            eXosip_lock(excontext);
            //һ�㴦��401/407���ÿ�Ĭ�ϴ���
            eXosip_default_action(excontext, osipEventPtr);
            eXosip_unlock(excontext);
        }
        //�¼��ռ����ȴ�
        if (NULL == osipEventPtr)
        {
            continue;
        }
        //�����̴߳����¼������¼�������Ͻ��¼�ָ���ͷ�
        _beginthreadex(NULL, 0, eventHandle, (void*)osipEventPtr, 0, 0);

        osipEventPtr = NULL;
    }
}







//����ע����Ϣ
int SendRegister(int& registerId, CSipFromToHeader &from, CSipFromToHeader &to,
    CContractHeader &contact, const std::string& userName, const std::string& pwd,
    const int expires, int iType)
{
    std::cout << "=============================================" << std::endl;
    if (iType == 0)
    {
        std::cout << "ע��������Ϣ��" << std::endl;
    }
    else if (iType == 1)
    {
        std::cout << "ˢ��ע����Ϣ��" << std::endl;
    }
    else
    {
        std::cout << "ע����Ϣ:" << std::endl;
    }
    std::cout << "registerId " << registerId << std::endl;
    std::cout << "from " << from.GetFormatHeader() << std::endl;
    std::cout << "to " << to.GetFormatHeader() << std::endl;
    std::cout << "contact" << contact.GetContractFormatHeader() << std::endl;
    std::cout << "userName" << userName << std::endl;
    std::cout << "pwd" << pwd << std::endl;
    std::cout << "expires" << expires << std::endl;
    std::cout << "=============================================" << std::endl;
    //������ע��
    static osip_message_t *regMsg = 0;
    int ret;

    ::eXosip_add_authentication_info(excontext, userName.c_str(), userName.c_str(), pwd.c_str(), "MD5", NULL);
    eXosip_lock(excontext);
    //����ע����Ϣ 401��Ӧ��eXosip2���Զ�����
    if (0 == registerId)
    {
        // ע����Ϣ�ĳ�ʼ��
        registerId = ::eXosip_register_build_initial_register(excontext,
            from.GetFormatHeader().c_str(), to.GetFormatHeader().c_str(),
            contact.GetContractFormatHeader().c_str(), expires, &regMsg);
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
    eXosip_unlock(excontext);

    return ret;
}

//ע��
void Register()
{
    if (iCurrentStatus == 1)
    {
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
    if (0 > SendRegister(registerId, stFrom, stTo, stContract, UACCODE, UACPWD,
        3000, 0))
    {
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


//�������߳�
unsigned __stdcall serverHandle(void *pUser)
{
    Sleep(3000);
    while (true)
    {
        help();
        char ch[256] = { 0 };
        scanf("%s", ch);

        switch (ch[0])
        {
        case '0':
            //ע��
            Register();
            break;
        case '1':
            //ˢ��ע��
            RefreshRegister();
            break;
        case '2':
            //ע��
            UnRegister();
            break;
        case '3':
            if (system("clear") < 0)
            {
                std::cout << "clear scream error" << std::endl;
                exit(1);
            }
            break;
        case '4':
            std::cout << "exit sipserver......" << std::endl;
            system("pause");
            exit(0);
        default:
            std::cout << "select error" << std::endl;
            break;
        }
    }
    return NULL;
}

//�¼������߳�
unsigned __stdcall eventHandle(void *pUser)
{
    eXosip_event_t* osipEventPtr = (eXosip_event_t*)pUser;
    switch (osipEventPtr->type)
    {
        //��Ҫ������֤REGISTER��ʲô����
    case EXOSIP_REGISTRATION_SUCCESS:
    case EXOSIP_REGISTRATION_FAILURE:
    {
        std::cout << "�յ�״̬��:" << osipEventPtr->response->status_code << "����" << std::endl;
        if (osipEventPtr->response->status_code == 401)
        {
            std::cout << "���ͼ�Ȩ����" << std::endl;
        }
        else if (osipEventPtr->response->status_code == 200)
        {
            std::cout << "���ճɹ�" << std::endl;
        }
        else
        {
        }
    }
    break;
    default:
        std::cout << "The sip event type that not be precessed.the event " "type is : " << osipEventPtr->type << std::endl;
        break;
    }
    eXosip_event_free(osipEventPtr);
    return NULL;
}



void help()
{
    const char *b =
        "-------------------------------------------------------------------------------\n"
        "SIP UAC�� ע��,ˢ��ע��,ע��ʵ��\n\n"
        "-------------------------------------------------------------------------------\n"
        "\n"
        "              0:Register\n"
        "              1:RefreshRegister\n"
        "              2:UnRegister\n"
        "              3:clear scream\n"
        "              4:exit\n"
        "-------------------------------------------------------------------------------\n"
        "\n";
    fprintf(stderr, b, strlen(b));
    std::cout << "please select method :";
}