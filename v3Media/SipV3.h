/**   @file    SipV3.h
*    @note    All Right Reserved.
*    @brief   v3平台稀有gb协议。
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
#include <Windows.h>
#include <process.h>
#endif
#include "tinyxml.h"
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include "GBRtpPsOverUdpStream.h"
#include "CommDef.h"

/**************************************************************************
* name          : SipUA_Init
* description   : 初始化
* input         : stSipCfg sip配置信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_Init(const ConfigSipServer& stSipCfg);

/**************************************************************************
* name          : SipUA_Fini
* description   : 反初始化
* input         : NA
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_Fini();

/**************************************************************************
* name          : SipUA_StartEventLoop
* description   : 开始SIPUA事件
* input         : fnCB 回调函数
*                 pParam 用户参数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_StartEventLoop(fnSipUAEventCB fnCB, void *pParam);

/**************************************************************************
* name          : SipUA_StartEventLoop
* description   : 结束SIPUA事件
* input         : NA
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_StopEventLoop();

/**************************************************************************
* name          : SipUA_InitMessage
* description   : 发送sip请求
* input         : pszMethod sip请求类型 OPTION/INFO/MESSAGE
*                 pszSrcId 本地sip id
*                 pszSrcIP 本地 ip
*                 iSrcPort 本地 sip 端口
*                 pszDestId 目标 sip id
*                 pszDestIP 目标 sip ip
*                 iDestPort 目标 sip 端口
*                 pszBody sip 消息体
*                 iBodyLen sip 消息体长度
*                 strV3Head sip v3头
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_InitMessage(const char * pszMethod, const char *pszSrcId, const char *pszSrcIP, int iSrcPort, const char *pszDestId, const char *pszDestIP, int iDestPort,
    const char *pszBody, int iBodyLen, std::string strV3Head);

/**************************************************************************
* name          : SipUA_Register
* description   : 发送 sip注册
* input         : pszSipId        本地sip id
*                 pszSipRegion    本地sip Region
*                 pszSipAddr      本地 ip
*                 iSipPort        本地 sip 端口
*                 expires         注册有效期
*                 pszServerId     目标 sip id
*                 pszServerRegion sip服务器Region
*                 pszServerIP     sip服务器 ip
*                 iServerPort     sip服务器 端口
*                 pszBody         sip消息体
*                 iBodyLen        sip消息体长度
*                 nRegid          sip 注册id 如果不是-1代表已注册
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_Register(const char *pszSipId, const char * pszSipRegion, const char *pszSipAddr, int iSipPort, int expires,
    const char *pszServerId, const char * pszServerRegion, const char *pszServerIP, int iServerPort,
    const char *pszBody, int iBodyLen, int nRegid);

/**************************************************************************
* name          : SipUA_AnswerInfo
* description   : 回复 sip请求消息
* input         : pMsgPtr  收到的请求消息
*                 status   状态码
*                 pszBody  sip消息体
*                 iBodyLen sip消息体长度
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_AnswerInfo(void *pMsgPtr, int status, const char *pszBody, int iBodyLen);

/**************************************************************************
* name          : SipUA_GetRequestBodyContent
* description   : 获取v3请求消息体
* input         : pMsgPtr  收到的请求消息
*                 iMaxBodyLen 输出消息体最大长度
* output        : pszOutBody   输出消息体
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_GetRequestBodyContent(void *pMsgPtr, char *pszOutBody, int iMaxBodyLen);

/**************************************************************************
* name          : SipUA_GetResponseBodyContent
* description   : 获取v3回复消息体
* input         : pMsgPtr  收到的请求消息
*                 iMaxBodyLen 输出消息体最大长度
* output        : pszOutBody   输出消息体
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_GetResponseBodyContent(void *pMsgPtr, char *pszOutBody, int iMaxBodyLen);

/**************************************************************************
* name          : SipUA_RspPreviewOk
* description   : 回复v3预览请求成功消息
* input         : pMsgPtr  收到的v3 sip请求消息
*                 stCurMediaInfo 预览请求信息
*                 nInitialUser 接收方路数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RspPreviewOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo, int nInitialUser);

/**************************************************************************
* name          : SipUA_RspPreviewStopOk
* description   : 回复v3停止预览请求成功消息
* input         : pMsgPtr  收到的v3 sip请求消息
*                 stCurMediaInfo 预览请求信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RspPreviewStopOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo);

/**************************************************************************
* name          : SipUA_RspPlayBackOk
* description   : 回复v3回放请求成功消息
* input         : pMsgPtr  收到的v3 sip请求消息
*                 stCurMediaInfo 预览请求信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RspPlayBackOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo);

/**************************************************************************
* name          : SipUA_RspPlayBackStopOk
* description   : 回复v3停止回放请求成功消息
* input         : pMsgPtr  收到的v3 sip请求消息
*                 stCurMediaInfo 预览请求信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RspPlayBackStopOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo);

/**************************************************************************
* name          : SipUA_RegisterV3
* description   : 发送v3 注册请求
* input         : configSipServer  vtdu配置信息
*                 bNeedBody 是否需要组装body
*                 nRegid 注册id 如果大于0 代表是注册更新
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_RegisterV3(const ConfigSipServer &configSipServer, bool bNeedBody, int nRegid);

/**************************************************************************
* name          : SipUA_Timeout
* description   : 发送v3 断流超时
* input         : stCurMediaInfo  任务信息
*                 configSipServer  vtdu配置信息
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_Timeout(const stMediaInfo &stCurMediaInfo, const ConfigSipServer &configSipServer);

/**************************************************************************
* name          : SipUA_HeartBeat
* description   : 发送v3 心跳
* input         : configSipServer  vtdu配置信息
*                 nInstreamNum     输入视频流路数
*                 nOutstreamNum    输出视频流路数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int SipUA_HeartBeat(const ConfigSipServer &configSipServer, int nInstreamNum, int nOutstreamNum);
