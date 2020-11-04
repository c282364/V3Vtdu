#pragma once
#include "CommDef.h"

class communicationHi
{
public:
    communicationHi();
    virtual ~communicationHi();

    /**************************************************************************
    * name          : Init
    * description   : 初始化
    * input         : configSipServer vtdu配置信息
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int Init(const ConfigSipServer &configSipServer);

    /**************************************************************************
    * name          : FIni
    * description   : 反初始化
    * input         : NA
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int FIni();

    /**************************************************************************
    * name          : StartEventLoop
    * description   : 开始消息接收
    * input         : fnCB 消息上报函数
    *                 pParam 用户指针
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int StartEventLoop(fHiEventCB fnCB, void *pParam);

    /**************************************************************************
    * name          : threadHiMsgLoop
    * description   : hi模块上报消息接收线程
    * input         : pParam 用户参数
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    void threadHiMsgLoop();

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
    int sendTransReq(int nRecvPort, const std::string &strPuInfo, const std::string &strHiIp, int nHiPort);

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
    int sendStopTransReq(const std::string &strPuInfo, const std::string &strHiIp, int nHiPort);

    /**************************************************************************
    * name          : sendReconnect
    * description   : 发送重连请求
    * input         : strHiIp  转码模块ip
    *                 nHiPort  转码模块端口
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int sendReconnect(const std::string &strHiIp, int nHiPort);

private:
    int m_fd2Vtdu;         //vtdu和hi转码模块通信socket
    fHiEventCB m_CBFunc;   //消息回调函数
    void *m_pEventCBParam; //上层句柄
};

