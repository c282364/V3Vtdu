#include "communicationHi.h"
#include "Stream.h"
communicationHi::communicationHi()
    :m_fd2Vtdu(-1)
    , m_CBFunc(NULL)
    , m_pEventCBParam(NULL)
{

}

communicationHi::~communicationHi()
{

}

/**************************************************************************
* name          : Init
* description   : 初始化
* input         : configSipServer vtdu配置信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int communicationHi::Init(const ConfigSipServer &configSipServer)
{
    m_fd2Vtdu = createFd(configSipServer.m_iVtdu2HiPort, true, configSipServer.m_strSipAddr);
    if (m_fd2Vtdu <= 0)
    {
        printf("createFd m_fd2Vtdu failed,port:%d\n", configSipServer.m_iVtdu2HiPort);
        return -1;
    }

    return 0;
}

/**************************************************************************
* name          : FIni
* description   : 反初始化
* input         : NA
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int communicationHi::FIni()
{
    closeFd(m_fd2Vtdu);
    return 0;
}

/**************************************************************************
* name          : StartEventLoop
* description   : 开始消息接收
* input         : fnCB 消息上报函数
*                 pParam 用户指针
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int communicationHi::StartEventLoop(fHiEventCB fnCB, void *pParam)
{
    m_CBFunc = fnCB;
    m_pEventCBParam = pParam;

    //处理hi模块消息线程 //tbd放到communicationHi模块
    std::thread ThHiMsgLoop(&communicationHi::threadHiMsgLoop, this);
    return 0;
}

/**************************************************************************
* name          : threadHiMsgLoop
* description   : hi模块上报消息接收线程
* input         : pParam 用户参数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
void communicationHi::threadHiMsgLoop()
{
    while (1)
    {
        const int iRecvBuffLen = 4096;
        char szRecvBuff[4096] = { 0 };
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50 * 1000;
        fd_set readFdSet;
        FD_ZERO(&readFdSet);
        FD_SET(m_fd2Vtdu, &readFdSet);
        int ret = select(m_fd2Vtdu + 1, &readFdSet, NULL, NULL, &tv);
        if (ret < 0)
        {
            break;
        }
        else if (0 == ret)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }
        else
        {
            if (FD_ISSET(m_fd2Vtdu, &readFdSet))
            {
                struct sockaddr_in addrFrom;
                int iAddrLen = sizeof(addrFrom);
                ret = recvfrom(m_fd2Vtdu, szRecvBuff, iRecvBuffLen, 0, (struct sockaddr*)&addrFrom, (int*)&iAddrLen);
                if (ret > 0 && m_CBFunc != NULL)
                {
                    printf("recv:%s\n", szRecvBuff);

                    szRecvBuff[ret] = 0;
                    int iSendBuffLen = 0;
                    char szSendBuffBuff[4096] = { 0 };
                    m_CBFunc(szRecvBuff, ret, szSendBuffBuff, &iSendBuffLen, m_pEventCBParam);
                    //回复
                    if (iSendBuffLen > 0)
                    {
                        int sendret = sendto(m_fd2Vtdu, (char*)szSendBuffBuff, iSendBuffLen, 0, (sockaddr*)&addrFrom, sizeof(sockaddr));
                    }
                }
            }
        }
    }
    return;
}

/**************************************************************************
* name          : sendTransReq
* description   : 发送转码请求
* input         : nRecvPort vtdu数据接收端口
*                 strPuInfo 任务id
*                 strHiIp  转码模块ip
*                 nHiPort  转码模块端口
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int communicationHi::sendTransReq(int nRecvPort, const std::string &strPuInfo, const std::string &strHiIp, int nHiPort)
{
    char szKeepaliveXml[1024] = { 0 };
    int iXmlLen = sprintf(szKeepaliveXml,
        "<VTDU_HEADER>\r\n"
        "<MessageType>MSG_VTDU_TRANS_REQ</MessageType>\r\n"
        "</VTDU_HEADER>\r\n"
        "<VTDU_STREAM_INFO>\r\n"
        "<VTDU_RECV_PORT>%d</VTDU_RECV_PORT>\r\n"
        "<VTDU_TASK_ID>%s</VTDU_TASK_ID>\r\n"
        "</VTDU_STREAM_INFO>\r\n", nRecvPort, strPuInfo.c_str());

    struct sockaddr_in sockaddrVtdu;
    sockaddrVtdu.sin_addr.s_addr = inet_addr(strHiIp.c_str());
    sockaddrVtdu.sin_family = AF_INET;
    sockaddrVtdu.sin_port = ntohs(nHiPort);
    int nRet = sendto(m_fd2Vtdu, szKeepaliveXml, iXmlLen, 0, (sockaddr*)&sockaddrVtdu, sizeof(sockaddr));
    if (nRet <= 0)
    {
        printf("send task to hi failed, hi info[%s:%d], body: %s", strHiIp.c_str(),
            nHiPort, szKeepaliveXml);
    }

    return nRet;
}

/**************************************************************************
* name          : sendStopTransReq
* description   : 发送停止转码请求
* input         : strPuInfo 任务id
*                 strHiIp  转码模块ip
*                 nHiPort  转码模块端口
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int communicationHi::sendStopTransReq(const std::string &strPuInfo, const std::string &strHiIp, int nHiPort)
{
    char szKeepaliveXml[1024] = { 0 };
    int iXmlLen = sprintf(szKeepaliveXml,
        "<VTDU_HEADER>\r\n"
        "<MessageType>MSG_VTDU_STOP_TRANS_REQ</MessageType>\r\n"
        "</VTDU_HEADER>\r\n"
        "<VTDU_STREAM_INFO>\r\n"
        "<VTDU_TASK_ID>%s</VTDU_TASK_ID>\r\n"
        "</VTDU_STREAM_INFO>\r\n", strPuInfo.c_str());
    struct sockaddr_in sockaddrVtdu;
    sockaddrVtdu.sin_addr.s_addr = inet_addr(strHiIp.c_str());
    sockaddrVtdu.sin_family = AF_INET;
    sockaddrVtdu.sin_port = ntohs(nHiPort);

    return sendto(m_fd2Vtdu, szKeepaliveXml, iXmlLen, 0, (sockaddr*)&sockaddrVtdu, sizeof(sockaddr));
}

/**************************************************************************
* name          : sendReconnect
* description   : 发送重连请求
* input         : strHiIp  转码模块ip
*                 nHiPort  转码模块端口
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int communicationHi::sendReconnect(const std::string &strHiIp, int nHiPort)
{
    char szKeepaliveXml[1024] = { 0 };
    int iXmlLen = sprintf(szKeepaliveXml,
        "<VTDU_HEADER>\r\n"
        "<MessageType>MSG_VTDU_RECONNECT</MessageType>\r\n"
        "</VTDU_HEADER>\r\n");
    struct sockaddr_in sockaddrVtdu;
    sockaddrVtdu.sin_addr.s_addr = inet_addr(strHiIp.c_str());
    sockaddrVtdu.sin_family = AF_INET;
    sockaddrVtdu.sin_port = ntohs(nHiPort);
    return sendto(m_fd2Vtdu, szKeepaliveXml, iXmlLen, 0, (sockaddr*)&sockaddrVtdu, sizeof(sockaddr));
}
