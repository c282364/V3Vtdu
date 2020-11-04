#include "GB28181Stream.h"
#include <iostream>
#include <time.h>
#include <string.h>

struct NaluPacket
{
    unsigned char*  data;
    int				length;
    int				prefix;
};

#define RTP_RECV_BUFF_MAX_LEN	(1024*1024)
#define memcpy_rtcpdata(buff, pos, val, len) {memcpy((buff+pos), &val, len); pos+=len;}


bool findNalu(unsigned char* buffer, size_t length, size_t start, NaluPacket& packet)
{
    if ((length < 3) || ((length - start) < 3))
    {
        return false;
    }

    bool		found = false;
    unsigned char* p = buffer;
    for (size_t i = start; i < (length - 3); ++i)
    {
        if ((p[i] == 0) && (p[i + 1] == 0))
        {
            if (p[i + 2] == 0)
            {
                if (((i + 3) < length) && (p[i + 3] == 1))
                {
                    //0x 00 00 00 01 
                    packet.data = p + i;
                    packet.length = i;
                    packet.prefix = 4;
                    found = true;
                    break;
                }
            }
            else if (p[i + 2] == 1)
            {
                packet.data = p + i;
                packet.length = i;
                packet.prefix = 3;
                found = true;
                break;
            }
        }
    }
    return found;
}

#ifdef WIN32
unsigned int __stdcall threadRecvV3(void *pParam)
#else
#endif
{
    if (NULL == pParam)
    {
        printf("threadRecvV3, para is NULL!\n");
        return -1;
    }
    GB28181Stream* pHandle = (GB28181Stream *)pParam;
    pHandle->V3StreamWorking();
    return 0;
}

#ifdef WIN32
unsigned int __stdcall threadRecvHi(void *pParam)
#else
#endif
{
    if (NULL == pParam)
    {
        printf("threadRecvHi, para is NULL!\n");
        return -1;
    }
    GB28181Stream* pHandle = (GB28181Stream *)pParam;
    pHandle->HiStreamWorking();
    return 0;
}

GB28181Stream::GB28181Stream(std::string strPuInfo, int nPortRecv, std::string strLocalIp)
{
    m_nOutNum = 0;
    m_nRecvPort = nPortRecv;

    m_bTrans = false;
    m_bWorkStop = true;
    m_hWorkThreadV3 = NULL;
    m_hWorkThreadHi = NULL;
    m_fdRtpRecv = -1;
    m_fdRtcpRecv = -1;

    m_strLocalIp = strLocalIp;

    /*随机生成SSRC*/
    srand((int)time(0));
    unsigned long ssrc = 100000 + (rand() % 200000);
    m_ulSSRC = ssrc;
    m_usSeq = 0;
    m_ulTimeStamp = 0;

    m_strPuInfo = strPuInfo;
}
GB28181Stream::~GB28181Stream()
{
    stop();
}

