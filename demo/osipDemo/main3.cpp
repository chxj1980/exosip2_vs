/** ����ע��, ͨ��. ����ms_initʧ�� */

//��osip2 + eXosip2 + ortp + mediastreamerʵ�ֵ�linux������绰Դ���루��ʵ��ͨ����
//http://blog.csdn.net/wanghaobo920/article/details/7993768
//�������
//gcc - losip2 - leXosip2 - lmediastreamer - lpthread - o my_exosip_phone my_exosip_phone.c
//my_exosip_phone.c���£�
#include <eXosip2/eXosip.h>
#include <osip2/osip_mt.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys\types.h>
#include <mediastreamer2/mediastream.h>
#ifdef _MSC_VER
#include <WinSock2.h>
#include <Windows.h>
#include <process.h>
#else
#include <netinet/in.h>
#include <sys/socket.h>
#include <pthread.h>
#endif // _MSC_VER

#define WAIT_TIMER 200
#define REG_TIMER 30*1000

int doing = 1; /* �¼�ѭ������ */
int rtp_port = 9900; //rtp��ý�屾�ض˿�
int dtmfing = 0; 
int calling = 0;/* �Ƿ����ڱ���(�ж�ack����) */
int picked = 0;
int call_id = 0;
int dialog_id = 0;

char *dtmf_str = "this is dtmf";
//CString dtmf_str; 
//char *server_url="192.168.18.249"; 
char *server_url = "192.168.6.80";
char localip[128] = { 0 };
char *server_port = "5060";
int i_server_port = 0;
char *local_port = "5000";
int i_local_port = 0;
char *username = "200";
char *password = "200";
char *telNum = "300";

struct eXosip_t *excontext = NULL;

AudioStream *audio = NULL;
RtpProfile *profile = NULL;
RtpSession *session = NULL;
OrtpEvQueue *q = NULL;

int my_sip_init(bool isAsServer);
void OnHang();
int sip_ua_monitor();
int real_send_register(int expires);
int build_media(int local_port, const char *remote_ip, int remote_port, int payload, const char *fmtp, int jitter, int ec, int bitrate);

