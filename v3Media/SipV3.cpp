/**   @file    SipV3.cpp
*    @note    All Right Reserved.
*    @brief   v3平台稀有gb协议。
*    @author   陈晓焱
*    @date     2020/10/29
*
*    @note    下面的note和warning为可选项目
*    @note
*    @note    历史记录：
*    @note    V0.0.1  创建
*
*    @warning 这里填写本文件相关的警告信息
*/
#include "SipV3.h"

#define UA_STRING "VTDU"

//回调函数以及用户参数
fnSipUAEventCB g_fnEventCB = NULL;
void *g_pEventCBParam = NULL;

//sip服务配置
ConfigSipServer g_stSipCfg;

//事件接收线程工作标志
bool g_bEventThreadWorking = false;

/**************************************************************************
* name          : SipUA_AnswerInfo
* description   : 回复 sip请求消息
* input         : pMsgPtr  收到的请求消息
*                 status   状态码
*                 pszBody  sip消息体
*                 iBodyLen sip消息体长度
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_AnswerInfo(void *pMsgPtr, int status, const char *pszBody, int iBodyLen)
{
    eXosip_event_t *evt = (eXosip_event_t *)pMsgPtr;
    osip_message_t *answer = NULL;
    int ret = eXosip_message_build_answer(evt->tid, status, &answer);

    char contact[256] = { 0 };//sip:122205106000000001@122205:5063
    sprintf(contact, "sip:%s@%s:%d", g_stSipCfg.m_strLocalSipId.c_str(), g_stSipCfg.m_strServerDomain.c_str(), g_stSipCfg.m_iSipPort);
    if (ret != 0 || answer == NULL)
    {
        VTDU_LOG_E("eXosip_message_build_answer failed. ret: " << ret);
        return ret;
    }
    osip_message_set_header(answer, "Contact", contact);
    if (iBodyLen > 0)
    {
        osip_message_set_body(answer, pszBody, iBodyLen);
        osip_message_set_content_type(answer, "xml/txt");
    }
    answer->v3_head = NULL;
    ret = eXosip_message_send_answer(evt->tid, status, answer);

    return ret;
}

/*回调呼叫类消息*/
static void callbackCallMsg(eXosip_event_t *evt, ESipUAMsg msg)
{
    if (g_fnEventCB)
    {
        g_fnEventCB(msg, (void*)evt, g_pEventCBParam);
}
}

