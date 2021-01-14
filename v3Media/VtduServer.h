/**   @file    VtduServer.h
*    @note    All Right Reserved.
*    @brief   vtdu服务主处理模块
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
#include "stdafx.h"
#include <iostream>
#include <eXosip2/eXosip.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#endif
#include "tinyxml.h"
#include <string>
#include <vector>
#include <list>
#include <mutex>
#include <map>
#include <thread>
#include "GBRtpPsStream.h"
#include "SipV3.h"
#include "communicationHi.h"
#include "XmlParse.h"
#include "ConfigFileParse.h"
#include "CommDef.h"

class VtduServer
{
public:
    VtduServer();
    virtual ~VtduServer();

    /**************************************************************************
    * name          : Init
    * description   : 初始化
    * input         : strCfgFile  配置文件路径
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int Init(const std::string &strCfgFile);

    /**************************************************************************
    * name          : Fini
    * description   : 反初始化
    * input         : NA
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int Fini();

    /**************************************************************************
    * name          : Start
    * description   : 启动
    * input         : NA
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int Start();

    /**************************************************************************
    * name          : Stop
    * description   : 停止
    * input         : NA
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int Stop();

    /**************************************************************************
    * name          : sipClientHandleRegisterSuccess
    * description   : 处理注册成功消息
    * input         : pMsgPtr sip消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipClientHandleRegisterSuccess(void *pMsgPtr);

    /**************************************************************************
    * name          : sipClientHandleRegisterFailed
    * description   : 处理注册失败
    * input         : pMsgPtr sip消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipClientHandleRegisterFailed(void *pMsgPtr);

    /**************************************************************************
    * name          : sipServerSaveInfoReq
    * description   : 保存info消息
    * input         : pMsgPtr sip消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerSaveInfoReq(void *pMsgPtr);

    /**************************************************************************
    * name          : sipServerHandleInfo
    * description   : 处理info消息
    * input         : stCurTask sip消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleInfo(const stuGbEvent &stCurTask);


    /**************************************************************************
    * name          : sipClientHandleOptionFailed
    * description   : option消息发送失败，option消息作为心跳发送失败判断为服务端掉线
    * input         : pMsgPtr sip消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipClientHandleOptionFailed();

    /**************************************************************************
    * name          : HandleHiTransMessage
    * description   : Hi转码上块上报消息处理
    * input         : pMsgPtr    收到的消息
    *                 nLen       收到的消息长度
    * output        : szSendBuff 回复消息
    *                 nSendBuf   回复消息长度
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiTransMessage(char *pMsgPtr, int nLen, char* szSendBuff, int* nSendBuf);

    /**************************************************************************
    * name          : HandleStreamInfoCallBack
    * description   : 处理流模块上报消息
    * input         : nType     消息类型 0：v3断流 1hi模块断流
    *                 strCbInfo 消息内容
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleStreamInfoCallBack(int nType, std::string strCbInfo);

    /**************************************************************************
    * name          : HandleHiRegister
    * description   : 处理hi注册请求
    * input         : pMsgPtr   hi注册消息
    * output        : szSendBuff 需要回复hi的消息
    *                 nSendBufLen 需要回复hi的消息长度
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiRegister(char *pMsgPtr, char* szSendBuff, int* nSendBufLen);

    /**************************************************************************
    * name          : HandleHiHeartBeat
    * description   : 处理hi心跳消息
    * input         : pMsgPtr   hi心跳消息
    * output        : szSendBuff 需要回复hi的消息
    *                 nSendBufLen 需要回复hi的消息长度
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiHeartBeat(char *pMsgPtr, char* szSendBuff, int* nSendBufLen);

    /**************************************************************************
    * name          : HandleHiTansRsp
    * description   : 处理hi转码回复消息
    * input         : pMsgPtr   hi回复消息
    * output        : szSendBuff 需要回复hi的消息
    *                 nSendBufLen 需要回复hi的消息长度
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiTansRsp(char *pMsgPtr, char* szSendBuff, int* nSendBufLen);

    /**************************************************************************
    * name          : HandleHiCutout
    * description   : 处理hi断流消息
    * input         : pMsgPtr   hi回复消息
    * output        : szSendBuff 需要回复hi的消息
    *                 nSendBufLen 需要回复hi的消息长度
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiCutout(char *pMsgPtr, char* szSendBuff, int* nSendBufLen);

    /**************************************************************************
    * name          : threadHandleGbEvent
    * description   : 处理国标请求线程
    * input         : pParam 用户参数
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    void threadHandleGbEvent();

    /**************************************************************************
    * name          : threadHiManager
    * description   : 转码模块保活管理线程
    * input         : pParam 用户参数
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    void threadHiManager();

    /**************************************************************************
    * name          : threadRegLoop
    * description   : 注册v3 sip线程
    * input         : pParam 用户参数
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    void threadRegLoop();

    /**************************************************************************
    * name          : threadHeartBeat
    * description   : v3 sip心跳线程
    * input         : pParam 用户参数
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    void threadHeartBeat();

    /**************************************************************************
    * name          : threadStreamCBMsgLoop
    * description   : 视频流处理模块上报消息接收线程
    * input         : pParam 用户参数
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    void threadStreamCBMsgLoop();

    /**************************************************************************
    * name          : sipServerHandleV3TransReadyTest
    * description   : 处理v3预览请求 测试函数
    * input         : pMsgPtr   v3消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleV3TransReadyTest(stMediaInfo &stCurMediaInfo);

 private:
    /**************************************************************************
    * name          : sipServerHandleV3TransReady
    * description   : 处理v3预览请求
    * input         : pMsgPtr   v3消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleV3TransReady(const stuGbEvent &stCurTask);

    /**************************************************************************
    * name          : sipServerHandleV3TransStop
    * description   : 处理v3停止预览请求
    * input         : pMsgPtr   v3消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleV3TransStop(const stuGbEvent &stCurTask);

    /**************************************************************************
    * name          : sipServerHandleV3FileStart
    * description   : 处理v3回放请求
    * input         : pMsgPtr   v3消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleV3FileStart(const stuGbEvent &stCurTask);

    /**************************************************************************
    * name          : sipServerHandleV3FileStop
    * description   : 处理v3停止回放请求
    * input         : pMsgPtr   v3消息
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleV3FileStop(const stuGbEvent &stCurTask);

    /**************************************************************************
    * name          : RecoverPairPort
    * description   : 回收一对收发端口
    * input         : nPortSend  发送端口
                    : nPortRecv 接收
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void RecoverPairPort(int nPortSend, int nPortRecv);

    /**************************************************************************
    * name          : RecoverPort
    * description   : 回收端口
    * input         : enPortType  端口类型
                    : nPort 端口
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void RecoverPort(PORT_type enPortType, int nPort);

    /**************************************************************************
    * name          : GetOneHi
    * description   : 获取一个已注册的hi模块
    * input         : NA
    * output        : stCurHiInfo hi信息
                      strError 错误信息
    * return        : 0正常 小于0 异常
    * remark        : NA
    **************************************************************************/
    int GetOneHi(stHiInfo &stCurHiInfo, std::string &strError);

    /**************************************************************************
    * name          : GetSendPort
    * description   : 获取一个vtdu发流端口
    * input         : NA
    * output        : nSendV3Port 发流端口
                      strError 错误信息
    * return        : 0正常 小于0 异常
    * remark        : NA
    **************************************************************************/
    int GetSendPort(int &nSendV3Port, std::string &strError);

    /**************************************************************************
    * name          : GetRecvPort
    * description   : 获取一个vtdu收流端口
    * input         : NA
    * output        : nRecvV3Port 收流端口
                      strError 错误信息
    * return        : 0正常 小于0 异常
    * remark        : NA
    **************************************************************************/
    int GetRecvPort(int &nRecvV3Port, std::string &strError);