int main3(int argc, char* argv[])
{
    //char *payload_str;
    char tmp[4096];
    char command;
    char dtmf[50] = { 0 };

    int reg_remain = REG_TIMER;
    char out_str[100] = { 0 };

    osip_message_t *invite = NULL;
    osip_message_t *info = NULL;
    osip_message_t *message = NULL;

    char source_call[100] = { 0 };
    char dest_call[100] = { 0 };

    bool isAsServer = false;
    if (argc >= 2) {
        isAsServer = (strchr(argv[1], 's') || strchr(argv[1], 'S'));
    }

    i_server_port = atoi(server_port);
    i_local_port = atoi(local_port);

    if (0 != my_sip_init(isAsServer)) {
        printf("Quit!\n");
        return -1;
    }

    eXosip_guess_localip(excontext, AF_INET, localip, sizeof(localip));

    printf("start as %s : %d\n", isAsServer ? "server" : "client", isAsServer ? i_server_port : i_local_port);
    printf("server: %s:%d    local: %s:%d\n\n\n", server_url, i_server_port, localip, i_local_port);

    //���� G711uLaw 0


    printf("r �������ע��\n\n");
    printf("c ȡ��ע��\n\n");
    printf("i �����������\n\n");
    printf("h �Ҷ�\n\n");
    printf("a �����绰\n\n");
    printf("q �Ƴ�����\n\n");
    printf("s ִ�з���INFO\n\n");
    printf("m ִ�з���MESSAGE\n\n");

    int flag = 1;

    while (flag)
    {
        //�������� 
        printf("Please input the command:\n");
        scanf("%c", &command);
        getchar();
        switch (command)
        {
        case 'r':// �������Ȩ��Ϣ����Ӧ��������ļ�Ȩ��Ϣ 
        {
            real_send_register(1800);
            Sleep(3000);
            real_send_register(1800);
        } break;
        case 'i':
        {
            sprintf(source_call, "sip:%s@%s:%s", username, localip, local_port);
            sprintf(dest_call, "sip:%s@%s:%s", telNum, server_url, server_port);

            //char tmp[4096]; 
            memset(tmp, 0, 4096);

            int i = eXosip_call_build_initial_invite(excontext, &invite, dest_call, source_call, NULL, "This is a call invite");
            if (i != 0)
            {
                printf("Intial INVITE failed!\n");
            }

            snprintf(tmp, 4096,
                "v=0\r\n"
                "o=youtoo 1 1 IN IP4 %s\r\n"
                "s=##youtoo demo\r\n"
                "c=IN IP4 %s\r\n"
                "t=0 0\r\n"
                "m=audio %d RTP/AVP 0 8 101\r\n"
                "a=rtpmap:0 PCMU/8000\r\n"
                "a=rtpmap:8 PCMA/8000\r\n"
                "a=rtpmap:101 telephone-event/8000\r\n"
                "a=fmtp:101 0-15\r\n", localip, localip, rtp_port);

            osip_message_set_body(invite, tmp, strlen(tmp));
            osip_message_set_content_type(invite, "application/sdp");

            eXosip_lock(excontext);
            i = eXosip_call_send_initial_invite(excontext, invite);
            eXosip_unlock(excontext);

        } break;
        case 'h': //�Ҷ� 
        {
            OnHang();
        } break;
        case 'c':
        {
            real_send_register(0);
            //printf("This modal is not commpleted!\n"); 
        } break;
        case 's': //����INFO���� 
        {
            eXosip_call_build_info(excontext, dialog_id, &info);
            snprintf(tmp, 4096, "\nThis is a sip message(Method:INFO)");
            osip_message_set_body(info, tmp, strlen(tmp));
            //��ʽ���������趨��text/plain�����ı���Ϣ 
            osip_message_set_content_type(info, "text/plain");
            eXosip_call_send_request(excontext, dialog_id, info);
        } break;
        case 'm':
        {
            //����MESSAGE������Ҳ���Ǽ�ʱ��Ϣ����INFO������ȣ�����Ϊ��Ҫ�����ǣ� 
            //MESiSAGE���ý������ӣ�ֱ�Ӵ�����Ϣ����INFO��Ϣ�����ڽ���INVITE�Ļ����ϴ��� 
            memset(tmp, 0, 4096);
            printf("the method : MESSAGE\n");
            eXosip_message_build_request(excontext, &message, "MESSAGE", dest_call, source_call, NULL);
            //���ݣ������� to ��from ��route 
            snprintf(tmp, 4096, "This is a sip message(Method:MESSAGE)");
            osip_message_set_body(message, tmp, strlen(tmp));
            //�����ʽ��xml 
            osip_message_set_content_type(message, "text/xml");
            eXosip_message_send_request(excontext, message);
        } break;
        case 'q':
        {
            doing = -1;
            flag = 0;
            Sleep(1000); //��֤�¼��߳����˳�(�߳�ѭ�����ʱ����뼶<1000)  �����ر�ý������(���ض�) 
            printf("Bye!\n");
        }  break;
        case 'a':
        {
            picked = 1;
            printf("�ѽ���\n");
        } break;
        case 'd':
        {
            dtmfing = 1;
        } break;
        default:
        {
            printf("input error! please input again\n");
        } break;
        }
    }
    return(0);
}