//sip 事件处理函数
#ifdef WIN32
unsigned int WINAPI threadEventLoop(void *pParam)
#else
static void *threadEventLoop(void *pParam)
#endif
{
    while (g_bEventThreadWorking)
    {
        eXosip_event_t *evt = NULL;
        if (!(evt = eXosip_event_wait(0, 50))) {
            osip_usleep(10000);
            continue;
        }

        //if (evt->type >= 0 && evt->type <= EXOSIP_EVENT_COUNT)
        //{
        //    printf("recv sip msg= , tid=%d, did=%d, rid=%d, cid=%d, sid=%d, nid=%d\n",
        //        evt->tid, evt->did, evt->rid, evt->cid, evt->sid, evt->nid);
        //}
        //else
        //{
        //    printf("recv sip msg=%d, out of range.\n", evt->type);
        //}

        eXosip_lock();
        // 自动处理401
        //eXosip_default_action(evt);
        eXosip_automatic_refresh();
        eXosip_unlock();

        switch (evt->type)
        {
            /*注册类消息*/
        case EXOSIP_REGISTRATION_NEW:
            break;

        case EXOSIP_REGISTRATION_SUCCESS:
            callbackCallMsg(evt, SIPUA_MSG_REGISTER_SUCCESS);
            break;

        case EXOSIP_REGISTRATION_FAILURE:
            callbackCallMsg(evt, SIPUA_MSG_REGISTER_FAILURE);
            break;

        case EXOSIP_REGISTRATION_REFRESHED:
            break;

            /*呼叫类消息*/
        case EXOSIP_CALL_INVITE: //invite
            break;

        case EXOSIP_CALL_ANSWERED: //invite响应
        case EXOSIP_CALL_REDIRECTED:
        case EXOSIP_CALL_REQUESTFAILURE:
        case EXOSIP_CALL_SERVERFAILURE:
        case EXOSIP_CALL_GLOBALFAILURE:
            break;

        case EXOSIP_CALL_ACK: //ack
            break;

        case EXOSIP_CALL_CLOSED: //呼叫关闭
            break;

            /*MESSAGE类消息*/
        case EXOSIP_MESSAGE_NEW:
            if (MSG_IS_REGISTER(evt->request))  /*注册消息*/
            {
                SipUA_AnswerInfo(evt, 200, NULL, 0);
            }
            else if (MSG_IS_MESSAGE(evt->request))
            {
                SipUA_AnswerInfo(evt, 200, NULL, 0);
            }
            else if (MSG_IS_INFO(evt->request))
            {
                callbackCallMsg(evt, SIPUA_MSG_INFO);
            }
            else if (MSG_IS_OPTIONS(evt->request))
            {
                SipUA_AnswerInfo(evt, 200, NULL, 0);
            }
            break;

        case EXOSIP_MESSAGE_ANSWERED: //MESSAGE 200 ok
            break;

        case EXOSIP_MESSAGE_REQUESTFAILURE: //MESSAGE 400响应
            //是否是options心跳 如果是的话判断为断开连接，启动注册线程重新注册 停止心跳线程
            if (MSG_IS_OPTIONS(evt->request))
            {
                callbackCallMsg(evt, SIPUA_MSG_OPTION_REQUESTFAILURE);  
            }
            break;

            /*订阅类消息*/
        case EXOSIP_SUBSCRIPTION_UPDATE:
            break;

        default:
            break;

        }
        //SipUA_AnswerInfo(evt, 200, NULL, 0);

        eXosip_event_free(evt);
    }

    return 0;
}

