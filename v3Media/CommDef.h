/**   @file    CommDef.h
*    @note    All Right Reserved.
*    @brief   公共定义文件
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
#pragma once
#include <time.h>
#include <string.h>
#ifdef WIN32
#include <Windows.h>
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
//****************************枚举定义***************************************

//视频任务类型
enum TASK_type
{
    PREVIEW,  //预览
    PLAYBACK  //回放
};

//断流模块类型
enum CUTOFF_type
{
    V3,     //v3平台管理的设备断流
    HiTrans //转码模块断流
};

//流类型
enum Stream_type
{
    UDP_PS,     //PS UDP
    TCP_PS_Initiative, // ps tcp 主动连接
    TCP_PS_Passive, // ps tcp 被连接
    UDP_TS,     //TS UDP
    TCP_TS_Initiative, // TS tcp 主动连接
    TCP_TS_Passive // TS tcp 被连接
};

//sip回调消息类型
typedef enum tagESipUAMsg
{
    SIPUA_MSG_UNKNOWN = 0,
    /*注册类消息*/
    SIPUA_MSG_REGISTER_NEW,           /*收到注册消息，对应EXOSIP_REGISTRATION_NEW，但已作废*/
    SIPUA_MSG_REGISTER_SUCCESS,       /*注册成功*/
    SIPUA_MSG_REGISTER_FAILURE,       /*注册失败*/
    SIPUA_MSG_REGISTER_REFRESH,       /*刷新注册, 对应EXOSIP_REGISTRATION_REFRESHED消息，但已作废*/

    /*呼叫类消息*/
    SIPUA_MSG_CALL_INVITE_INCOMING,   /*收到INVITE */
    SIPUA_MSG_CALL_ANSWER_200OK,      /*200 ok消息*/
    SIPUA_MSG_CALL_ANSWER_RSP,        /*其他非200响应*/
    SIPUA_MSG_CALL_ACK,               /*ack*/
    SIPUA_MSG_CALL_BYE,               /*bye*/

    /*MESSAGE消息*/
    SIPUA_MSG_MESSAGE,                //收到MESSAGE消息
    SIPUA_MSG_MESSAGE_ANSWERED,       //收到MESSAGE消息的200 ok响应
    SIPUA_MSG_MESSAGE_REQUESTFAILURE, //收到MESSAGE消息的错误响应

    /*INFO消息*/
    SIPUA_MSG_INFO,                   //收到INFO消息
    SIPUA_MSG_INFO_ANSWERED,          //收到INFO消息的200 ok响应
    SIPUA_MSG_INFO_REQUESTFAILURE,    //收到INFO消息的错误响应

    /*OPTION消息*/
    SIPUA_MSG_OPTION,                 //收到OPTION消息
    SIPUA_MSG_OPTION_ANSWERED,        //收到OPTION消息的200 ok响应
    SIPUA_MSG_OPTION_REQUESTFAILURE,  //收到OPTION消息的错误响应
}ESipUAMsg;

//****************************结构定义***************************************

//sip 配置信息
typedef struct tagConfigSipServer
{
    std::string m_strCfgFile; //配置文件路径

    /*本地SIP配置参数*/
    std::string m_strSipAddr;
    int m_iSipPort;
    std::string m_strLocalSipId; //"34020100002000000001";
    std::string m_strSipRegion;
    std::string m_strSipDomain;
    std::string m_strPwd;       //"123456";
    int m_iExpires;
    int m_iMonitorKeepalive; //监视下级平台的keepalive超时时间

    /*远端SIP服务器配置参数*/
    std::string m_strServerId;
    std::string m_strServerRegion;
    std::string m_strServerDomain;
    std::string m_strServerIP; //"192.168.2.5"; //
    int m_iServerPort;

    //vtdu和转码模块通信端口
    int m_iVtdu2HiPort;

    //vtdu 视频流收发端口范围
    int m_nRecvV3PortBegin;
    int m_nRecvV3PortEnd;
    int m_nSendV3PortBegin;
    int m_nSendV3PortEnd;
}ConfigSipServer;

//图像像素
typedef struct Ratio
{
    int nWide;
    int nHigh;
}stRatio;

//视频任务信息
typedef struct MediaInfo
{
    //接收方信息
    std::string  strCuSession; //接收Session
    std::string  strCuUserID;  //接收用户id
    std::string  strCuIp;      //接收ip
    int nCuport;               //接收端口
    std::string strCuPort;     //接收端口
    int nCuNat;
    int nCuTranmode;
    int nCuTransType;
    int nCuUserType;
    int nMediaType;
    std::string strCuGBVideoTransMode;
    int nMSCheck;
    //流发送方信息
    std::string strPUID;
    int nPuChannelID;
    int nPuProtocol;
    int nPuStreamType;
    int nPuTransType;
    int nPuFCode;
    int nOpenVideoStreamType;
    std::string strOpenVideoGBVideoTransMode;
    //vtdu本身信息
    std::string strVtduRecvIp; //接收v3平台流ip
    std::string strVtduSendIP; //vtdu发送流端口
    int nVtduRecvPort;         //vtdu接收流端口
    int nVtduSendPort;         //vtdu发送流端口
    bool bTrans;               //视频转码信息       
}stMediaInfo;

//转码模块信息
typedef struct HiInfo
{
    std::string  strSipId;
    std::string  strSipRegion;
    std::string  strSipIp;
    int nSipPort;
    int nTansTaskNum;
    int nMaxTransTaskNum;
    long long nHeartBeat;
}stHiInfo;

//转码任务信息
typedef struct HiTaskInfo
{
    void* pStream; //流处理句柄
    Stream_type enStreamType;
    std::string strHiSipId; //转码模块id
    int nRecvPort;          //接收端口
    std::string strRecvIp;  //接收ip
    stMediaInfo stMedia;    //媒体信息
    TASK_type enTaskType; //0 预览 1回放
}stHiTaskInfo;

//断流上报信息
typedef struct CutOffInfo
{
    std::string strCutOffInfo; //任务id
    CUTOFF_type enCutOffType; //0 v3断流 2 hi模块断流
}stCutOffInfo;

//****************************回调函数定义***************************************
/*SIPUA事件回调函数
@msg: 消息类型参考ESipUAMsg
@pSdpInfo: 由SDP字符串解析出的结构体，INVITE消息会解析，
其他非呼叫类的消息为NULL
@pMsgPtr: 指向消息内容地址的指针
@pszBody/iBodyLen: 消息体字符串/长度
@pParam: 回调函数用户参数
*/
typedef void(*fnSipUAEventCB)(ESipUAMsg msg, void *pMsgPtr, void *pParam);
typedef void(*fHiEventCB)(char *szRecvBuff, int nLen, char* szSendBuff, int* nSendBuf, void *pParam);


/*获取系统开机以来的毫秒数*/
unsigned long Comm_GetTickCount();

/*获取1970年至当前时间的毫秒数*/
long long Comm_GetMilliSecFrom1970();

/*获取1970年至当前时间的秒数*/
long long Comm_GetSecFrom1970();