int sip_ua_monitor()
{
    int ret = -1;
    char *payload_str; /* ���������ȱ���ֵ */
    char tmp[4096];
    char dtmf[50] = { 0 };

    int reg_remain = REG_TIMER;
    Sleep(2000);
    printf("Event monitor for uac/uas start!\n");

    eXosip_event_t *uac_e; /* �¼����� */
    osip_message_t *ack = NULL; /* ��Ӧ��Ϣ */
    osip_message_t *answer = NULL; /* ������Ϣ�Ļ�Ӧ */

                                   /* ��ӦSDP(����UAC) */
    sdp_message_t * msg_rsp = NULL;
    sdp_connection_t * con_rsp = NULL;
    sdp_media_t * md_rsp = NULL;

    /* ����SDP(����UAS) */
    sdp_message_t * msg_req = NULL;
    sdp_connection_t * con_req = NULL;
    sdp_media_t * md_req = NULL;

    char out_str[100] = { 0 };

    eXosip_lock(excontext);
    eXosip_automatic_action(excontext);
    eXosip_unlock(excontext);

    while (doing)
    {
        uac_e = eXosip_event_wait(excontext, 0, WAIT_TIMER);

        reg_remain = reg_remain - WAIT_TIMER;
        if (reg_remain < 0)
        {
            //��ʱ������ע�� 
            eXosip_lock(excontext);
            //eXosip_automatic_refresh(excontext);
            eXosip_unlock(excontext);
            reg_remain = REG_TIMER;
            printf("register timeout, retry!");
        }

        if (dtmfing)
        {
            strcpy(dtmf, dtmf_str);
            int index;
            for (index = 0; index < 50; index++)
            {
                //���ζ�ȡ�ַ� 
                if (dtmf[index] == '\0') break;

                /* ����DTMF */
                eXosip_lock(excontext);
                audio_stream_send_dtmf(audio, dtmf[index]);
                eXosip_unlock(excontext);

                sprintf(out_str, "DTMF send <%c> OK!", dtmf[index]);
                printf("%s", out_str);
                Sleep(500);
            }

            dtmfing = 0;
        }

        if (uac_e == NULL)
        {
            //DEBUG_INFO("nothing"); 
            continue;
        }

        eXosip_lock(excontext);
        eXosip_default_action(excontext, uac_e); /* ����407�����Ȩ��Ϣ */
        eXosip_unlock(excontext);

        if (NULL != uac_e->response)
        {
            //UAC ��Ϣ����ǰ��� 
            sprintf(out_str, "%d %s\n", uac_e->response->status_code, uac_e->response->reason_phrase);
            printf(out_str);

            if (487 == uac_e->response->status_code)
            {
                printf("(ȡ�����гɹ�)\n");
                continue;
            }

            if (480 == uac_e->response->status_code)
            {
                //480 ��Ӧ�� 
                printf("(��Ӧ��)\n");
                picked = 0;
                calling = 0;
                call_id = 0;
                dialog_id = 0;
                continue;
            }
            if (401 == uac_e->response->status_code)
            {
                eXosip_clear_authentication_info(excontext); //ȥ���ϴμ���Ĵ�����֤��Ϣ 
                eXosip_add_authentication_info(excontext, username, username, password, "md5", NULL);
                printf("register again!\n");
            }
        }

        if (NULL != uac_e->request)
        {
        }

        if (NULL != uac_e->ack)
        {
        }

        switch (uac_e->type)
        {
        case EXOSIP_CALL_SERVERFAILURE:
        case EXOSIP_CALL_RELEASED:
        {
            /* �����������Է�æ */
            printf("(�Է����������æ!)");

            call_id = 0;
            dialog_id = 0;
            picked = 0;
            calling = 0;

            printf("Dest or Server Busy!");
        } break;

            /* UAS �����¼� */
        case EXOSIP_MESSAGE_NEW: //�µ���Ϣ����
            printf("EXOSIP_MESSAGE_NEW!\n");
            if (MSG_IS_MESSAGE(uac_e->request)) //������ܵ�����Ϣ������MESSAGE 
            {
                osip_body_t *body;
                osip_message_get_body(uac_e->request, 0, &body);
                printf("Reveivc a msg : %s\n", body->body);
            }
            //���չ�����Ҫ�ظ�200 OK��Ϣ 
            eXosip_message_build_answer(excontext, uac_e->tid, 200, &answer);
            eXosip_message_send_answer(excontext, uac_e->tid, 200, answer);
            break;

        case EXOSIP_CALL_INVITE:
            sprintf(out_str, "�յ����� %s �ĺ���!", uac_e->request->from->url->string);
            printf("%s\n", out_str);

            eXosip_lock(excontext);
            eXosip_call_send_answer(excontext, uac_e->tid, 180, NULL);
            if (0 != eXosip_call_build_answer(excontext, uac_e->tid, 200, &answer))
            {
                eXosip_call_send_answer(excontext, uac_e->tid, 603, NULL);
                printf("error build answer!");
                continue;
            }
            eXosip_unlock(excontext);

            call_id = uac_e->cid; //���Ҷϵ绰�����Ĳ��� 
            dialog_id = uac_e->did;

            /* ��������SDP��Ϣ��ý�彨�� */
            snprintf(tmp, 4096,
                "v=0\r\n"
                "o=youtoo 1 1 IN IP4 %s\r\n"
                "s=##youtoo demo\r\n"
                "c=IN IP4 %s\r\n"
                "t=0 0\r\n"
                "m=audio %d RTP/AVP 0 8 101\r\n"
                "a=rtpmap:0 PCMU/8000\r\n"
                "a=rtpmap:8 PCMA/8000\r\n"
                "a=rtpmap:101 telephone-event/8000\r\n"
                "a=fmtp:101 0-15\r\n", localip, localip, rtp_port);

            //���ûظ���SDP��Ϣ��,��һ���ƻ�������Ϣ�� 
            eXosip_lock(excontext);
            osip_message_set_body(answer, tmp, strlen(tmp));
            osip_message_set_content_type(answer, "application/sdp");

            /* ����SDP */
            msg_req = eXosip_get_remote_sdp(excontext, uac_e->did);
            con_req = eXosip_get_audio_connection(msg_req);
            md_req = eXosip_get_audio_media(msg_req);
            eXosip_unlock(excontext);

            /*
            payload_str = (char *)osip_list_get(&md_req->m_payloads, 0); //ȡ����ý����������
            //��ʱֻ֧��0/8 711u/711a
            */

            calling = 1;

            while (!picked)
            {
                //δ��ͨ����ѭ����� 
                Sleep(200);
            }

            eXosip_unlock(excontext);
            eXosip_call_send_answer(excontext, uac_e->tid, 200, answer);
            eXosip_unlock(excontext);

            printf("200 ok ����");
            break;

        case EXOSIP_CALL_CANCELLED:
            /* �ܾ����� */

            call_id = 0;
            dialog_id = 0;
            picked = 0;
            calling = 0;
            printf("���оܾ�����\n");
            break;

        case EXOSIP_CALL_ACK:
            /* ����200���յ�ack�Ž���ý�� */
            if (calling)
            {
                /* ����RTP���� */
                ret = build_media(rtp_port, con_req->c_addr, atoi(md_req->m_port), 0, NULL, 0, 0, 0);
                if (!ret)
                {
                    printf("ý�彨��ʧ�ܣ��޷�����ͨ������Ҷϣ�");
                    //pMainWnd->OnHang(); 
                }
            }
            break;

            /* UAC �����¼� */
        case EXOSIP_REGISTRATION_FAILURE:
            if (401 == uac_e->response->status_code)
            {
                //4o1 Unauthorized register again 
                printf("register again!\n");
                /* �������Ȩ��Ϣ����Ӧ��������ļ�Ȩ��Ϣ */
                eXosip_clear_authentication_info(excontext);
                eXosip_add_authentication_info(excontext, username, username, password, "md5", NULL);

                osip_message_t *rereg;
                eXosip_lock(excontext);
                eXosip_register_build_register(excontext, uac_e->rid, 300, &rereg);// 
                                                                                   //ȡ����֤���ַ���authorization 
                                                                                   //osip_authorization_t *auth; 
                                                                                   ///char *strAuth; 
                                                                                   //osip_message_get_authorization((const osip_message_t *)rereg,0,&auth); 
                                                                                   //osip_authorization_to_str(auth,&strAuth); 
                                                                                   //strcpy(str_auth,strAuth);//������֤�ַ��� 
                eXosip_register_send_register(excontext, uac_e->rid, rereg);
                eXosip_unlock(excontext);
            }
            break;
        case EXOSIP_REGISTRATION_SUCCESS:
            printf("textinfo is %s\n", uac_e->textinfo);
            break;

        case EXOSIP_CALL_CLOSED:
            if (audio)
            {
                //�����ر�ý������(Զ�˴���) 
                eXosip_lock(excontext);
                audio_stream_stop(audio);

                ortp_ev_queue_destroy(q);
                rtp_profile_destroy(profile);
                eXosip_unlock(excontext);

                audio = NULL;
                q = NULL;
                profile = NULL;
                printf("audio stream stoped!");
            }

            printf("(�Է��ѹҶ�)");
            call_id = 0;
            dialog_id = 0;
            picked = 0;
            calling = 0;
            break;

        case EXOSIP_CALL_PROCEEDING:
            printf("(����������..)");
            break;

        case EXOSIP_CALL_RINGING:
            printf("(�Է�����)");
            call_id = uac_e->cid;
            dialog_id = uac_e->did;

            /*
            RingStream *r;
            MSSndCard *sc;
            sc=ms_snd_card_manager_get_default_card(ms_snd_card_manager_get());
            r=ring_start("D://mbstudio//vcwork//YouToo//dial.wav",2000,sc);

            Sleep(10);
            ring_stop(r);
            */
            break;

        case EXOSIP_CALL_ANSWERED:
            //ring_stop(ring_p); 
            printf("(�Է��ѽ���)");

            call_id = uac_e->cid;
            dialog_id = uac_e->did;

            eXosip_lock(excontext);
            eXosip_call_build_ack(excontext, uac_e->did, &ack);
            eXosip_call_send_ack(excontext, uac_e->did, ack);

            /* ��ӦSIP��Ϣ��SDP���� */
            msg_rsp = eXosip_get_sdp_info(uac_e->response);
            con_rsp = eXosip_get_audio_connection(msg_rsp);
            md_rsp = eXosip_get_audio_media(msg_rsp);

            /* ȡ������֧�ֵ������ȵı��뷽ʽ */
            payload_str = (char *)osip_list_get(&md_rsp->m_payloads, 0);
            eXosip_unlock(excontext);

            /* ����RTP���� */
            ret = build_media(rtp_port, con_rsp->c_addr, atoi(md_rsp->m_port), atoi(payload_str), NULL, 0, 0, 0);
            if (!ret)
            {
                printf("ý�彨��ʧ�ܣ��޷�����ͨ������Ҷϣ�");
                //pMainWnd->OnHang(); 
            }
            break;

        default:
            break;
        }

        eXosip_event_free(uac_e);

        fflush(stdout);
    }

    return 0;
}

