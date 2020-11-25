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
    * description   : 启动
    * input         : bTans 是否需要转码
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int start(bool bTans);

    /**************************************************************************
    * name          : stop
    * description   : 停止
    * input         : NA
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int stop();

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
    int addOneSend(const std::string &strClientIp, int nClientPort, const std::string &strCuUserID, int nSendPort);

    /**************************************************************************
    * name          : DelOneSend
    * description   : 删除一个接收方
    * input         : strCuUserID  接收方id
    *                 nCurSendPort 发送端口
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int DelOneSend(const std::string &strCuUserID, int &nCurSendPort);

    /**************************************************************************
    * name          : getRecvPort
    * description   : 获取接收端口
    * input         : NA
    * output        : nRecvPort 发送端口
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int getRecvPort(int &nRecvPort);

    /**************************************************************************
    * name          : getOutstreamNum
    * description   : 获取分发路数
    * input         : NA
    * output        : nOutstreamNum 分发路数
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int getOutstreamNum(int &nOutstreamNum);

    /**************************************************************************
    * name          : V3StreamWorking
    * description   : 处理从v3平台接收流
    * input         : NA
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void V3StreamWorking();

    /**************************************************************************
    * name          : HiStreamWorking
    * description   : 处理从Hi转码模块接收流
    * input         : NA
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HiStreamWorking();

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
    int inputFrameData(unsigned char* pFrameData, int iFrameLen, unsigned long long i64TimeStamp);

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
    int sendOneBlock(unsigned char *pBlockData, int iBlockLen, unsigned long ulIndex, unsigned long ulTimeStamp, bool bEndHead);

private:
    ////rtcp结构体转成字节数组,参数都要求输入网络字节序
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
    int makeRtcpPacketBuff(unsigned long ulSenderId, unsigned long ssrc,
        unsigned long ulTimeStamp, unsigned short usSeq, unsigned char *pOutBuff);

public:
    int m_nRecvPort; //接收端口
    int m_nOutNum;  //发送路数
    bool m_bTrans;  //是否需要转码
private:
    bool m_bWorkStop;       //工作线程停止标志
    std::thread m_hWorkThreadV3; //v3流接收线程句柄
    std::thread m_hWorkThreadHi; //转码模块流接收线程句柄

    int m_fdRtpRecv;  //rtp接收socket
    int m_fdRtcpRecv; //rtcp接收socket

    std::string m_strLocalIp;

    unsigned long m_ulSSRC;      //ssrc
    unsigned short m_usSeq;      //seq
    unsigned long m_ulTimeStamp; //time stamp
    unsigned char *m_pPsBuff;    //ps流缓存

    std::string m_strPuInfo; //任务唯一id
};