private:
    ConfigSipServer m_configSipServer;//vtdu配置

    std::vector<int> g_vecSendV3Port;                             //发送端口列表
    std::vector<int> g_vecRecvPort;                               //接收端口列表
    std::map<std::string, stHiInfo> g_mapRegHiInfo;               //已注册转码模块列表
    std::map<std::string, stHiTaskInfo> g_mapVtduPreviewTaskInfo; //视频任务列表
    std::map<std::string, int> g_mapGetHirecvPort;                //hi转码模块上报接收端口列表
    std::vector<stCutOffInfo> g_vecCutOff;                         //断流模块列表
    std::list<stuGbEvent> m_lstGBtask; //国标拉流请求列表

    std::mutex mtRegHi;           //已注册转码模块列表锁
    std::mutex mtVtduPreviewTask; //视频任务列表锁
    std::mutex mtSendV3Port;      //发送端口列表锁
    std::mutex mtRecvPort;        //接收端口列表锁
    std::mutex mtGetHirecvPort;   //hi转码模块上报接收端口列表锁
    std::mutex mtCutOff;          //断流模块列表锁
    std::mutex mtGBtask;          //国标拉流列表锁

    int g_nRegid;     //v3 sip注册id
    bool g_bStopHeartBeat; //v3心跳线程停止标志
    bool g_bStopReg;       //v3注册线程停止标志
    __time64_t g_nLastRegTime; //上一次注册v3平台时间

    int g_nInstreamNum;    //输入视频路数
    int g_nOutstreamNum;   //输出视频路数
    bool g_bStopHiManager; //转码模块管理线程停止标志

    communicationHi m_oCommunicationHi; //hi模块交互类
    XmlParse        m_oXmlParse;        //xml解析类
};

