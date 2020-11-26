#include "Stream.h"

/**************************************************************************
* name          : createFdUdp
* description   : 创建socket
* input         : port 端口
*                 bSend 是否需要发送数据
*                 strIp ip
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int createFdUdp(int port, bool bSend, const std::string &strIp)
{
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0)
    {
        VTDU_LOG_E("create socket failed, err: " << errno);
        return -1;
    }

    struct sockaddr_in localAddr;
    localAddr.sin_family = AF_INET;
    if (bSend)
    {
        localAddr.sin_addr.s_addr = inet_addr(strIp.c_str());
    }
    else
    {
        localAddr.sin_addr.s_addr = htonl(INADDR_ANY); //inet_addr(strLocalIP.c_str());
    }
    localAddr.sin_port = htons(port);
    int ret = bind(fd, (struct sockaddr*)&localAddr, sizeof(struct sockaddr));
    if (ret < 0)
    {
        VTDU_LOG_E("bind port failed, port: " << port << ", error: " << errno);
#ifdef WIN32
        closesocket(fd);
#else
        close(fd);
#endif
        fd = -1;
        return -1;
    }
#ifndef WIN32
    //设置fd为非阻塞
    int flag = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flag | O_NONBLOCK);
#else
    unsigned long ul = 1;
    ioctlsocket(fd, FIONBIO, (unsigned long *)&ul);
#endif //WIN32

    return fd;
}

/**************************************************************************
* name          : closeFd
* description   : 销毁socket
* input         : nFd socket句柄
* output        : nFd socket句柄
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int closeFd(int &nFd)
{
#ifdef WIN32
    closesocket(nFd);
#else
    close(nFd);
#endif

    nFd = -1;

    return 0;
}

Stream::Stream()
{
    m_pFuncCb = NULL;
    m_pUserPara = NULL;
}


Stream::~Stream()
{
}

///**************************************************************************
//* name          : start
//* description   : 启动
//* input         : bTans 是否需要转码
//* output        : NA
//* return        : 0表示成功 小于零失败 具体见错误码定义
//* remark        : NA
//**************************************************************************/
//int Stream::start(bool bTans)
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : stop
//* description   : 停止
//* input         : NA
//* output        : NA
//* return        : 0表示成功 小于零失败 具体见错误码定义
//* remark        : NA
//**************************************************************************/
//int Stream::stop()
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : addOneSend
//* description   : 添加一个接收方
//* input         : strClientIp 接收方ip
//*                 nClientPort 接收方端口
//*                 strCuUserID 接收方id
//*                 nSendPort   发送端口
//* output        : NA
//* return        : 0表示成功 小于零失败 具体见错误码定义
//* remark        : NA
//**************************************************************************/
//int Stream::addOneSend(const std::string &strClientIp, int nClientPort, const std::string &strCuUserID, int nSendPort)
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : DelOneSend
//* description   : 删除一个接收方
//* input         : strCuUserID  接收方id
//*                 nCurSendPort 发送端口
//* output        : NA
//* return        : 0表示成功 小于零失败 具体见错误码定义
//* remark        : NA
//**************************************************************************/
//int Stream::DelOneSend(const std::string &strCuUserID, int &nCurSendPort)
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : getRecvPort
//* description   : 获取接收端口
//* input         : NA
//* output        : nRecvPort 发送端口
//* return        : 0表示成功 小于零失败 具体见错误码定义
//* remark        : NA
//**************************************************************************/
//int Stream::getRecvPort(int &nRecvPort)
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : getOutstreamNum
//* description   : 获取分发路数
//* input         : NA
//* output        : nOutstreamNum 分发路数
//* return        : 0表示成功 小于零失败 具体见错误码定义
//* remark        : NA
//**************************************************************************/
//int Stream::getOutstreamNum(int &nOutstreamNum)
//{
//    return 0;
//}
//
/**************************************************************************
* name          : setCallBack
* description   : 设置回调函数上报流处理层事件到上层管理层
* input         : StreamInfoCallbackFun  管理层回调函数
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void Stream::setCallBack(StreamInfoCallbackFun pFuncCb, void* pPara)
{
    m_pFuncCb = pFuncCb;
    m_pUserPara = pPara;
    return;
}

/**************************************************************************
* name          : getSendPort
* description   : 获取所有发送端口
* input         : vecSendPort  发送端口列表
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int Stream::getSendPort(std::vector<int>& vecSendPort)
{
    mtSendList.lock();
    std::map<std::string, stSendClientInfo>::iterator itor = m_mapSendList.begin();
    for (; itor != m_mapSendList.end(); ++itor)
    {
        vecSendPort.push_back(itor->second.nSendPort);
    }
    mtSendList.unlock();

    return 1;
}
