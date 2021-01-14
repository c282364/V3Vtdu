#include "GBRtpPsStream.h"
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
        VTDU_LOG_E("threadRecvV3, para is NULL!");
        return -1;
    }
    GBRtpPsOverUdpStream* pHandle = (GBRtpPsOverUdpStream *)pParam;
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
        VTDU_LOG_E("threadRecvHi, para is NULL");
        return -1;
    }
    GBRtpPsOverUdpStream* pHandle = (GBRtpPsOverUdpStream *)pParam;
    pHandle->HiStreamWorking();
    return 0;
}

GBRtpPsOverUdpStream::GBRtpPsOverUdpStream(std::string strPuInfo, int nPortRecv, std::string strLocalIp)
{
    m_nOutNum = 0;
    m_nRecvPort = nPortRecv;

    m_bTrans = false;
    m_bWorkStop = true;
    m_fdRtpRecv = -1;
    m_fdRtcpRecv = -1;

    m_strLocalIp = strLocalIp;

    /*�������SSRC*/
    srand((int)time(0));
    unsigned long ssrc = 100000 + (rand() % 200000);
    m_ulSSRC = ssrc;
    m_usSeq = 0;
    m_ulTimeStamp = 0;

    m_strPuInfo = strPuInfo;

    m_iRawArrElemCount = 0;
    m_iCurrWriteIndex = 0;
    m_iCurrReadIndex = 0;

    for (int i = 0; i < RAW_DATA_ARRAY_MAX_SIZE; i++)
    {
        m_rawDataArr[i].iDataLen = 0;
        m_rawDataArr[i].pFrameData = NULL;
    }
}
GBRtpPsOverUdpStream::~GBRtpPsOverUdpStream()
{
    stop();
}