unsigned _stdcall sip_ua_monitor_thread(void*) {
    return sip_ua_monitor();
}




int build_media(int local_port, const char *remote_ip, int remote_port, int payload, const char *fmtp, int jitter, int ec, int bitrate)
{
    if (payload != 0 && payload != 8)
    {
        /* Ŀǰ��֧��0,8 711ulaw,711alaw */
        return -1;
    }

    PayloadType *pt;

    profile = rtp_profile_clone_full(&av_profile);
    q = ortp_ev_queue_new();

    pt = rtp_profile_get_payload(profile, payload);
    if (pt == NULL)
    {
        printf("codec error!");
        return -1;
    }

    if (fmtp != NULL) payload_type_set_send_fmtp(pt, fmtp);
    if (bitrate > 0) pt->normal_bitrate = bitrate;

    if (pt->type != PAYLOAD_VIDEO)
    {
        printf("audio stream start!");
        audio = audio_stream_start(profile, local_port, remote_ip, remote_port, payload, jitter, ec);
        if (audio)
        {
            //~session = audio->session;
            audio->ms.sessions.rtp_session;
        }
        else
        {
            printf("session error!");
            return -1;
        }
    }
    else
    {
        printf("codec select error!");
        return -1;
    }

    rtp_session_register_event_queue(session, q);
    return 0;
}

