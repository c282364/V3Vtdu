/**   @file    XmlParse.h
*    @note    All Right Reserved.
*    @brief   xml解析类
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
#include "CommDef.h"
#include <iostream>
#include <stdio.h>
#include <stdlib.h>
#include "tinyxml.h"
#include <string>
#include <vector>
#include <mutex>
#include <map>
class XmlParse
{
public:
    /**************************************************************************
    * name          : getMsgCmdTypeV3
    * description   : 获取v3请求信息
    * input         : pszBody   v3请求消息体
    *                 iBodyLen  v3请求消息体长度
    * output        : NA
    * return        : 消息类型
    * remark        : NA
    **************************************************************************/
    std::string getMsgCmdTypeV3(const char *pszBody, int iBodyLen);

    /**************************************************************************
    * name          : ParseXmlTransReady
    * description   : 解析v3预览请求信息
    * input         : pszBody   v3请求消息体
    * output        : stCurMediaInfo 解析后的请求信息
    *                 strError       错误消息
    * return        : 200 正常 大于200异常
    * remark        : NA
    **************************************************************************/
    int ParseXmlTransReady(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError);

    /**************************************************************************
    * name          : ParseXmlTransStop
    * description   : 解析v3停止预览请求信息
    * input         : pszBody   v3请求消息体
    * output        : stCurMediaInfo 解析后的请求信息
    *                 strError       错误消息
    * return        : 200 正常 大于200异常
    * remark        : NA
    **************************************************************************/
    int ParseXmlTransStop(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError);

    /**************************************************************************
    * name          : ParseRegisterSuccessXml
    * description   : 解析v3注册成功消息
    * input         : pszBody   v3请求消息体
    * output        : nErrorcode 错误码
    * return        : 200 正常 大于200异常
    * remark        : NA
    **************************************************************************/
    int ParseRegisterSuccessXml(const char* szBody, int &nErrorcode);

    /**************************************************************************
    * name          : ParseXmlV3FileStart
    * description   : 解析v3回放请求信息
    * input         : pszBody   v3请求消息体
    * output        : stCurMediaInfo 解析后的请求信息
    *                 strError       错误消息
    * return        : 200 正常 大于200异常
    * remark        : NA
    **************************************************************************/
    int ParseXmlV3FileStart(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError);

    /**************************************************************************
    * name          : ParseXmlV3FileStop
    * description   : 解析v3停止回放请求信息
    * input         : pszBody   v3请求消息体
    * output        : stCurMediaInfo 解析后的请求信息
    *                 strError       错误消息
    * return        : 200 正常 大于200异常
    * remark        : NA
    **************************************************************************/
    int ParseXmlV3FileStop(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError);

    /**************************************************************************
    * name          : ParseXmlHiReg
    * description   : 解析hi注册请求信息
    * input         : pszBody  hi请求消息体
    * output        : stCurHi 解析后的注册信息
    *                 strError       错误消息
    * return        :  0 正常 小于0异常
    * remark        : NA
    **************************************************************************/
    int ParseXmlHiReg(const char* szBody, stHiInfo& stCurHi, std::string &strError);

    /**************************************************************************
    * name          : ParseXmlHiHeartBeat
    * description   : 解析hi心跳信息
    * input         : pszBody  hi请求消息体
    * output        : stCurHi 解析后的注册信息
    *                 bFindSipInfo 是否查找到sip消息
    * return        :  0 正常 小于0异常
    * remark        : NA
    **************************************************************************/
    int ParseXmlHiHeartBeat(const char* szBody, stHiInfo& stCurHi, bool& bFindSipInfo);

    /**************************************************************************
    * name          : ParseXmlHiTansRsp
    * description   : 解析hi转码回复信息
    * input         : pszBody  hi请求消息体
    * output        : strHiTaskId 任务id
    *                 nHiRecvPort hi码流接收端口
    * return        :  0 正常 小于0异常
    * remark        : NA
    **************************************************************************/
    int ParseXmlHiTansRsp(const char* szBody, std::string& strHiTaskId, int& nHiRecvPort);

    /**************************************************************************
    * name          : ParseXmlHiCutout
    * description   : 解析hi断流信息
    * input         : pszBody  hi请求消息体
    * output        : strHiTaskId 任务id
    * return        :  0 正常 小于0异常
    * remark        : NA
    **************************************************************************/
    int ParseXmlHiCutout(const char* szBody, std::string& strHiTaskId);

        
    

    

};

