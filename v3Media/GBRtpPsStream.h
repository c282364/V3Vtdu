#pragma once
#ifdef WIN32
#include <process.h>
#include <ctime>
#include <cstdlib>
#else
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#endif
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "psmux.h"
#include "Stream.h"
#include <thread>

class GBRtpPsOverUdpStream :public Stream
{
public:
    GBRtpPsOverUdpStream(std::string strPuInfo, int nPortRecv, std::string strLocalIp);
    ~GBRtpPsOverUdpStream();

    /**************************************************************************
    * name          : start
    * description   : ����
    * input         : bTans �Ƿ���Ҫת��
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int start(bool bTans);

    /**************************************************************************
    * name          : stop
    * description   : ֹͣ
    * input         : NA
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int stop();

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
    int addOneSend(const std::string &strClientIp, int nClientPort, const std::string &strCuUserID, int nSendPort);

    /**************************************************************************
    * name          : DelOneSend
    * description   : ɾ��һ�����շ�
    * input         : strCuUserID  ���շ�id
    *                 nCurSendPort ���Ͷ˿�
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int DelOneSend(const std::string &strCuUserID, int &nCurSendPort);

    /**************************************************************************
    * name          : getRecvPort
    * description   : ��ȡ���ն˿�
    * input         : NA
    * output        : nRecvPort ���Ͷ˿�
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int getRecvPort(int &nRecvPort);

    /**************************************************************************
    * name          : getOutstreamNum
    * description   : ��ȡ�ַ�·��
    * input         : NA
    * output        : nOutstreamNum �ַ�·��
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int getOutstreamNum(int &nOutstreamNum);

    /**************************************************************************
    * name          : V3StreamWorking
    * description   : �����v3ƽ̨������
    * input         : NA
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void V3StreamWorking();

    /**************************************************************************
    * name          : HiStreamWorking
    * description   : �����Hiת��ģ�������
    * input         : NA
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HiStreamWorking();

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
    int inputFrameData(unsigned char* pFrameData, int iFrameLen, unsigned long long i64TimeStamp);

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
    int sendOneBlock(unsigned char *pBlockData, int iBlockLen, unsigned long ulIndex, unsigned long ulTimeStamp, bool bEndHead);

private:
    ////rtcp�ṹ��ת���ֽ�����,������Ҫ�����������ֽ���
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
    int makeRtcpPacketBuff(unsigned long ulSenderId, unsigned long ssrc,
        unsigned long ulTimeStamp, unsigned short usSeq, unsigned char *pOutBuff);

public:
    int m_nRecvPort; //���ն˿�
    int m_nOutNum;  //����·��
    bool m_bTrans;  //�Ƿ���Ҫת��
private:
    bool m_bWorkStop;       //�����߳�ֹͣ��־
    std::thread m_hWorkThreadV3; //v3�������߳̾��
    std::thread m_hWorkThreadHi; //ת��ģ���������߳̾��

    int m_fdRtpRecv;  //rtp����socket
    int m_fdRtcpRecv; //rtcp����socket

    std::string m_strLocalIp;

    unsigned long m_ulSSRC;      //ssrc
    unsigned short m_usSeq;      //seq
    unsigned long m_ulTimeStamp; //time stamp
    unsigned char *m_pPsBuff;    //ps������

    std::string m_strPuInfo; //����Ψһid
};