/**************************************************************************
* name          : start
* description   : ����
* input         : bTans �Ƿ���Ҫת��
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int GBRtpPsOverUdpStream::start(bool bTans)
{
    m_bTrans = bTans;

    m_pPsBuff = new(std::nothrow) unsigned char[1024 * 512];
    if (NULL == m_pPsBuff)
    {
        stop();
        return -1;
    }

    for (int i = 0; i < RAW_DATA_ARRAY_MAX_SIZE; i++)
    {
        m_rawDataArr[i].iDataLen = 0;
        m_rawDataArr[i].pFrameData = new(std::nothrow) char[CHANNEL_MAX_FRAME_DATA_LEN];
        if (NULL == m_rawDataArr[i].pFrameData)
        {
            stop();
            return -1;
        }

    }

    m_fdRtpRecv = createFdUdp(m_nRecvPort, false, m_strLocalIp);
    if (m_fdRtpRecv <= 0)
    {
        stop();
        return -1;
    }

    int iRtcpRecvPort = m_nRecvPort + 1;
    m_fdRtcpRecv = createFdUdp(iRtcpRecvPort, true, m_strLocalIp);
    if (m_fdRtcpRecv <= 0)
    {
        stop();
        return -1;
    }

    m_bWorkStop = false;
    if (!bTans)
    {
        m_hWorkThreadV3 = std::thread(&GBRtpPsOverUdpStream::V3StreamWorking, this);
    }
    else
    {
        m_hWorkThreadHi = std::thread(&GBRtpPsOverUdpStream::HiStreamWorking, this);
    }

    m_hWorkThreadSend = std::thread(&GBRtpPsOverUdpStream::SendStreamWorking, this);

    return 0;
}

/**************************************************************************
* name          : stop
* description   : ֹͣ
* input         : NA
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int GBRtpPsOverUdpStream::stop()
{
    //�ͷ�����socket
    if (false == m_bWorkStop)
    {
        m_bWorkStop = true;
        if (m_hWorkThreadV3.joinable())
        {
            m_hWorkThreadV3.join();
        }

        if (m_hWorkThreadHi.joinable())
        {
            m_hWorkThreadHi.join();
        }

        if (m_hWorkThreadSend.joinable())
        {
            m_hWorkThreadSend.join();
        }
    }

    {
        std::lock_guard<std::mutex> lock(mtSendList);
        std::map<std::string, stSendClientInfo>::iterator itor = m_mapSendList.begin();
        for (; itor != m_mapSendList.end(); ++itor)
        {
            closeFd(itor->second.fdSend);
        }
    }


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

    for (int i = 0; i < RAW_DATA_ARRAY_MAX_SIZE; i++)
    {
        m_rawDataArr[i].iDataLen = 0;
        if (NULL != m_rawDataArr[i].pFrameData)
        {
            delete m_rawDataArr[i].pFrameData;
            m_rawDataArr[i].pFrameData = NULL;
        }
    }

    return 0;
}

/**************************************************************************
* name          : addOneSend
* description   : ���һ�����շ�
* input         : strClientIp ���շ�ip
*                 nClientPort ���շ��˿�
*                 strCuUserID ���շ�id
*                 nSendPort   ���Ͷ˿�
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int GBRtpPsOverUdpStream::addOneSend(const std::string &strClientIp, int nClientPort, const std::string &strCuUserID, int nSendPort)
{
    //��ӵ������б�
    struct sockaddr_in sockaddrV3Clinet;
    sockaddrV3Clinet.sin_addr.s_addr = inet_addr(strClientIp.c_str());
    sockaddrV3Clinet.sin_family = AF_INET;
    sockaddrV3Clinet.sin_port = ntohs(nClientPort);

    stSendClientInfo curSendClientInfo;

    curSendClientInfo.fdSend = createFdUdp(nSendPort, true, m_strLocalIp);
    if (curSendClientInfo.fdSend <= 0)
    {
        return -1;
    }

    curSendClientInfo.nSendPort = nSendPort;

    curSendClientInfo.stClientAddr = sockaddrV3Clinet;

    {
        std::lock_guard<std::mutex> lock(mtSendList);
        m_mapSendList[strCuUserID] = curSendClientInfo;
    }
    m_nOutNum++;

    return 0;
}

/**************************************************************************
* name          : DelOneSend
* description   : ɾ��һ�����շ�
* input         : strCuUserID  ���շ�id
*                 nCurSendPort ���Ͷ˿�
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int GBRtpPsOverUdpStream::DelOneSend(const std::string &strCuUserID, int &nCurSendPort)
{
    int nSendNum = 0;
    //ɾ�������б�
    {
        std::lock_guard<std::mutex> lock(mtSendList);
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
            return -1;
        }
        nSendNum = m_mapSendList.size();
    }
    
    return nSendNum;
}

/**************************************************************************
* name          : getRecvPort
* description   : ��ȡ���ն˿�
* input         : NA
* output        : nRecvPort ���Ͷ˿�
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int GBRtpPsOverUdpStream::getRecvPort(int &nRecvPort)
{
    nRecvPort = m_nRecvPort;
    return 0;
}

/**************************************************************************
* name          : getOutstreamNum
* description   : ��ȡ�ַ�·��
* input         : NA
* output        : nOutstreamNum �ַ�·��
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int GBRtpPsOverUdpStream::getOutstreamNum(int &nOutstreamNum)
{
    nOutstreamNum = m_nOutNum;
    return 0;
}

/**************************************************************************
* name          : inputFrameData
* description   : ת��ģ����Ƶ�������룬��ת��PS����Զ�˹���ƽ̨
* input         : pFrameData  ���ݰ�
*                 iFrameLen   ���ݰ�����
*                 i64TimeStamp ʱ���
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int GBRtpPsOverUdpStream::inputFrameData(unsigned char* pFrameData, int iFrameLen, unsigned long long i64TimeStamp)
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

    int iPsLen = h264PsMux(pFrameData, iFrameLen, Type, (unsigned long long)m_ulTimeStamp, m_pPsBuff);
    if (iPsLen > 0)
    {        //fwrite(m_pPsBuff, 1, iPsLen, fpt);        ////ÿ�η���1400�ֽ�����
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
                //��������
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
* description   : һ����������Ƶ֡���ֿ�RTP����
* input         : pBlockData  ���ݰ�
*                 iBlockLen   ���ݰ�����
*                 ulIndex     ������
*                 ulTimeStamp ʱ���
*                 bEndHead    �Ƿ�Ϊ������
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int GBRtpPsOverUdpStream::sendOneBlock(unsigned char *pBlockData, int iBlockLen, unsigned long ulIndex, unsigned long ulTimeStamp, bool bEndHead)
{
    int len = 0;

    const int iMaxSendBufSize = 2048;
    if (iBlockLen > iMaxSendBufSize)
    {
        VTDU_LOG_E("block size: " << iBlockLen << ", max size: " << iMaxSendBufSize);
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

     /*RTPͷ�������ϣ������Ǽ򵥴���RTPͷ*/

    unsigned char pSendBuff[iMaxSendBufSize] = { 0 };

    pSendBuff[0] = 0x80;

    if (bEndHead)
    {
        pSendBuff[1] = 0xE0;
    }
    else
        pSendBuff[1] = 0x60;

    //����
    pSendBuff[2] = ((ulIndex & 0xFF00) >> 8);
    pSendBuff[3] = ulIndex & 0x00FF;

    //ʱ���
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

    //���Ϳͻ���
    {
        std::lock_guard<std::mutex> lock(mtSendList);
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
                VTDU_LOG_E("select RtpSend failed, port: " << itor->second.nSendPort);
            }
            else
            {
                if (FD_ISSET(itor->second.fdSend, &writeFdSet))
                {
                    //����
                    sendret = sendto(itor->second.fdSend, (char*)pSendBuff, len, 0, (sockaddr*)&(itor->second.stClientAddr), sizeof(sockaddr));

                }
            }
        }
    }

    return sendret;
}