/**************************************************************************
* name          : start
* description   : 启动
* input         : bTans 是否需要转码
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int GB28181Stream::start(bool bTans)
{
    m_bTrans = bTans;

    m_pPsBuff = new(std::nothrow) unsigned char[1024 * 512];
    if (NULL == m_pPsBuff)
    {
        stop();
        return -1;
    }

    m_fdRtpRecv = createFd(m_nRecvPort, false, m_strLocalIp);
    if (m_fdRtpRecv <= 0)
    {
        stop();
        return -1;
    }

    int iRtcpRecvPort = m_nRecvPort + 1;
    m_fdRtcpRecv = createFd(iRtcpRecvPort, true, m_strLocalIp);
    if (m_fdRtcpRecv <= 0)
    {
        stop();
        return -1;
    }

    m_bWorkStop = false;
    if (!bTans)
    {
#ifdef WIN32
        m_hWorkThreadV3 = (HANDLE)_beginthreadex(
            NULL,
            0,
            threadRecvV3,
            this,
            0,
            NULL);
#else

#endif
        if (0 >= m_hWorkThreadV3)
        {
            stop();
            return -1;
        }
    }
    else
    {
#ifdef WIN32
        m_hWorkThreadHi = (HANDLE)_beginthreadex(
            NULL,
            0,
            threadRecvHi,
            this,
            0,
            NULL);
#else
#endif
        if (0 >= m_hWorkThreadHi)
        {
            stop();
            return -1;
        }
    }

    return 0;
}

/**************************************************************************
* name          : stop
* description   : 停止
* input         : NA
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int GB28181Stream::stop()
{
    //释放所有socket
    if (false == m_bWorkStop)
    {
        m_bWorkStop = true;
        if (0 < m_hWorkThreadV3)
        {
            WaitForSingleObject(m_hWorkThreadV3, INFINITE);
            CloseHandle(m_hWorkThreadV3);
            m_hWorkThreadHi = 0;
        }
        
        if (m_bTrans && 0 < m_hWorkThreadHi)
        {
            WaitForSingleObject(m_hWorkThreadHi, INFINITE);
            CloseHandle(m_hWorkThreadHi);
            m_hWorkThreadHi = 0;
        }  
    }

    mtSendList.lock();
    std::map<std::string, stSendClientInfo>::iterator itor = m_mapSendList.begin();
    for (; itor != m_mapSendList.end(); ++itor)
    {
        closeFd(itor->second.fdSend);
    }
    mtSendList.unlock();

    if (m_fdRtpRecv > 0)
    {
        closeFd(m_fdRtpRecv);
    }
    if (m_fdRtcpRecv > 0)
    {
        closeFd(m_fdRtcpRecv);
    }

    if (NULL != m_pPsBuff)
    {
        delete m_pPsBuff;
        m_pPsBuff = NULL;
    }

    return 0;
}

/**************************************************************************
* name          : addOneSend
* description   : 添加一个接收方
* input         : strClientIp 接收方ip
*                 nClientPort 接收方端口
*                 strCuUserID 接收方id
*                 nSendPort   发送端口
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int GB28181Stream::addOneSend(const std::string &strClientIp, int nClientPort, const std::string &strCuUserID, int nSendPort)
{
    //添加到发送列表。
    struct sockaddr_in sockaddrV3Clinet;
    sockaddrV3Clinet.sin_addr.s_addr = inet_addr(strClientIp.c_str());
    sockaddrV3Clinet.sin_family = AF_INET;
    sockaddrV3Clinet.sin_port = ntohs(nClientPort);

    stSendClientInfo curSendClientInfo;

    curSendClientInfo.fdSend = createFd(nSendPort, true, m_strLocalIp);
    if (curSendClientInfo.fdSend <= 0)
    {
        return -1;
    }

    curSendClientInfo.nSendPort = nSendPort;

    curSendClientInfo.stClientAddr = sockaddrV3Clinet;
    mtSendList.lock();
    m_mapSendList[strCuUserID] = curSendClientInfo;
    mtSendList.unlock();
    m_nOutNum++;

    return 0;
}

/**************************************************************************
* name          : DelOneSend
* description   : 删除一个接收方
* input         : strCuUserID  接收方id
*                 nCurSendPort 发送端口
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int GB28181Stream::DelOneSend(const std::string &strCuUserID, int &nCurSendPort)
{
    int nSendNum = 0;
    //删除发送列表
    mtSendList.lock();
    std::map<std::string, stSendClientInfo>::iterator itorSendList = m_mapSendList.find(strCuUserID);
    if (itorSendList != m_mapSendList.end())
    {
        closeFd(itorSendList->second.fdSend);
        nCurSendPort = itorSendList->second.nSendPort;
        m_nOutNum--;
        m_mapSendList.erase(itorSendList);
    }
    else
    {
        mtSendList.unlock();      
        return -1;
    }
    nSendNum = m_mapSendList.size();
    mtSendList.unlock();
    
    return nSendNum;
}

/**************************************************************************
* name          : getRecvPort
* description   : 获取接收端口
* input         : NA
* output        : nRecvPort 发送端口
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int GB28181Stream::getRecvPort(int &nRecvPort)
{
    nRecvPort = m_nRecvPort;
    return 0;
}

/**************************************************************************
* name          : getOutstreamNum
* description   : 获取分发路数
* input         : NA
* output        : nOutstreamNum 分发路数
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int GB28181Stream::getOutstreamNum(int &nOutstreamNum)
{
    nOutstreamNum = m_nOutNum;
    return 0;
}

/**************************************************************************
* name          : inputFrameData
* description   : 转码模块视频裸流传入，再转发PS流到远端国标平台
* input         : pFrameData  数据包
*                 iFrameLen   数据包长度
*                 i64TimeStamp 时间戳
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int GB28181Stream::inputFrameData(unsigned char* pFrameData, int iFrameLen, unsigned long long i64TimeStamp)
{
    //static unsigned long ulTimeStamp = 0; //(unsigned long)i64TimeStamp;
    //ulTimeStamp += 3000;
    //static FILE *fpt = fopen("ps_dump.ps", "wb");

    //00 00 00 01 67/68/65/41
    NAL_type Type = getH264NALtype(pFrameData[4]);
    if (NAL_SPS == Type || NAL_PFRAME == Type)
    {
        m_ulTimeStamp += 3600;
    }
    //printf("NAL_type:%d\n", Type);
    int iPsLen = h264PsMux(pFrameData, iFrameLen, Type, (unsigned long long)m_ulTimeStamp, m_pPsBuff);
    if (iPsLen > 0)
    {
        //fwrite(m_pPsBuff, 1, iPsLen, fpt);

        ////每次发送1400字节数据
        int iFrameSizePerCap = 1400;
        int iTotalSent = 0;

        int iLeftLen = iPsLen;
        int iSentLen = 0;
        int iBlockLen = iFrameSizePerCap;
        while (iLeftLen > 0)
        {
            m_usSeq++;

            if (iLeftLen > iFrameSizePerCap)
            {
                iBlockLen = iFrameSizePerCap;
                iTotalSent += sendOneBlock(m_pPsBuff + iSentLen, iBlockLen, m_usSeq, m_ulTimeStamp, false);
            }
            else
            {
                //最后结束块
                iBlockLen = iLeftLen;
                iTotalSent += sendOneBlock(m_pPsBuff + iSentLen, iBlockLen, m_usSeq, m_ulTimeStamp, true);
            }

            iLeftLen -= iBlockLen;
            iSentLen += iBlockLen;
        }

        return iTotalSent;
    }

    return -1;
}

/**************************************************************************
* name          : sendOneBlock
* description   : 一个完整的视频帧，分块RTP发送
* input         : pBlockData  数据包
*                 iBlockLen   数据包长度
*                 ulIndex     包索引
*                 ulTimeStamp 时间戳
*                 bEndHead    是否为结束块
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int GB28181Stream::sendOneBlock(unsigned char *pBlockData, int iBlockLen, unsigned long ulIndex, unsigned long ulTimeStamp, bool bEndHead)
{
    int len = 0;

    const int iMaxSendBufSize = 2048;
    if (iBlockLen > iMaxSendBufSize)
    {
        printf("block size[%d] > max size[%d]\r\n", iBlockLen, iMaxSendBufSize);
        return -1;
    }

    /* build the RTP header */
    /*
     *
     *    0                   1                   2                   3
     *    0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *   |V=2|P|X|  CC   |M|     PT      |       sequence number         |
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *   |                           timestamp                           |
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *   |           synchronization source (SSRC) identifier            |
     *   +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
     *   |            contributing source (CSRC) identifiers             |
     *   :                             ....                              :
     *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
     *
     **/

     //BYTE pSendBuff[200] = {0x80, 0x60, 0xA8, 0x5F, 0x08, 0x1B, 0x64, 0xAC, 0x44, 0xB4, 0xAC, 0xB1, 0x00};

     /*RTP头定义如上，以下是简单处理RTP头*/

    unsigned char pSendBuff[iMaxSendBufSize] = { 0 };

    pSendBuff[0] = 0x80;

    if (bEndHead)
    {
        pSendBuff[1] = 0xE0;
    }
    else
        pSendBuff[1] = 0x60;

    //索引
    pSendBuff[2] = ((ulIndex & 0xFF00) >> 8);
    pSendBuff[3] = ulIndex & 0x00FF;

    //时间戳
    pSendBuff[4] = ((ulTimeStamp & 0xFF000000) >> 24);
    pSendBuff[5] = ((ulTimeStamp & 0x00FF0000) >> 16);
    pSendBuff[6] = ((ulTimeStamp & 0x0000FF00) >> 8);
    pSendBuff[7] = ((ulTimeStamp & 0x000000FF));

    //ssrc
    pSendBuff[8] = ((m_ulSSRC & 0xFF000000) >> 24);
    pSendBuff[9] = ((m_ulSSRC & 0x00FF0000) >> 16);
    pSendBuff[10] = ((m_ulSSRC & 0x0000FF00) >> 8);
    pSendBuff[11] = ((m_ulSSRC & 0x000000FF));

    memcpy(pSendBuff + RTP_HDR_LEN, pBlockData, iBlockLen);
    len = RTP_HDR_LEN + iBlockLen;


    /*int select(int nfds, fd_set *readfds, fd_set *writefds,
                      fd_set *exceptfds, struct timeval *timeout);*/


    int sendret = 0;

    //发送客户端
    mtSendList.lock();
    std::map<std::string, stSendClientInfo>::iterator itor = m_mapSendList.begin();
    for (; itor != m_mapSendList.end(); ++itor)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50 * 1000;
        fd_set writeFdSet;
        FD_ZERO(&writeFdSet);
        FD_SET(itor->second.fdSend, &writeFdSet);
        int ret1 = select(itor->second.fdSend + 1, NULL, &writeFdSet, NULL, &tv);
        if (ret1 <= 0)
        {
            printf("select RtpSend failed,port:%d\n", itor->second.nSendPort);
        }
        else
        {
            if (FD_ISSET(itor->second.fdSend, &writeFdSet))
            {
                //发送
                sendret = sendto(itor->second.fdSend, (char*)pSendBuff, len, 0, (sockaddr*)&(itor->second.stClientAddr), sizeof(sockaddr));
                printf("send to v3:%d\n", sendret);
                
            }
        }
    }
    mtSendList.unlock();

    return sendret;
}