int real_send_register(int expires)
{
    char identity[100];
    char registerer[100];
    sprintf(identity, "sip:%s@%s:%s", username, localip, local_port);
    sprintf(registerer, "sip:%s:%s", server_url, server_port);

    osip_message_t *reg = NULL;

    eXosip_lock(excontext);
    int ret = eXosip_register_build_initial_register(excontext, identity, registerer, NULL, expires, &reg);
    eXosip_unlock(excontext);
    if (0 > ret)
    {
        printf("register init Failed!");
        return -1;
    }

    eXosip_lock(excontext);
    eXosip_clear_authentication_info(excontext); //ȥ���ϴμ���Ĵ�����֤��Ϣ 
    eXosip_add_authentication_info(excontext, username, username, password, "md5", NULL);
    ret = eXosip_register_send_register(excontext, ret, reg);
    eXosip_unlock(excontext);
    if (0 != ret)
    {
        printf("register send Failed!");
        return -1;
    }

    return 0;
}


// �Ҷ�
void OnHang()
{
    int ret;

    if (audio)
    {
        //�����ر�ý������(���ض�) 
        eXosip_lock(excontext);
        audio_stream_stop(audio);

        ortp_ev_queue_destroy(q);
        rtp_profile_destroy(profile);
        eXosip_unlock(excontext);

        printf("audio stream stoped!");
        audio = NULL;
        q = NULL;
        profile = NULL;
    }

    eXosip_lock(excontext);
    ret = eXosip_call_terminate(excontext, call_id, dialog_id);
    eXosip_unlock(excontext);
    if (0 != ret)
    {
        printf("hangup terminate Failed!\n");
    }
    else
    {
        printf("(�ѹҶ�)");
        call_id = 0;
        dialog_id = 0;
        picked = 0;
        calling = 0;
    }
}