/**************************************************************************
* name          : SipUA_Init
* description   : 初始化
* input         : stSipCfg sip配置信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_Init(const ConfigSipServer& stSipCfg)
{
    g_stSipCfg = stSipCfg;
    int ret = eXosip_init();
    if (0 != ret)
    {
        VTDU_LOG_E("eXosip_init failed. ret: " << ret);
        return -1;
    }
    eXosip_set_user_agent(UA_STRING);

    int proto = IPPROTO_TCP;
    //int proto = IPPROTO_UDP;

    ret = eXosip_listen_addr(proto, stSipCfg.m_strSipAddr.c_str(), stSipCfg.m_iSipPort, AF_INET, 0);
    if (0 != ret)
    {
        eXosip_quit();
        VTDU_LOG_E("eXosip_listen_addr failed. ret: " << ret << ", proto: " << proto << ", ip: " << stSipCfg.m_strSipAddr << ", port:" << stSipCfg.m_iSipPort);
        return -1;
    }
    
    return 0;
}

/**************************************************************************
* name          : SipUA_Fini
* description   : 反初始化
* input         : NA
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_Fini()
{
    eXosip_quit();
    return 0;
}

/**************************************************************************
* name          : SipUA_StartEventLoop
* description   : 开始SIPUA事件
* input         : fnCB 回调函数
*                 pParam 用户参数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_StartEventLoop(fnSipUAEventCB fnCB, void *pParam)
{
    g_fnEventCB = fnCB;
    g_pEventCBParam = pParam;

    g_bEventThreadWorking = true;
#ifdef WIN32
    HANDLE hWorkingThread = (HANDLE)_beginthreadex(
        NULL,
        0,
        threadEventLoop,
        NULL,
        0,
        NULL);
#else
    pthread_t tThreadId;
    pthread_create(&tThreadId, 0, threadEventLoop, NULL);
#endif
    return 0;
}

/**************************************************************************
* name          : SipUA_StartEventLoop
* description   : 结束SIPUA事件
* input         : NA
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_StopEventLoop()
{
    if (true == g_bEventThreadWorking)
    {
        g_bEventThreadWorking = false;
    }
    return 0;
}

/**************************************************************************
* name          : SipUA_InitMessage
* description   : 发送sip请求
* input         : pszMethod sip请求类型 OPTION/INFO/MESSAGE
*                 pszSrcId 本地sip id
*                 pszSrcIP 本地 ip
*                 iSrcPort 本地 sip 端口
*                 pszDestId 目标 sip id
*                 pszDestIP 目标 sip ip
*                 iDestPort 目标 sip 端口
*                 pszBody sip 消息体
*                 iBodyLen sip 消息体长度
*                 strV3Head sip v3头
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_InitMessage(const char * pszMethod, const char *pszSrcId, const char *pszSrcIP, int iSrcPort, const char *pszDestId, const char *pszDestIP, int iDestPort,
    const char *pszBody, int iBodyLen, std::string strV3Head)
{

    char szSrc[1024] = { 0 };
    char szDest[1024] = { 0 };
    sprintf(szSrc, "sip:%s@%s:%d", pszSrcId, pszSrcIP, iSrcPort);
    sprintf(szDest, "sip:%s@%s:%d", pszDestId, pszDestIP, iDestPort);

    char contact[256] = { 0 };//sip:122205106000000001@122205:5063
    sprintf(contact, "sip:%s@%s:%d", pszSrcId, pszSrcIP, iSrcPort);

    osip_message_t *message = NULL;
    int ret = eXosip_message_build_request(&message, pszMethod, szDest, szSrc, NULL);
    if (0 != ret)
    {
        VTDU_LOG_E("build message request failed, src: " << szSrc << ", dest:" << szDest);
        return -1;
    }
    osip_message_set_header(message, "Contact", contact);
    osip_message_set_body(message, pszBody, iBodyLen);
    osip_message_set_content_type(message, "txt/xml");

    if ("" != strV3Head)
    {
        message->v3_head = (char*)(strV3Head.c_str());
    }
    else
    {
        message->v3_head = NULL;
    }

    ret = eXosip_message_send_request(message);
    return ret;
}

/**************************************************************************
* name          : SipUA_Register
* description   : 发送 sip注册
* input         : pszSipId        本地sip id
*                 pszSipRegion    本地sip Region
*                 pszSipAddr      本地 ip
*                 iSipPort        本地 sip 端口
*                 expires         注册有效期
*                 pszServerId     目标 sip id
*                 pszServerRegion sip服务器Region
*                 pszServerIP     sip服务器 ip
*                 iServerPort     sip服务器 端口
*                 pszBody         sip消息体
*                 iBodyLen        sip消息体长度
*                 nRegid          sip 注册id 如果不是-1代表已注册
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_Register(const char *pszSipId, const char * pszSipRegion, const char *pszSipAddr, int iSipPort, int expires,
    const char *pszServerId, const char * pszServerRegion, const char *pszServerIP, int iServerPort,
    const char *pszBody, int iBodyLen, int nRegid)
{
    int ret = 0;
    char fromuser[256] = { 0 };
    sprintf(fromuser, "sip:%s@%s:%d", pszSipId, pszSipAddr, iSipPort);
    char proxy[256] = { 0 };
    sprintf(proxy, "sip:%s@%s:%d", pszServerId, pszServerIP, iServerPort);
    char contact[256] = { 0 };//sip:122205106000000001@122205:5063
    sprintf(contact, "sip:%s@%s:%d", pszSipId, pszSipRegion, iSipPort);
    //const char *contact = NULL; 
    osip_message_t *reg = NULL;

    if (nRegid != -1)
    {
        eXosip_register_build_register(nRegid, expires, &reg);
    }
    else
    {

        nRegid = eXosip_register_build_initial_register(fromuser, proxy, contact, expires, &reg);
        if (nRegid < 1)
        {
            VTDU_LOG_E("init register failed, from: " << fromuser << ", proxy:" << proxy << ", contact:" << contact << ", ret:" << nRegid);
            return -1;
        }
    }

    //设置消息体
    if (pszBody && iBodyLen > 0)
    {
        ret = osip_message_set_body(reg, pszBody, iBodyLen);
        osip_message_set_content_type(reg, "txt/xml");
        if (0 != ret)
        {
            VTDU_LOG_E("set body failed, bodylen: " << iBodyLen << ", Body :" << pszBody);
            return -1;
        }
    }

    char *szV3_head = new char[30];
    memset(szV3_head, 0, 30);
    sprintf(szV3_head, "%s", ";MSG_TYPE=MSG_VTDU_LOGIN_REQ");
    reg->v3_head = szV3_head;
    ret = eXosip_register_send_register(nRegid, reg);
    if (ret != 0)
    {
        VTDU_LOG_E("send register failed, from: " << fromuser << ", proxy :" << proxy);
        return -1;
    }
    return nRegid;
}

/**************************************************************************
* name          : SipUA_GetRequestBodyContent
* description   : 获取v3请求消息体
* input         : pMsgPtr  收到的请求消息
*                 iMaxBodyLen 输出消息体最大长度
* output        : pszOutBody   输出消息体
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_GetRequestBodyContent(void *pMsgPtr, char *pszOutBody, int iMaxBodyLen)
{

    eXosip_event_t *evt = (eXosip_event_t *)pMsgPtr;
    if (NULL == evt->request)
    {
        VTDU_LOG_E("SipUA_GetRequestBodyContent get request body failed, request null.");
        return -1;
    }

    int ret = 0;
    osip_body_t *body = NULL;
    osip_message_get_body(evt->request, 0, &body);
    if (body)
    {
        ret = (int)body->length - 38 - 25 - 10;
        if (ret > 0 && ret <= iMaxBodyLen)
        {
            memcpy(pszOutBody, body->body + 38 + 25, ret);
        }
        else
        {
            VTDU_LOG_E("SipUA_GetRequestBodyContent get request body failed, size: " << ret << ", out of range:" << iMaxBodyLen);
        }
    }
    else
    {
        VTDU_LOG_E("osip_message_get_body failed, body is null.");
        return -1;
    }

    return ret;
}

/**************************************************************************
* name          : SipUA_GetResponseBodyContent
* description   : 获取v3回复消息体
* input         : pMsgPtr  收到的请求消息
*                 iMaxBodyLen 输出消息体最大长度
* output        : pszOutBody   输出消息体
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_GetResponseBodyContent(void *pMsgPtr, char *pszOutBody, int iMaxBodyLen)
{

    eXosip_event_t *evt = (eXosip_event_t *)pMsgPtr;
    if (NULL == evt->response)
    {
        VTDU_LOG_E("get response body failed, request null.");
        return -1;
    }

    int ret = 0;
    osip_body_t *body = NULL;
    osip_message_get_body(evt->response, 0, &body);
    if (body)
    {
        ret = (int)body->length - 38 - 25 - 10;
        if (ret > 0 && ret <= iMaxBodyLen)
        {
            memcpy(pszOutBody, body->body + 38 + 25, ret);
        }
        else
        {
            VTDU_LOG_E("get request body failed, size: " << ret << ", out of range:" << iMaxBodyLen);
        }
    }

    return ret;
}

/**************************************************************************
* name          : SipUA_RspPreviewOk
* description   : 回复v3预览请求成功消息
* input         : pMsgPtr  收到的v3 sip请求消息
*                 stCurMediaInfo 预览请求信息
*                 nInitialUser 接收方路数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RspPreviewOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo, int nInitialUser)
{
    char pszBody[4096] = { 0 };
    int iBodyLen = 0;

    iBodyLen = sprintf(pszBody, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        "<Message Version=\"1.0.2\">\r\n"
        "<IE_HEADER>\r\n"
        "<MessageType>MSG_READY_VIDEO_TRANS_RESP</MessageType>\r\n"
        "</IE_HEADER>\r\n"
        "<IE_CU_ADDRINFO>\r\n"
        "<UserID>%s</UserID>\r\n"
        "<IP>%s</IP>\r\n"
        "<Port>%d</Port>\r\n"
        "<Nat>%d</Nat>\r\n"
        "<TransType>%d</TransType>\r\n"
        "<MSCheck>%d</MSCheck>\r\n"
        "<VtduToCuPort>%d</VtduToCuPort>\r\n"
        "<InitialUser>%d</InitialUser>\r\n"
        "</IE_CU_ADDRINFO>\r\n"
        "<IE_PU_ADDRINFO>\r\n"
        "<PUID>%s</PUID>\r\n"
        "<ChannelID>%d</ChannelID>\r\n"
        "<Protocol>%d</Protocol>\r\n"
        "<StreamType>%d</StreamType>\r\n"
        "<TransType>%d</TransType>\r\n"
        "<FCode>%d</FCode>\r\n"
        "<PuToVtduPort>%d</PuToVtduPort>\r\n"
        "</IE_PU_ADDRINFO>\r\n"
        "<IE_VTDU_ROUTE>\r\n"
        "<RecvIP>%s</RecvIP>\r\n"
        "<SendIP>%s</SendIP>\r\n"
        "</IE_VTDU_ROUTE>\r\n"
        "<IE_OPEN_VIDEO>\r\n"
        "<StreamType>%d</StreamType> \r\n"
        "<GBVideoTransMode>%s</GBVideoTransMode>\r\n"
        "</IE_OPEN_VIDEO>\r\n"
        "<IE_RESULT>\r\n"
        "<ErrorCode>0</ErrorCode>\r\n"
        "<ErrorMessage/>\r\n"
        "</IE_RESULT>\r\n"
        "<VTDU_LOAD_STATE>\r\n"
        "<State/>\r\n"
        "</VTDU_LOAD_STATE>\r\n"
        "</Message>\r\n", stCurMediaInfo.strCuUserID.c_str(), stCurMediaInfo.strCuIp.c_str(), stCurMediaInfo.nCuport, stCurMediaInfo.nCuNat, stCurMediaInfo.nCuTransType, stCurMediaInfo.nMSCheck,
        stCurMediaInfo.nVtduSendPort, nInitialUser, stCurMediaInfo.strPUID.c_str(), stCurMediaInfo.nPuChannelID, stCurMediaInfo.nPuProtocol, stCurMediaInfo.nPuStreamType, stCurMediaInfo.nPuTransType,
        stCurMediaInfo.nPuFCode, stCurMediaInfo.nVtduRecvPort, stCurMediaInfo.strVtduRecvIp.c_str(), stCurMediaInfo.strVtduSendIP.c_str(), stCurMediaInfo.nOpenVideoStreamType,
        stCurMediaInfo.strOpenVideoGBVideoTransMode.c_str());

    SipUA_AnswerInfo(pMsgPtr, 200, pszBody, iBodyLen);
    return 0;
}

/**************************************************************************
* name          : SipUA_RspPreviewStopOk
* description   : 回复v3停止预览请求成功消息
* input         : pMsgPtr  收到的v3 sip请求消息
*                 stCurMediaInfo 预览请求信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RspPreviewStopOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo)
{
    char pszBody[4096] = { 0 };
    int iBodyLen = 0;
    iBodyLen = sprintf(pszBody, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        "<Message Version=\"1.0.2\">\r\n"
        "<IE_HEADER>\r\n"
        "<MessageType>MSG_VTDU_STOP_VIDEO_RESP</MessageType>\r\n"
        "</IE_HEADER>\r\n"
        "<IE_STOP_VIDEO>\r\n"
        "<PUID>%s</PUID>\r\n"
        "<ChannelID>%d</ChannelID>\r\n"
        "<StreamType>%d</StreamType>\r\n"
        "</IE_STOP_VIDEO>\r\n"
        "<IE_USER_INFO>\r\n"
        "<UserID>%s</UserID>\r\n"
        "</IE_USER_INFO>\r\n"
        "<IE_RESULT>\r\n"
        "<ErrorCode>0</ErrorCode>\r\n"
        "<ErrorMessage/>\r\n"
        "</IE_RESULT>\r\n"
        "<VTDU_LOAD_STATE>\r\n"
        "<State/>\r\n"
        "</VTDU_LOAD_STATE>\r\n"
        "</Message>\r\n", stCurMediaInfo.strPUID.c_str(), stCurMediaInfo.nPuChannelID, stCurMediaInfo.nPuStreamType, stCurMediaInfo.strCuUserID.c_str());

    SipUA_AnswerInfo(pMsgPtr, 200, pszBody, iBodyLen);
    return 0;
}

/**************************************************************************
* name          : SipUA_RspPlayBackOk
* description   : 回复v3回放请求成功消息
* input         : pMsgPtr  收到的v3 sip请求消息
*                 stCurMediaInfo 预览请求信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RspPlayBackOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo)
{
    char pszBody[4096] = { 0 };
    int iBodyLen = 0;

    iBodyLen = sprintf(pszBody, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        "<Message Version=\"1.0.2\">\r\n"
        "<IE_HEADER>\r\n"
        "<MessageType>MSG_START_FILE_VOD_TASK_RESP</MessageType>\r\n"
        "</IE_HEADER>\r\n"
        "<IE_RTSP_INFO>\r\n"
        "<Session>%s</Session>\r\n"
        "<ClientIP>%s</ClientIP>\r\n"
        "<ClientPorts>%s</ClientPorts>\r\n"
        "<TransMode>%d</TransMode>\r\n"
        "<UserID>%s</UserID>\r\n"
        "</IE_RTSP_INFO>\r\n"
        "<IE_VTDU_ROUTE>\r\n"
        "<RecvIP>%s</RecvIP>\r\n"
        "<SendIP>%s</SendIP>\r\n"
        "<RcvPURTPPort>%d</RcvPURTPPort>\r\n"
        "<RcvPURTCPPort>%d</RcvPURTCPPort>\r\n"
        "<SendCURTPPort>%d</SendCURTPPort>\r\n"
        "<SendCURTCPPort>%d</SendCURTCPPort>\r\n"
        "</IE_VTDU_ROUTE>\r\n"
        "<IE_NETLINK>\r\n"
        "<TransType>%d</TransType> \r\n"
        "</IE_NETLINK>\r\n"
        "<IE_STREAM>\r\n"
        "<MediaType>%d</MediaType> \r\n"
        "<MSCheck>%d</MSCheck> \r\n"
        "<Protocol>%d</Protocol> \r\n"
        "</IE_STREAM>\r\n"
        "<IE_RESULT>\r\n"
        "<ErrorCode>0</ErrorCode>\r\n"
        "<ErrorMessage/>\r\n"
        "</IE_RESULT>\r\n"
        "<VTDU_LOAD_STATE>\r\n"
        "<State/>\r\n"
        "</VTDU_LOAD_STATE>\r\n"
        "</Message>\r\n", stCurMediaInfo.strCuSession.c_str(), stCurMediaInfo.strCuIp.c_str(), stCurMediaInfo.strCuPort.c_str(), stCurMediaInfo.nCuTranmode, stCurMediaInfo.strCuUserID.c_str(),
        stCurMediaInfo.strVtduRecvIp.c_str(), stCurMediaInfo.strVtduSendIP.c_str(), stCurMediaInfo.nVtduRecvPort, stCurMediaInfo.nVtduRecvPort + 1, stCurMediaInfo.nVtduSendPort, stCurMediaInfo.nVtduSendPort + 1,
        stCurMediaInfo.nCuTransType, stCurMediaInfo.nMediaType, stCurMediaInfo.nMSCheck, stCurMediaInfo.nPuProtocol);
    //回复状态
    SipUA_AnswerInfo(pMsgPtr, 200, pszBody, iBodyLen);

    return 0;
}

/**************************************************************************
* name          : SipUA_RspPlayBackStopOk
* description   : 回复v3停止回放请求成功消息
* input         : pMsgPtr  收到的v3 sip请求消息
*                 stCurMediaInfo 预览请求信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RspPlayBackStopOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo)
{
    char pszBody[4096] = { 0 };
    int iBodyLen = 0;

    iBodyLen = sprintf(pszBody, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        "<Message Version=\"1.0.2\">\r\n"
        "<IE_HEADER>\r\n"
        "<MessageType>STOP_FILE_VOD_TASK_RESP</MessageType>\r\n"
        "</IE_HEADER>\r\n"
        "<IE_RTSP_INFO>\r\n"
        "<Session>%s</Session>\r\n"
        "<UserID>%s</UserID>\r\n"
        "</IE_RTSP_INFO>\r\n"
        "<IE_RESULT>\r\n"
        "<ErrorCode>0</ErrorCode>\r\n"
        "<ErrorMessage/>\r\n"
        "</IE_RESULT>\r\n"
        "<VTDU_LOAD_STATE>\r\n"
        "<State/>\r\n"
        "</VTDU_LOAD_STATE>\r\n"
        "</Message>\r\n", stCurMediaInfo.strCuSession.c_str(), stCurMediaInfo.strCuUserID.c_str());

    SipUA_AnswerInfo(pMsgPtr, 200, pszBody, iBodyLen);
    return 0;
}

/**************************************************************************
* name          : SipUA_RegisterV3
* description   : 发送v3 注册请求
* input         : configSipServer  vtdu配置信息
*                 bNeedBody 是否需要组装body
*                 nRegid 注册id 如果大于0 代表是注册更新
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RegisterV3(const ConfigSipServer &configSipServer, bool bNeedBody, int nRegid)
{
    int nRegidCur = -1;
    if (!bNeedBody)
    {
        nRegidCur = SipUA_Register(
            configSipServer.m_strLocalSipId.c_str(), configSipServer.m_strSipRegion.c_str(), configSipServer.m_strSipAddr.c_str(), configSipServer.m_iSipPort, configSipServer.m_iExpires,
            configSipServer.m_strServerId.c_str(), configSipServer.m_strServerRegion.c_str(), configSipServer.m_strServerIP.c_str(),
            configSipServer.m_iServerPort, NULL, 0, nRegid);
    }
    else
    {
        char szBody[4096] = { 0 };
        int iBodyLen = sprintf(szBody,
            "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
            "<Message Version=\"1.0.2\">\r\n"
            "<IE_HEADER>\r\n"
            "<MessageType>MSG_VTDU_LOGIN_REQ</MessageType>\r\n"
            "<Reserved />\r\n"
            "</IE_HEADER>\r\n"
            "<IE_SERVER_REGISTER_INFO>\r\n"
            "<ServerType>VTDU</ServerType>\r\n"
            "<ServerID>%s</ServerID>\r\n"
            "<DomainID>%s</DomainID>\r\n"
            "<ServerIP>%s</ServerIP>\r\n"
            "<Version>3.1.1.13</Version>\r\n"
            "</IE_SERVER_REGISTER_INFO>\r\n"
            "<IE_VTDU_PARAM>\r\n"
            "<maxAudioInAmount>200</maxAudioInAmount>\r\n"
            "<maxVideoInAmount>200</maxVideoInAmount>\r\n"
            "<maxAudioOutAmount>200</maxAudioOutAmount>\r\n"
            "<maxVideoOutAmount>200</maxVideoOutAmount>\r\n"
            "<xDomRcvIP>%s</xDomRcvIP>\r\n"
            "</IE_VTDU_PARAM>\r\n"
            "<IE_IO_OPTION>\r\n"
            "<IP_NUM>1</IP_NUM>\r\n"
            "<IP_RECORD>\r\n"
            "<IP>%s</IP>\r\n"
            "<IN_BW>100000</IN_BW >\r\n"
            "<OUT_BW>100000</OUT_BW >\r\n"
            "</IP_RECORD>\r\n"
            "</IE_IO_OPTION>\r\n"
            "<IE_DOMAIN_LIST>\r\n"
            "<DomNUM>1</DomNUM>\r\n"
            "<Domain>\r\n"
            "<DomainID>%s</DomainID>\r\n"
            "</Domain>\r\n"
            "</IE_DOMAIN_LIST>\r\n"
            "</Message>\r\n", configSipServer.m_strLocalSipId.c_str(), configSipServer.m_strSipDomain.c_str(), configSipServer.m_strSipAddr.c_str(), configSipServer.m_strSipAddr.c_str(), configSipServer.m_strSipAddr.c_str(), configSipServer.m_strSipDomain.c_str());

        nRegidCur = SipUA_Register(
            configSipServer.m_strLocalSipId.c_str(), configSipServer.m_strSipRegion.c_str(), configSipServer.m_strSipAddr.c_str(), configSipServer.m_iSipPort, configSipServer.m_iExpires,
            configSipServer.m_strServerId.c_str(), configSipServer.m_strServerRegion.c_str(), configSipServer.m_strServerIP.c_str(),
            configSipServer.m_iServerPort, szBody, iBodyLen, nRegid);
    }

    return nRegidCur;
}

/**************************************************************************
* name          : SipUA_Timeout
* description   : 发送v3 断流超时
* input         : stCurMediaInfo  任务信息
*                 configSipServer  vtdu配置信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_Timeout(const stMediaInfo &stCurMediaInfo, const ConfigSipServer &configSipServer)
{
    char szKeepaliveXml[1024] = { 0 };
    int iXmlLen = sprintf(szKeepaliveXml,
        "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        "<Message Version=\"1.0.2\">\r\n"
        "<IE_HEADER>\r\n"
        "<MessageType>MSG_VTDU_VIDEO_TIMEOUT_REQ</MessageType>\r\n"
        "<Reserved />\r\n"
        "</IE_HEADER>\r\n"
        "<IE_STOP_VIDEO>\r\n"
        "<ChannelID>%d</ChannelID>\r\n"
        "<StreamType>%d</StreamType>\r\n"
        "<PUID>%s</PUID>\r\n"
        "<CUID>%s</CUID>\r\n"
        "</IE_STOP_VIDEO>\r\n"
        "</Message>\r\n", stCurMediaInfo.nPuChannelID, stCurMediaInfo.nPuStreamType, stCurMediaInfo.strPUID.c_str(), stCurMediaInfo.strCuUserID.c_str());
    int ret = SipUA_InitMessage("INFO", configSipServer.m_strLocalSipId.c_str(), configSipServer.m_strSipAddr.c_str(), configSipServer.m_iSipPort, configSipServer.m_strServerId.c_str(), configSipServer.m_strServerIP.c_str(),
        configSipServer.m_iServerPort, szKeepaliveXml, iXmlLen, ";MSG_TYPE=MSG_VTDU_VIDEO_TIMEOUT_REQ");
    if (ret < 0)
    {
        VTDU_LOG_E("send MSG_VTDU_VIDEO_TIMEOUT_REQ failed, server: " << configSipServer.m_strLocalSipId << "@" << configSipServer.m_strServerIP << ":" << configSipServer.m_iServerPort << ", body: " << szKeepaliveXml);
    }

    return ret;
}

/**************************************************************************
* name          : SipUA_HeartBeat
* description   : 发送v3 心跳
* input         : configSipServer  vtdu配置信息
*                 nInstreamNum     输入视频流路数
*                 nOutstreamNum    输出视频流路数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_HeartBeat(const ConfigSipServer &configSipServer, int nInstreamNum, int nOutstreamNum)
{
    char szKeepaliveXml[1024] = { 0 };
    int iXmlLen = sprintf(szKeepaliveXml, "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\r\n"
        "<Message Version=\"1.0.2\">\r\n"
        "<IE_HEADER/>\r\n"
        "<VTDU_LOAD>\r\n"
        "<inStrmSize>%d</inStrmSize>\r\n"
        "<oStrmSize>%d</oStrmSize>\r\n"
        "<VODCount>0</VODCount>\r\n"
        "</VTDU_LOAD>\r\n"
        "</Message>\r\n", nInstreamNum, nOutstreamNum);
    int ret = SipUA_InitMessage("OPTIONS", configSipServer.m_strLocalSipId.c_str(), configSipServer.m_strSipAddr.c_str(), configSipServer.m_iSipPort, configSipServer.m_strServerId.c_str(), configSipServer.m_strServerIP.c_str(),
        configSipServer.m_iServerPort, szKeepaliveXml, iXmlLen, ";MSG_TYPE=MSG_VTDU_HEART");
    if (ret < 0)
    {
        VTDU_LOG_E("send keepalive failed, server: " << configSipServer.m_strLocalSipId << "@" << configSipServer.m_strServerIP << ":" << configSipServer.m_iServerPort << ", body: " << szKeepaliveXml);
    }

    return ret;
}