/**************************************************************************
* name          : V3StreamWorking
* description   : 处理从v3平台接收流
* input         : NA
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void GB28181Stream::V3StreamWorking()
{
    printf("begin GB28181Stream::V3StreamWorking\n");
    const int iRecvBuffLen = 4096;
    char szRecvBuff[iRecvBuffLen] = { 0 };
    long long i64LastTime = Comm_GetMilliSecFrom1970();
    unsigned long ulNowTick = Comm_GetTickCount();
    unsigned long ulSenderId = htonl(ulNowTick);
    bool bRecvPacket = false;
    long long i64LastRecvTime = Comm_GetSecFrom1970();
    while (!m_bWorkStop)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50 * 1000;
        fd_set readFdSet;
        FD_ZERO(&readFdSet);
        FD_SET(m_fdRtpRecv, &readFdSet);
        int ret = select(m_fdRtpRecv + 1, &readFdSet, NULL, NULL, &tv);
        if (ret < 0)
        {
            printf("m_fdRtpRecv select failed,port:%d\n", m_nRecvPort);
            continue;
        }
        else if (0 == ret)
        {
            long long nCurTime = Comm_GetSecFrom1970();
            //收到第一个数据包以后，开始监控 连续15秒没有数据判断为断流。回收资源，上报vtdu.
            if (bRecvPacket && nCurTime - i64LastRecvTime >= 15)
            {
                printf("video stream cut off,last time:%I64d, curtime:%I64d\n", i64LastRecvTime, nCurTime);
                //通知收流失败
                if (NULL != m_pFuncCb)
                {
                    m_pFuncCb(0, m_strPuInfo, m_pUserPara);
                }
                break;
            }
            continue;
        }
        else
        {
            if (FD_ISSET(m_fdRtpRecv, &readFdSet))
            {
                struct sockaddr_in addrFrom;
                int iAddrLen = sizeof(addrFrom);
                ret = recvfrom(m_fdRtpRecv, szRecvBuff, iRecvBuffLen, 0, (struct sockaddr*)&addrFrom, (int*)&iAddrLen);
                if (ret > 0)
                {
                    //test
                    //static FILE* file = fopen("d:\\recvV3.264", "wb+");
                    //fwrite(szRecvBuff,1, ret,file);
                    //test end
                    bRecvPacket = true;
                    i64LastRecvTime = Comm_GetSecFrom1970();
                    unsigned char* pRtpData = (unsigned char*)szRecvBuff;
                    //send data 发送数据到客户端
                    mtSendList.lock();
                    std::map<std::string, stSendClientInfo>::iterator itor = m_mapSendList.begin();
                    for (; itor != m_mapSendList.end(); ++itor)
                    {
                        struct timeval tv;
                        tv.tv_sec = 0;
                        tv.tv_usec = 50 * 1000;
                        fd_set writeFdSet;
                        FD_ZERO(&writeFdSet);
                        FD_SET(itor->second.fdSend, &writeFdSet);
                        int ret1 = select(itor->second.fdSend + 1, NULL, &writeFdSet, NULL, &tv);
                        if (ret1 <= 0)
                        {
                            printf("select RtpSend failed,port:%d\n", itor->second.nSendPort);
                        }
                        else
                        {
                            if (FD_ISSET(itor->second.fdSend, &writeFdSet))
                            {
                                //发送
                                int sendret = sendto(itor->second.fdSend, (char*)szRecvBuff, ret, 0, (sockaddr*)&(itor->second.stClientAddr), sizeof(sockaddr));
                                printf("send to v3,len:%d\n", sendret);

                            }
                        }
                    }
                    mtSendList.unlock();
                    long long i64CurTime = Comm_GetMilliSecFrom1970();
                    long long i64TimeGap = i64CurTime - i64LastTime;
                    if (i64TimeGap >= 5000) //5秒回复一次RTCP receiver report消息
                    {
                        i64LastTime = i64CurTime;

                        unsigned char btRtcpData[1024] = { 0 };
                        unsigned short seq = 0;
                        unsigned long ulTimeStamp = 0;
                        unsigned long ssrc = 0;

                        memcpy(&seq, pRtpData + 2, 2);
                        memcpy(&ulTimeStamp, pRtpData + 4, 4);
                        memcpy(&ssrc, pRtpData + 8, 4);

                        int iRtcpDataLen = makeRtcpPacketBuff(ulSenderId, ssrc, ulTimeStamp, seq, btRtcpData);
                        if (iRtcpDataLen > 0)
                        {
                            //rtcp地址为发送RTP端口自动+1
                            unsigned short usRtpFromPort = ntohs(addrFrom.sin_port);
                            struct sockaddr_in addrToRtcp;
                            memcpy(&addrToRtcp, &addrFrom, iAddrLen);
                            addrToRtcp.sin_port = htons(usRtpFromPort + 1);
                            sendto(m_fdRtcpRecv, (char*)btRtcpData, iRtcpDataLen, 0, (sockaddr*)&addrToRtcp, sizeof(sockaddr));
                        }
                    }
                }
            }
        }
    }
    printf("end GB28181Stream::V3StreamWorking\n");
    return ;
}

/**************************************************************************
* name          : HiStreamWorking
* description   : 处理从Hi转码模块接收流
* input         : NA
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void GB28181Stream::HiStreamWorking()
{
    printf("begin GB28181Stream::HiStreamWorking\n");
    const int iRecvBuffLen = 4096;
    char szRecvBuff[iRecvBuffLen] = { 0 };
    long long i64LastTime = Comm_GetMilliSecFrom1970();
    unsigned long ulNowTick = Comm_GetTickCount();
    unsigned long ulSenderId = htonl(ulNowTick);
    int nBeginParseIndex = 0;
    unsigned char *pBufferDataPtr;
    int   sBufferDataReadable = 0;
    int nCurTotalLen = 0;
    unsigned char *szBufParse = new unsigned char[2 * 1024 * 1024];
    unsigned char *szBufParseLeft = new unsigned char[2 * 1024 * 1024];
    bool bRecvPacket = false;
    long long i64LastRecvTime = Comm_GetSecFrom1970();
    while (!m_bWorkStop)
    {
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = 50 * 1000;
        fd_set readFdSet;
        FD_ZERO(&readFdSet);
        FD_SET(m_fdRtpRecv, &readFdSet);
        int ret = select(m_fdRtpRecv + 1, &readFdSet, NULL, NULL, &tv);
        if (ret < 0)
        {
            printf("m_fdRtpRecv select failed,port:%d\n", m_nRecvPort);
            continue;
        }
        else if (0 == ret)
        {
            long long nCurTime = Comm_GetSecFrom1970();
            //收到第一个数据包以后，开始监控 连续15秒没有数据判断为断流。回收资源，上报vtdu.
            if (bRecvPacket && nCurTime - i64LastRecvTime >= 15)
            {
                printf("video stream cut off,last time:%I64d, curtime:%I64d\n", i64LastRecvTime, nCurTime);
                //通知收流失败
                if (NULL != m_pFuncCb)
                {
                    m_pFuncCb(1, m_strPuInfo, m_pUserPara);
                }
                break;
            }
            continue;
        }
        else
        {
            if (FD_ISSET(m_fdRtpRecv, &readFdSet))
            {
                struct sockaddr_in addrFrom;
                int iAddrLen = sizeof(addrFrom);
                ret = recvfrom(m_fdRtpRecv, szRecvBuff, iRecvBuffLen, 0, (struct sockaddr*)&addrFrom, (int*)&iAddrLen);
                if (ret > 0)
                {
                    bRecvPacket = true;
                    i64LastRecvTime = Comm_GetSecFrom1970();
                    if (sBufferDataReadable > 0)
                    {
                        memcpy(szBufParse, szBufParseLeft, sBufferDataReadable);
                    }
                    memcpy(szBufParse + sBufferDataReadable, szRecvBuff, ret);
                    nCurTotalLen = sBufferDataReadable + ret;
                    nBeginParseIndex = 0;
                    while (true)
                    {
                        pBufferDataPtr = szBufParse + nBeginParseIndex;
                        sBufferDataReadable = nCurTotalLen - nBeginParseIndex;
                        NaluPacket firstPacket;
                        if (!findNalu(pBufferDataPtr, sBufferDataReadable, 0, firstPacket))
                        {
                            memcpy(szBufParseLeft, pBufferDataPtr, sBufferDataReadable);
                            break;
                        }

                        NaluPacket secondPacket;
                        if (!findNalu(pBufferDataPtr, sBufferDataReadable, firstPacket.length + firstPacket.prefix, secondPacket))
                        {
                            memcpy(szBufParseLeft, pBufferDataPtr, sBufferDataReadable);
                            break;
                        }
                        firstPacket.length = secondPacket.length - firstPacket.length;
                        nBeginParseIndex += firstPacket.length;

                        //test
                        //static FILE* fileRecvHi = fopen("d:\\recvHi.264", "wb+");
                        //fwrite(firstPacket.data, 1, firstPacket.length, fileRecvHi);
                        inputFrameData(firstPacket.data, firstPacket.length, 0);
                    }
                    //inputFrameData((unsigned char*)szRecvBuff, ret, 0);
                }
            }
        }
    }

    return;
    printf("end GB28181Stream::HiStreamWorking\n");
}

//rtcp结构体转成字节数组,参数都要求输入网络字节序

/**************************************************************************
* name          : makeRtcpPacketBuff
* description   : 构建rtcp包
* input         : ulSenderId  发送id
*                 ssrc        ssrc
*                 ulTimeStamp 时间戳
*                 usSeq       序列号
*                 pOutBuff    输出rtcp包
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int GB28181Stream::makeRtcpPacketBuff(unsigned long ulSenderId, unsigned long ssrc,
    unsigned long ulTimeStamp, unsigned short usSeq, unsigned char *pOutBuff)
{
    //DEBUG_LOG_MEDIAGATE("sender:%lu, ssrc:%lu, time:%lu, seq:%lu", ulSenderId, ssrc, ulTimeStamp, ulSeq);

//        0               1               2               3
//        0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//byte=0 |V=2|P|    RC   |   PT=SR=201   |             length            |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//     4 |                         SSRC of sender                        |
//       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//     8 |              identifier: ssrc of rtp				             |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    12 | fraction lost |  cumulative number of packets lost            |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    16 | sequence number cycles count  | highest sequence number recved|
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    20 |           		interarrival jitter		                     |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    24 |                  last SR timestamp		                     |
//       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//    28 |           delay since last SR timestamp		                 |
//       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//    32 |V=2|P|    SC   |  PT=SDES=202  |             length            |
//       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
//    36 |                          SSRC of sender                       |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//    40 |    CNAME=1    |     length    | user and domain name ...	 end(0)
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+

    int ret = 0;
    unsigned long ulValueZero = 0;
    //构造receiver report
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
//byte=0 |V=2|P|    RC   |   PT=SR=200   |             length            |
//       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    pOutBuff[0] = 0x81;
    pOutBuff[1] = 0xC9;
    pOutBuff[2] = 0x00;
    pOutBuff[3] = 0x07;
    ret = 4;
    //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //     4 |                         SSRC of sender                        |
    //       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    memcpy_rtcpdata(pOutBuff, ret, ulSenderId, 4);
    //		 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    //	   8 |				identifier: ssrc of rtp 						 |
    //		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    memcpy_rtcpdata(pOutBuff, ret, ssrc, 4);
    //		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //	  12 | fraction lost |	cumulative number of packets lost			 |
    //		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    unsigned long ulFractLost = 0;
    memcpy_rtcpdata(pOutBuff, ret, ulFractLost, 4);
    //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    16 | sequence number cycles count  | highest sequence number recved|
    //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    unsigned short usSeqCyclesCount = 2;
    memcpy_rtcpdata(pOutBuff, ret, usSeqCyclesCount, 2);
    memcpy_rtcpdata(pOutBuff, ret, usSeq, 2);
    //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //    20 |           		interarrival jitter		                     |
    //       +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    memcpy_rtcpdata(pOutBuff, ret, ulValueZero, 4);
    //		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //	  24 |					last SR timestamp							 |
    //		 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    memcpy_rtcpdata(pOutBuff, ret, ulValueZero, 4);
    //		 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    //	  28 |			 delay since last SR timestamp						 |
    //		 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    memcpy_rtcpdata(pOutBuff, ret, ulValueZero, 4);
    //       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    //    32 |V=2|P|    SC   |  PT=SDES=202  |             length            |
    //       +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    pOutBuff[ret] = 0x81;
    pOutBuff[ret + 1] = 0xCA;
    pOutBuff[ret + 2] = 0x00;
    pOutBuff[ret + 3] = 0x06;
    ret += 4;
    //		 +=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+=+
    //	  36 |							SSRC of sender						 |
    //		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    memcpy_rtcpdata(pOutBuff, ret, ulSenderId, 4);
    //		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    //	  40 |	  CNAME=1	 |	   length	 | user and domain name ...  end(0)
    //		 +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
    pOutBuff[ret] = 0x01; //cname=1
    ret++;
    const unsigned char ucTextLen = 15;
    pOutBuff[ret] = ucTextLen; //length=15
    ret++;
    char szText[ucTextLen + 1] = "vtdu-chenxy-PCX"; //TEXT
    memcpy(pOutBuff + ret, szText, ucTextLen);
    ret += ucTextLen;
    pOutBuff[ret] = 0x00; //end
    ret++;

    return ret;
}