void __stdcall MyCbSipCallback(osip_message_t * msg, int received)
{
    char* dest;
    size_t length;
    osip_message_to_str(msg, &dest, &length);

    printf("\n\t\t\t\t\t <#######[%s] sip_method: %s, status_code: %d  %s \n%s\n#######>\n",
        (received == 1) ? "recv" : "send", msg->sip_method, msg->status_code, msg->reason_phrase,
        dest);
}

int my_sip_init(bool isAsServer)
{
    int ret = 0;

    excontext = eXosip_malloc();
    ret = eXosip_init(excontext);
    if (0 != ret)
    {
        printf("Couldn't initialize eXosip!\n");
        return -1;
    }

    eXosip_set_user_agent(excontext, "##YouToo0.1");


    eXosip_set_cbsip_message(excontext, MyCbSipCallback);

    int port = isAsServer ? i_server_port : i_local_port;
    ret = eXosip_listen_addr(excontext, IPPROTO_UDP, NULL, port, AF_INET, 0);
    if (0 != ret) {
        eXosip_quit(excontext);
        printf("Couldn't initialize transport layer!\n");
        return -1;
    }

    //AfxBeginThread(sip_ua_monitor,(void *)this); 
#ifdef _MSC_VER
    _beginthreadex(NULL, 0, sip_ua_monitor_thread, NULL, 0, NULL);
#else
    pthread_t id;
    pthread_create(&id, NULL, (void *)sip_ua_monitor, NULL);
#endif // _MSC_VER

    /* rtp */
    ortp_init();
    //ortp_set_log_level_mask(ORTP_MESSAGE|ORTP_WARNING|ORTP_ERROR|ORTP_FATAL); 

    /* media */
    ms_init();

    return 0;
}