/**************************************************************************
* name          : V3StreamWorking
* description   : �����v3ƽ̨������
* input         : NA
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void GBRtpPsOverUdpStream::V3StreamWorking()
{
    const int iRecvBuffLen = 4096;
    char szRecvBuff[iRecvBuffLen] = { 0 };
    long long i64LastTime = Comm_GetMilliSecFrom1970();
    unsigned long ulNowTick = Comm_GetTickCount();
    unsigned long ulSenderId = htonl(ulNowTick);
    bool bRecvPacket = false;
    long long i64LastRecvTime = Comm_GetSecFrom1970();
    unsigned short usLastRtpSeq = 0;
    //test
    int nRecvCount = 0;
    int nRecvInvalidCount = 0;

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
            VTDU_LOG_E("m_fdRtpRecv select failed, port: " << m_nRecvPort);
            //֪ͨ����ʧ��
            if (NULL != m_pFuncCb)
            {
                m_pFuncCb(0, m_strPuInfo, m_pUserPara);
            }
            break;
        }
        else if (0 == ret)
        {
            long long nCurTime = Comm_GetSecFrom1970();
            //�յ���һ�����ݰ��Ժ󣬿�ʼ��� ����15��û�������ж�Ϊ������������Դ���ϱ�vtdu.
            if (bRecvPacket && nCurTime - i64LastRecvTime >= 15)
            {
                VTDU_LOG_E("video stream cut off,last time: " << i64LastRecvTime << ", curtime: " << nCurTime);
                //֪ͨ����ʧ��
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

                if (m_iRawArrElemCount >= RAW_DATA_ARRAY_MAX_SIZE)
                {
                    VTDU_LOG_E("stream[%s] raw data array elem count: " << m_iRawArrElemCount);
                    m_iRawArrElemCount = 0;
                }

                if (m_iCurrWriteIndex < 0 || m_iCurrWriteIndex >= RAW_DATA_ARRAY_MAX_SIZE)
                {
                    m_iCurrWriteIndex = 0;
                }

                m_tLockRawDataArr.lock();
                ret = recvfrom(m_fdRtpRecv, m_rawDataArr[m_iCurrWriteIndex].pFrameData, CHANNEL_MAX_FRAME_DATA_LEN, 0, (struct sockaddr*)&addrFrom, (int*)&iAddrLen);
                if (ret > 0)
                {
                    nRecvCount++;
                    if (ret < 12)
                    {
                        nRecvInvalidCount++;
                        VTDU_LOG_E("rtp��̫С,from: " << inet_ntoa(addrFrom.sin_addr) <<"_" << ntohs(addrFrom.sin_port) << ", len: " << ret);
                        m_tLockRawDataArr.unlock();
                        continue;
                    }
                    m_rawDataArr[m_iCurrWriteIndex].iDataLen = ret;

                    unsigned char* pRtpData = (unsigned char*)m_rawDataArr[m_iCurrWriteIndex].pFrameData;

                    m_iRawArrElemCount++;
                    m_iCurrWriteIndex++;
                    bRecvPacket = true;
                    i64LastRecvTime = Comm_GetSecFrom1970();

                    m_tLockRawDataArr.unlock();
                    unsigned short seq = (pRtpData[2] << 8) | pRtpData[3];
                    if ((usLastRtpSeq + 1) != seq) //rtp seq�����������ֶ���
                    {
                        if ((0xFFFF == usLastRtpSeq && 0x0000 == seq) || (0x0000 == usLastRtpSeq && 0x0000 == seq))
                        {
                            //���seq���»ص�0, ���߳�ʼseq=0������ʾ����
                        }
                        else
                        {
                            VTDU_LOG_E("stream rtp seq error, last=" << usLastRtpSeq << ",curr=" << seq);
                        }

                    }
                    usLastRtpSeq = seq;

                    //test
                    if (0 == nRecvCount % 50)
                    {
                        VTDU_LOG_I("stream get pak count:" << nRecvCount << ",invlid count:" << nRecvInvalidCount);
                    }

                    long long i64CurTime = Comm_GetMilliSecFrom1970();
                    long long i64TimeGap = i64CurTime - i64LastTime;
                    if (i64TimeGap >= 5000) //5��ظ�һ��RTCP receiver report��Ϣ
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
                            //rtcp��ַΪ����RTP�˿��Զ�+1
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

        ////����rtcp�����ظ�
        //fd_set readRtcpFdSet;
        //FD_ZERO(&readRtcpFdSet);
        //FD_SET(m_fdRtcpRecv, &readRtcpFdSet);
        //ret = select(m_fdRtcpRecv + 1, &readRtcpFdSet, NULL, NULL, &tv);
        //if (ret < 0)
        //{
        //    VTDU_LOG_E("m_fdRtcpRecv select failed,port: " << m_nRecvPort + 1);
        //    continue;
        //}
        //else if (0 == ret)
        //{
        //    continue;
        //}
        //else
        //{
        //    if (FD_ISSET(m_fdRtcpRecv, &readRtcpFdSet))
        //    {
        //        struct sockaddr_in addrFrom;
        //        int iAddrLen = sizeof(addrFrom);
        //        ret = recvfrom(m_fdRtcpRecv, szRecvBuff, iRecvBuffLen, 0, (struct sockaddr*)&addrFrom, (int*)&iAddrLen);
        //        if (ret > 0)
        //        {
        //            if (iRecvBuffLen >= 8)
        //            {
        //                szRecvBuff[1] = 0xC9;
        //                szRecvBuff[2] = 0x00;
        //                szRecvBuff[3] = 0x01;
        //                unsigned short usRtpFromPort = ntohs(addrFrom.sin_port);
        //                struct sockaddr_in addrToRtcp;
        //                memcpy(&addrToRtcp, &addrFrom, iAddrLen);
        //                addrToRtcp.sin_port = htons(usRtpFromPort + 1);
        //                sendto(m_fdRtcpRecv, (char*)szRecvBuff, iRecvBuffLen, 0, (sockaddr*)&addrToRtcp, sizeof(sockaddr));
        //            }
        //        }
        //    }
        //}
    }

    return ;
}

/**************************************************************************
* name          : HiStreamWorking
* description   : �����Hiת��ģ�������
* input         : NA
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void GBRtpPsOverUdpStream::HiStreamWorking()
{
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
            VTDU_LOG_E("m_fdRtpRecv select failed,port: " << m_nRecvPort);
            //֪ͨ����ʧ��
            if (NULL != m_pFuncCb)
            {
                m_pFuncCb(1, m_strPuInfo, m_pUserPara);
            }
            break;
        }
        else if (0 == ret)
        {
            long long nCurTime = Comm_GetSecFrom1970();
            //�յ���һ�����ݰ��Ժ󣬿�ʼ��� ����15��û�������ж�Ϊ������������Դ���ϱ�vtdu.
            if (bRecvPacket && nCurTime - i64LastRecvTime >= 15)
            {
                VTDU_LOG_E("video stream cut off,last time: " << i64LastRecvTime << ", curtime: " << nCurTime);
                //֪ͨ����ʧ��
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

                        inputFrameData(firstPacket.data, firstPacket.length, 0);
                    }
                }
            }
        }
    }

    return;
}

/**************************************************************************
* name          : SendStreamWorking
* description   : ����������
* input         : NA
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void GBRtpPsOverUdpStream::SendStreamWorking()
{
    //test 
    int nSendCount = 0;
    int nSendSuccess = 0;
    while (!m_bWorkStop)
    {
        char *pFrameData = NULL;
        int iFrameDataLen = 0;

        if (m_iRawArrElemCount <= 0)
        {
            //����������û֡��raw����
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
            continue;
        }

        if (m_iCurrReadIndex < 0 || m_iCurrReadIndex >= RAW_DATA_ARRAY_MAX_SIZE)
        {
            m_iCurrReadIndex = 0;
        }

        m_tLockRawDataArr.lock();
        pFrameData = m_rawDataArr[m_iCurrReadIndex].pFrameData;
        iFrameDataLen = m_rawDataArr[m_iCurrReadIndex].iDataLen;

        m_iRawArrElemCount--;
        m_iCurrReadIndex++;

        m_tLockRawDataArr.unlock();
        nSendCount++;
        {
            std::lock_guard<std::mutex> lock(mtSendList);
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
                    VTDU_LOG_E("select RtpSend failed,port: " << itor->second.nSendPort);
                }
                else
                {
                    if (FD_ISSET(itor->second.fdSend, &writeFdSet))
                    {
                        nSendSuccess++;
                        //����
                        int sendret = sendto(itor->second.fdSend, (char*)pFrameData, iFrameDataLen, 0, (sockaddr*)&(itor->second.stClientAddr), sizeof(sockaddr));
                    }
                }
            }
        }
        if (0 == nSendCount % 50)
        {
            VTDU_LOG_I("send count: " << nSendCount << "send success count: " << nSendSuccess);
        }
    }
}


//rtcp�ṹ��ת���ֽ�����,������Ҫ�����������ֽ���
/**************************************************************************
* name          : makeRtcpPacketBuff
* description   : ����rtcp��
* input         : ulSenderId  ����id
*                 ssrc        ssrc
*                 ulTimeStamp ʱ���
*                 usSeq       ���к�
*                 pOutBuff    ���rtcp��
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int GBRtpPsOverUdpStream::makeRtcpPacketBuff(unsigned long ulSenderId, unsigned long ssrc,
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
    //����receiver report
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

int GBRtpPsOverUdpStream::insertFrameNode(unsigned char *pFrameData, int iLen, unsigned int ulTimeStamp)
{
    if (iLen <= 0 || iLen > CHANNEL_MAX_FRAME_DATA_LEN)
    {
        VTDU_LOG_E("frame data len[%d] out of range"<< iLen);
        return -1;
    }

    std::lock_guard<std::mutex> lock(m_tLockRawDataArr);
    if (m_iRawArrElemCount > RAW_DATA_ARRAY_MAX_SIZE)
    {
        VTDU_LOG_E("stream[%s] raw data array elem count: "<< m_iRawArrElemCount);
        m_iRawArrElemCount = 0;
        return -1;
    }

    if (m_iCurrWriteIndex < 0 || m_iCurrWriteIndex >= RAW_DATA_ARRAY_MAX_SIZE)
    {
        m_iCurrWriteIndex = 0;
    }

    memcpy(m_rawDataArr[m_iCurrWriteIndex].pFrameData, pFrameData, iLen);
    m_rawDataArr[m_iCurrWriteIndex].iDataLen = iLen;
    m_rawDataArr[m_iCurrWriteIndex].ulTimeStamp = ulTimeStamp;
    m_iRawArrElemCount++;
    m_iCurrWriteIndex++;

    return iLen;
}

