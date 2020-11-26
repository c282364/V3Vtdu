#include "Stream.h"

/**************************************************************************
* name          : createFdUdp
* description   : ����socket
* input         : port �˿�
*                 bSend �Ƿ���Ҫ��������
*                 strIp ip
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
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
    //����fdΪ������
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
* description   : ����socket
* input         : nFd socket���
* output        : nFd socket���
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
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
//* description   : ����
//* input         : bTans �Ƿ���Ҫת��
//* output        : NA
//* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
//* remark        : NA
//**************************************************************************/
//int Stream::start(bool bTans)
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : stop
//* description   : ֹͣ
//* input         : NA
//* output        : NA
//* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
//* remark        : NA
//**************************************************************************/
//int Stream::stop()
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : addOneSend
//* description   : ���һ�����շ�
//* input         : strClientIp ���շ�ip
//*                 nClientPort ���շ��˿�
//*                 strCuUserID ���շ�id
//*                 nSendPort   ���Ͷ˿�
//* output        : NA
//* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
//* remark        : NA
//**************************************************************************/
//int Stream::addOneSend(const std::string &strClientIp, int nClientPort, const std::string &strCuUserID, int nSendPort)
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : DelOneSend
//* description   : ɾ��һ�����շ�
//* input         : strCuUserID  ���շ�id
//*                 nCurSendPort ���Ͷ˿�
//* output        : NA
//* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
//* remark        : NA
//**************************************************************************/
//int Stream::DelOneSend(const std::string &strCuUserID, int &nCurSendPort)
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : getRecvPort
//* description   : ��ȡ���ն˿�
//* input         : NA
//* output        : nRecvPort ���Ͷ˿�
//* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
//* remark        : NA
//**************************************************************************/
//int Stream::getRecvPort(int &nRecvPort)
//{
//    return 0;
//}
//
///**************************************************************************
//* name          : getOutstreamNum
//* description   : ��ȡ�ַ�·��
//* input         : NA
//* output        : nOutstreamNum �ַ�·��
//* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
//* remark        : NA
//**************************************************************************/
//int Stream::getOutstreamNum(int &nOutstreamNum)
//{
//    return 0;
//}
//
/**************************************************************************
* name          : setCallBack
* description   : ���ûص������ϱ���������¼����ϲ�����
* input         : StreamInfoCallbackFun  �����ص�����
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
* description   : ��ȡ���з��Ͷ˿�
* input         : vecSendPort  ���Ͷ˿��б�
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
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
