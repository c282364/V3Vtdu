/**   @file    XmlParse.cpp
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
#include "XmlParse.h"

/**************************************************************************
* name          : getMsgCmdTypeV3
* description   : 获取v3请求信息
* input         : pszBody   v3请求消息体
*                 iBodyLen  v3请求消息体长度
* output        : NA
* return        : 消息类型
* remark        : NA
**************************************************************************/
std::string XmlParse::getMsgCmdTypeV3(const char *pszBody, int iBodyLen)
{
    std::string strCmd = "";
    TiXmlDocument doc;
    doc.Parse(pszBody);

    TiXmlElement *pXmlElementHeader = doc.FirstChildElement("IE_HEADER");
    if (NULL == pXmlElementHeader)
    {
        VTDU_LOG_E("getMsgCmdTypeV3 cannot find IE_HEADER: " << pszBody);
        doc.Clear();
        return strCmd;
    }

    TiXmlElement *pXmlElementMessageType = pXmlElementHeader->FirstChildElement("MessageType");
    if (NULL == pXmlElementMessageType)
    {
        VTDU_LOG_E("getMsgCmdTypeV3 cannot find pXmlElementMessageType: " << pszBody);
        doc.Clear();
        return strCmd;
    }

    strCmd = pXmlElementMessageType->GetText();
    doc.Clear();
    return strCmd;
}

/**************************************************************************
* name          : ParseXmlTransReady
* description   : 解析v3预览请求信息
* input         : pszBody   v3请求消息体
* output        : stCurMediaInfo 解析后的请求信息
*                 strError       错误消息
* return        : 200 正常 大于200异常
* remark        : NA
**************************************************************************/
int XmlParse::ParseXmlTransReady(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError)
{
    //解析消息体 / 获取本地接收发送端口
    TiXmlDocument doc;
    doc.Parse(szBody);

    //cu
    TiXmlElement *pXmlElementCuInfo = doc.FirstChildElement("IE_CU_ADDRINFO");
    if (NULL == pXmlElementCuInfo)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find IE_CU_ADDRINFO: " << szBody);
        doc.Clear();
        strError = "cannot find IE_CU_ADDRINFO";
        return 400;
    }

    TiXmlElement *pXmlElementCuUserID = pXmlElementCuInfo->FirstChildElement("UserID");
    if (NULL == pXmlElementCuUserID)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find UserID: " << szBody);
        doc.Clear();
        strError = "cannot find CU UserID";
        return 400;
    }
    stCurMediaInfo.strCuUserID = pXmlElementCuUserID->GetText();

    TiXmlElement *pXmlElementCuIP = pXmlElementCuInfo->FirstChildElement("IP");
    if (NULL == pXmlElementCuIP)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find CU IP: " << szBody);
        doc.Clear();
        strError = "cannot find CU IP";
        return 400;
    }
    stCurMediaInfo.strCuIp = pXmlElementCuIP->GetText();

    TiXmlElement *pXmlElementCuPort = pXmlElementCuInfo->FirstChildElement("Port");
    if (NULL == pXmlElementCuPort)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find CU Port: " << szBody);
        doc.Clear();
        strError = "cannot find CU Port";
        return 400;
    }
    std::string strCuPort = pXmlElementCuPort->GetText();
    stCurMediaInfo.nCuport = atoi(strCuPort.c_str());

    TiXmlElement *pXmlElementCuNat = pXmlElementCuInfo->FirstChildElement("Nat");
    if (NULL == pXmlElementCuNat)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find CU NAT: " << szBody);
        doc.Clear();
        strError = "cannot find CU NAT";
        return 400;
    }
    std::string strCuNat = pXmlElementCuNat->GetText();
    stCurMediaInfo.nCuNat = atoi(strCuNat.c_str());

    TiXmlElement *pXmlElementCuTransType = pXmlElementCuInfo->FirstChildElement("TransType");
    if (NULL == pXmlElementCuTransType)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find CU TransType: " << szBody);
        doc.Clear();
        strError = "cannot find CU TransType";
        return 400;
    }
    std::string strCuTransType = pXmlElementCuTransType->GetText();
    stCurMediaInfo.nCuTransType = atoi(strCuTransType.c_str());

    std::string strCuUserType = "";
    TiXmlElement *pXmlElementCuUserType = pXmlElementCuInfo->FirstChildElement("UserType");
    if (NULL == pXmlElementCuUserType)
    {
        VTDU_LOG_I("ParseXmlTransReady cannot find CU UserType: " << szBody);
        stCurMediaInfo.nCuUserType = 1;
    }
    else
    {
        strCuUserType = pXmlElementCuUserType->GetText();
        stCurMediaInfo.nCuUserType = atoi(strCuUserType.c_str());
    }

    TiXmlElement *pXmlElementCuGBVideoTransMode = pXmlElementCuInfo->FirstChildElement("GBVideoTransMode");
    if (NULL == pXmlElementCuGBVideoTransMode)
    {
        VTDU_LOG_I("ParseXmlTransReady cannot find CU GBVideoTransMode: " << szBody);
        stCurMediaInfo.strCuGBVideoTransMode = "UDP";
    }
    else
    {
        stCurMediaInfo.strCuGBVideoTransMode = pXmlElementCuGBVideoTransMode->GetText();
    }

    TiXmlElement *pXmlElementCuMSCheck = pXmlElementCuInfo->FirstChildElement("MSCheck");
    if (NULL == pXmlElementCuMSCheck)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find CU MSCheck: " << szBody);
        doc.Clear();
        strError = "cannot find CU MSCheck";
        return 400;
    }
    std::string strCuMSCheck = pXmlElementCuMSCheck->GetText();
    stCurMediaInfo.nMSCheck = atoi(strCuMSCheck.c_str());


    //pu
    TiXmlElement *pXmlElementPuInfo = doc.FirstChildElement("IE_PU_ADDRINFO");
    if (NULL == pXmlElementPuInfo)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find IE_PU_ADDRINFO: " << szBody);
        doc.Clear();
        strError = "cannot find IE_PU_ADDRINFO";
        return 400;
    }

    TiXmlElement *pXmlElementPuPUID = pXmlElementPuInfo->FirstChildElement("PUID");
    if (NULL == pXmlElementPuPUID)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find PUID: " << szBody);
        doc.Clear();
        strError = "cannot find PUID";
        return 400;
    }
    stCurMediaInfo.strPUID = pXmlElementPuPUID->GetText();

    TiXmlElement *pXmlElementPuChannelID = pXmlElementPuInfo->FirstChildElement("ChannelID");
    if (NULL == pXmlElementPuChannelID)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find ChannelID: " << szBody);
        doc.Clear();
        strError = "cannot find PU ChannelID";
        return 400;
    }
    std::string strPuChannelID = pXmlElementPuChannelID->GetText();
    stCurMediaInfo.nPuChannelID = atoi(strPuChannelID.c_str());

    TiXmlElement *pXmlElementPuProtocol = pXmlElementPuInfo->FirstChildElement("Protocol");
    if (NULL == pXmlElementPuProtocol)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find PU Protocol: " << szBody);
        doc.Clear();
        strError = "cannot find PU Protocol";
        return 400;
    }
    std::string strPuProtocol = pXmlElementPuProtocol->GetText();
    stCurMediaInfo.nPuProtocol = atoi(strPuProtocol.c_str());

    TiXmlElement *pXmlElementPuStreamType = pXmlElementPuInfo->FirstChildElement("StreamType");
    if (NULL == pXmlElementPuStreamType)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find PU StreamType: " << szBody);
        doc.Clear();
        strError = "cannot find PU StreamType";
        return 400;
    }
    std::string strPuStreamType = pXmlElementPuStreamType->GetText();
    stCurMediaInfo.nPuStreamType = atoi(strPuStreamType.c_str());

    TiXmlElement *pXmlElementPuTransType = pXmlElementPuInfo->FirstChildElement("TransType");
    if (NULL == pXmlElementPuTransType)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find PU TransType: " << szBody);
        doc.Clear();
        strError = "cannot find PU TransType";
        return 400;
    }
    std::string strPuTransType = pXmlElementPuTransType->GetText();
    stCurMediaInfo.nPuTransType = atoi(strPuTransType.c_str());

    TiXmlElement *pXmlElementPuFCode = pXmlElementPuInfo->FirstChildElement("FCode");
    if (NULL == pXmlElementPuFCode)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find FCode: " << szBody);
        doc.Clear();
        strError = "cannot find PU FCode";
        return 400;
    }
    std::string strPuFCode = pXmlElementPuFCode->GetText();
    stCurMediaInfo.nPuFCode = atoi(strPuFCode.c_str());

    //IE_VTDU_ROUTE
    TiXmlElement *pXmlElementVtduRoute = doc.FirstChildElement("IE_VTDU_ROUTE");
    if (NULL == pXmlElementVtduRoute)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find IE_VTDU_ROUTE: " << szBody);
        doc.Clear();
        strError = "cannot find IE_VTDU_ROUTE";
        return 400;
    }

    TiXmlElement *pXmlElementVtduRecvIP = pXmlElementVtduRoute->FirstChildElement("RecvIP");
    if (NULL == pXmlElementVtduRecvIP)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find RecvIP: " << szBody);
        doc.Clear();
        strError = "cannot find RecvIP";
        return 400;
    }
    stCurMediaInfo.strVtduRecvIp = pXmlElementVtduRecvIP->GetText();

    TiXmlElement *pXmlElementVtduSendIP = pXmlElementVtduRoute->FirstChildElement("SendIP");
    if (NULL == pXmlElementVtduSendIP)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find SendIP: " << szBody);
        doc.Clear();
        strError = "cannot find SendIP";
        return 400;
    }
    stCurMediaInfo.strVtduSendIP = pXmlElementVtduSendIP->GetText();

    //IE_OPEN_VIDEO
    TiXmlElement *pXmlElementOpenVideo = doc.FirstChildElement("IE_OPEN_VIDEO");
    if (NULL == pXmlElementOpenVideo)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find IE_OPEN_VIDEO: " << szBody);
        doc.Clear();
        strError = "cannot find IE_OPEN_VIDEO";
        return 400;
    }

    TiXmlElement *pXmlElementOpenVideoStreamType = pXmlElementOpenVideo->FirstChildElement("StreamType");
    if (NULL == pXmlElementOpenVideoStreamType)
    {
        VTDU_LOG_E("ParseXmlTransReady cannot find OpenVideoStreamType: " << szBody);
        doc.Clear();
        strError = "cannot find OpenVideoStreamType";
        return 400;
    }
    std::string strOpenVideoStreamType = pXmlElementOpenVideoStreamType->GetText();
    stCurMediaInfo.nOpenVideoStreamType = atoi(strOpenVideoStreamType.c_str());

    TiXmlElement *pXmlElementOpenVideoGBVideoTransMode = pXmlElementOpenVideo->FirstChildElement("GBVideoTransMode");
    if (NULL == pXmlElementOpenVideoGBVideoTransMode)
    {
        VTDU_LOG_I("ParseXmlTransReady cannot find GBVideoTransMode: " << szBody);
        stCurMediaInfo.strOpenVideoGBVideoTransMode = "UDP";
    }
    else
    {
        stCurMediaInfo.strOpenVideoGBVideoTransMode = pXmlElementOpenVideoGBVideoTransMode->GetText();
    }

    //IE_TRANSINFO //tbd 是否需要转码
    TiXmlElement *pXmlTRANSINFO = doc.FirstChildElement("IE_TRANSINFO");
    if (NULL == pXmlTRANSINFO)
    {
        VTDU_LOG_I("ParseXmlTransReady cannot find IE_TRANSINFO: " << szBody);
        stCurMediaInfo.bTrans = false;
    }
    else
    {
        stCurMediaInfo.bTrans = true;
    }

    doc.Clear();

    return 200;
}

/**************************************************************************
* name          : ParseXmlTransStop
* description   : 解析v3停止预览请求信息
* input         : pszBody   v3请求消息体
* output        : stCurMediaInfo 解析后的请求信息
*                 strError       错误消息
* return        : 200 正常 大于200异常
* remark        : NA
**************************************************************************/
int XmlParse::ParseXmlTransStop(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError)
{
    //解析消息体 / 获取本地接收发送端口
    TiXmlDocument doc;
    doc.Parse(szBody);

    //cu
    TiXmlElement *pXmlElementPuInfo = doc.FirstChildElement("IE_STOP_VIDEO");
    if (NULL == pXmlElementPuInfo)
    {
        VTDU_LOG_E("ParseXmlTransStop cannot find IE_STOP_VIDEO: " << szBody);
        doc.Clear();
        strError = "cannot find IE_STOP_VIDEO";
        return 400;
    }


    TiXmlElement *pXmlElementPuPUID = pXmlElementPuInfo->FirstChildElement("PUID");
    if (NULL == pXmlElementPuPUID)
    {
        VTDU_LOG_E("ParseXmlTransStop cannot find PUID: " << szBody);
        doc.Clear();
        strError = "cannot find PUID";
        return 400;
    }
    stCurMediaInfo.strPUID = pXmlElementPuPUID->GetText();

    TiXmlElement *pXmlElementPuChannelID = pXmlElementPuInfo->FirstChildElement("ChannelID");
    if (NULL == pXmlElementPuChannelID)
    {
        VTDU_LOG_E("ParseXmlTransStop cannot find PU ChannelID: " << szBody);
        doc.Clear();
        strError = "cannot find PU ChannelID";
        return 400;
    }
    std::string strPuChannelID = pXmlElementPuChannelID->GetText();
    stCurMediaInfo.nPuChannelID = atoi(strPuChannelID.c_str());


    TiXmlElement *pXmlElementPuStreamType = pXmlElementPuInfo->FirstChildElement("StreamType");
    if (NULL == pXmlElementPuStreamType)
    {
        VTDU_LOG_E("ParseXmlTransStop cannot find PU StreamType: " << szBody);
        doc.Clear();
        strError = "cannot find PU StreamType";
        return 400;
    }
    std::string strPuStreamType = pXmlElementPuStreamType->GetText();
    stCurMediaInfo.nPuStreamType = atoi(strPuStreamType.c_str());

    TiXmlElement *pXmlElementUserInfo = doc.FirstChildElement("IE_USER_INFO");
    if (NULL == pXmlElementUserInfo)
    {
        VTDU_LOG_E("ParseXmlTransStop cannot find IE_USER_INFO: " << szBody);
        doc.Clear();
        strError = "cannot find IE_USER_INFO";
        return 400;
    }
    TiXmlElement *pXmlElementCuUserID = pXmlElementUserInfo->FirstChildElement("UserID");
    if (NULL == pXmlElementCuUserID)
    {
        VTDU_LOG_E("ParseXmlTransStop cannot find UserID: " << szBody);
        doc.Clear();
        strError = "cannot find CU UserID";
        return 400;
    }
    stCurMediaInfo.strCuUserID = pXmlElementCuUserID->GetText();

    //IE_TRANSINFO //tbd 是否需要转码
    TiXmlElement *pXmlTRANSINFO = doc.FirstChildElement("IE_TRANSINFO");
    if (NULL == pXmlTRANSINFO)
    {
        VTDU_LOG_E("ParseXmlTransStop cannot find IE_TRANSINFO: " << szBody);
        stCurMediaInfo.bTrans = false;
    }
    else
    {
        stCurMediaInfo.bTrans = true;
    }

    doc.Clear();

    return 200;
}

/**************************************************************************
* name          : ParseRegisterSuccessXml
* description   : 解析v3注册成功消息
* input         : pszBody   v3请求消息体
* output        : nErrorcode 错误码
* return        : 200 正常 大于200异常
* remark        : NA
**************************************************************************/
int XmlParse::ParseRegisterSuccessXml(const char* szBody, int &nErrorcode)
{
    //解析消息体
    TiXmlDocument doc;
    doc.Parse(szBody);

    //cu
    TiXmlElement *pXmlElementResult = doc.FirstChildElement("IE_RESULT");
    if (NULL == pXmlElementResult)
    {
        VTDU_LOG_E("ParseRegisterSuccessXml cannot find IE_RESULT: " << szBody);
        doc.Clear();
        return -1;
    }

    TiXmlElement *pXmlElementErrorCode = pXmlElementResult->FirstChildElement("ErrorCode");
    if (NULL == pXmlElementErrorCode)
    {
        VTDU_LOG_E("ParseRegisterSuccessXml cannot find ErrorCode: " << szBody);
        doc.Clear();
        return -1;
    }
    std::string strErrorCode = pXmlElementErrorCode->GetText();
    int nErrorCode = atoi(strErrorCode.c_str());

    return 0;
}

/**************************************************************************
* name          : ParseXmlV3FileStart
* description   : 解析v3回放请求信息
* input         : pszBody   v3请求消息体
* output        : stCurMediaInfo 解析后的请求信息
*                 strError       错误消息
* return        : 200 正常 大于200异常
* remark        : NA
**************************************************************************/
int XmlParse::ParseXmlV3FileStart(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError)
{
    //解析消息体 / 获取本地接收发送端口
    TiXmlDocument doc;
    doc.Parse(szBody);

    //rtsp
    TiXmlElement *pXmlElementCuInfo = doc.FirstChildElement("IE_RTSP_INFO");
    if (NULL == pXmlElementCuInfo)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find IE_RTSP_INFO: " << szBody);
        doc.Clear();
        strError = "cannot find IE_RTSP_INFO";
        return 400;
    }

    TiXmlElement *pXmlElementCuSwssion = pXmlElementCuInfo->FirstChildElement("Session");
    if (NULL == pXmlElementCuSwssion)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find Session: " << szBody);
        doc.Clear();
        strError = "cannot find CU Session";
        return 400;
    }
    stCurMediaInfo.strCuSession = pXmlElementCuSwssion->GetText();


    TiXmlElement *pXmlElementCuIP = pXmlElementCuInfo->FirstChildElement("ClientIP");
    if (NULL == pXmlElementCuIP)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find ClientIP: " << szBody);
        doc.Clear();
        strError = "cannot find  ClientIP";
        return 400;
    }
    stCurMediaInfo.strCuIp = pXmlElementCuIP->GetText();

    TiXmlElement *pXmlElementCuPort = pXmlElementCuInfo->FirstChildElement("Port");
    if (NULL == pXmlElementCuPort)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find CU Port: " << szBody);
        doc.Clear();
        strError = "cannot find CU Port";
        return 400;
    }
    std::string strCuPort = pXmlElementCuPort->GetText();
    stCurMediaInfo.strCuPort = strCuPort;
    int nPos = strCuPort.find_first_of("-");
    strCuPort = strCuPort.substr(0, nPos);
    stCurMediaInfo.nCuport = atoi(strCuPort.c_str());

    TiXmlElement *pXmlElementCuTransMode = pXmlElementCuInfo->FirstChildElement("TransMode");
    if (NULL == pXmlElementCuTransMode)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find CU TransMode: " << szBody);
        doc.Clear();
        strError = "cannot find CU TransMode";
        return 400;
    }
    std::string strTransMode = pXmlElementCuTransMode->GetText();
    stCurMediaInfo.nCuTranmode = atoi(strTransMode.c_str());

    TiXmlElement *pXmlElementCuUserID = pXmlElementCuInfo->FirstChildElement("UserID");
    if (NULL == pXmlElementCuUserID)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find CU UserID: " << szBody);
        doc.Clear();
        strError = "cannot find CU UserID";
        return 400;
    }
    stCurMediaInfo.strCuUserID = pXmlElementCuUserID->GetText();

    //IE_VTDU_ROUTE
    TiXmlElement *pXmlElementVtduRoute = doc.FirstChildElement("IE_VTDU_ROUTE");
    if (NULL == pXmlElementVtduRoute)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find IE_VTDU_ROUTE: " << szBody);
        doc.Clear();
        strError = "cannot find IE_VTDU_ROUTE";
        return 400;
    }

    TiXmlElement *pXmlElementVtduRecvIP = pXmlElementVtduRoute->FirstChildElement("RecvIP");
    if (NULL == pXmlElementVtduRecvIP)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find RecvIP: " << szBody);
        doc.Clear();
        strError = "cannot find RecvIP";
        return 400;
    }
    stCurMediaInfo.strVtduRecvIp = pXmlElementVtduRecvIP->GetText();

    TiXmlElement *pXmlElementVtduSendIP = pXmlElementVtduRoute->FirstChildElement("SendIP");
    if (NULL == pXmlElementVtduSendIP)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find SendIP: " << szBody);
        doc.Clear();
        strError = "cannot find SendIP";
        return 400;
    }
    stCurMediaInfo.strVtduSendIP = pXmlElementVtduSendIP->GetText();


    //IE_NETLINK
    TiXmlElement *pXmlElementNetlink = doc.FirstChildElement("IE_NETLINK");
    if (NULL == pXmlElementNetlink)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find IE_NETLINK: " << szBody);
        doc.Clear();
        strError = "cannot find IE_NETLINK";
        return 400;
    }

    TiXmlElement *pXmlElementTransType = pXmlElementNetlink->FirstChildElement("TransType");
    if (NULL == pXmlElementTransType)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find TransType: " << szBody);
        doc.Clear();
        strError = "cannot find TransType";
        return 400;
    }
    std::string strTransType = pXmlElementVtduSendIP->GetText();
    stCurMediaInfo.nPuTransType = atoi(strTransType.c_str());

    //IE_STREAM
    TiXmlElement *pXmlElementStream = doc.FirstChildElement("IE_STREAM");
    if (NULL == pXmlElementStream)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find IE_STREAM: " << szBody);
        doc.Clear();
        strError = "cannot find IE_STREAM";
        return 400;
    }

    TiXmlElement *pXmlElementMediaType = pXmlElementStream->FirstChildElement("MediaType");
    if (NULL == pXmlElementMediaType)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find MediaType: " << szBody);
        doc.Clear();
        strError = "cannot find MediaType";
        return 400;
    }
    std::string strMediaType = pXmlElementMediaType->GetText();
    stCurMediaInfo.nMediaType = atoi(strMediaType.c_str());

    TiXmlElement *pXmlElementCuMSCheck = pXmlElementStream->FirstChildElement("MSCheck");
    if (NULL == pXmlElementCuMSCheck)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find CU MSCheck: " << szBody);
        doc.Clear();
        strError = "cannot find CU MSCheck";
        return 400;
    }
    std::string strCuMSCheck = pXmlElementCuMSCheck->GetText();
    stCurMediaInfo.nMSCheck = atoi(strCuMSCheck.c_str());

    TiXmlElement *pXmlElementProtocol = pXmlElementStream->FirstChildElement("Protocol");
    if (NULL == pXmlElementProtocol)
    {
        VTDU_LOG_E("ParseXmlV3FileStart cannot find Protocol: " << szBody);
        doc.Clear();
        strError = "cannot find Protocol";
        return 400;
    }
    std::string strProtocol = pXmlElementProtocol->GetText();
    stCurMediaInfo.nPuProtocol = atoi(strProtocol.c_str());

    //IE_TRANSINFO //tbd 是否需要转码
    TiXmlElement *pXmlTRANSINFO = doc.FirstChildElement("IE_TRANSINFO");
    if (NULL == pXmlTRANSINFO)
    {
        VTDU_LOG_I("ParseXmlV3FileStart cannot find IE_TRANSINFO: " << szBody);
        stCurMediaInfo.bTrans = false;
    }
    else
    {
        stCurMediaInfo.bTrans = true;
    }

    doc.Clear();

    return 200;
}

/**************************************************************************
* name          : ParseXmlV3FileStop
* description   : 解析v3停止回放请求信息
* input         : pszBody   v3请求消息体
* output        : stCurMediaInfo 解析后的请求信息
*                 strError       错误消息
* return        : 200 正常 大于200异常
* remark        : NA
**************************************************************************/
int XmlParse::ParseXmlV3FileStop(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError)
{
    //解析消息体 / 获取本地接收发送端口
    TiXmlDocument doc;
    doc.Parse(szBody);

    //cu
    TiXmlElement *pXmlElementRtspInfo = doc.FirstChildElement("IE_RTSP_INFO");
    if (NULL == pXmlElementRtspInfo)
    {
        VTDU_LOG_E("ParseXmlV3FileStop cannot find IE_RTSP_INFO: " << szBody);
        doc.Clear();
        strError = "cannot find IE_RTSP_INFO";
        return 400;
    }


    TiXmlElement *pXmlElementSession = pXmlElementRtspInfo->FirstChildElement("Session");
    if (NULL == pXmlElementSession)
    {
        VTDU_LOG_E("ParseXmlV3FileStop cannot find Session: " << szBody);
        doc.Clear();
        strError = "cannot find Session";
        return 400;
    }
    stCurMediaInfo.strPUID = pXmlElementSession->GetText();

    TiXmlElement *pXmlElementUserID = pXmlElementRtspInfo->FirstChildElement("UserID");
    if (NULL == pXmlElementUserID)
    {
        VTDU_LOG_E("ParseXmlV3FileStop cannot find PU ChannelID: " << szBody);
        doc.Clear();
        strError = "cannot find PU ChannelID";
        return 400;
    }
    stCurMediaInfo.strCuUserID = pXmlElementUserID->GetText();

    //IE_TRANSINFO //tbd 是否需要转码
    TiXmlElement *pXmlTRANSINFO = doc.FirstChildElement("IE_TRANSINFO");
    if (NULL == pXmlTRANSINFO)
    {
        VTDU_LOG_I("ParseXmlV3FileStop cannot find PU IE_TRANSINFO: " << szBody);
        stCurMediaInfo.bTrans = false;
    }
    else
    {
        stCurMediaInfo.bTrans = true;
    }

    doc.Clear();

    return 200;
}

/**************************************************************************
* name          : ParseXmlHiReg
* description   : 解析hi注册请求信息
* input         : pszBody  hi请求消息体
* output        : stCurHi 解析后的注册信息
*                 strError       错误消息
* return        :  0 正常 小于0异常
* remark        : NA
**************************************************************************/
int XmlParse::ParseXmlHiReg(const char* szBody, stHiInfo& stCurHi, std::string &strError)
{
    TiXmlDocument doc;
    doc.Parse(szBody);
    //cu
    TiXmlElement *pXmlElementSipInfo = doc.FirstChildElement("HI_SipInfo");
    if (NULL == pXmlElementSipInfo)
    {
        VTDU_LOG_E("ParseXmlHiReg cannot find HI_SipInfo: " << szBody);
        doc.Clear();
        strError = "cannot find HI_SipInfo";
        return -1;
    }

    TiXmlElement *pXmlElementSipId = pXmlElementSipInfo->FirstChildElement("HI_SipId");
    if (NULL == pXmlElementSipId)
    {
        VTDU_LOG_E("ParseXmlHiReg cannot find HI_SipId: " << szBody);
        doc.Clear();
        strError = "cannot find HI_SipId";
        return -1;
    }
    stCurHi.strSipId = pXmlElementSipId->GetText();

    TiXmlElement *pXmlElementSipRegion = pXmlElementSipInfo->FirstChildElement("HI_SipRegion");
    if (NULL == pXmlElementSipRegion)
    {
        VTDU_LOG_E("ParseXmlHiReg cannot find HI_SipRegion: " << szBody);
        strError = "cannot find HI_SipRegion";
        return -1;
    }
    stCurHi.strSipRegion = pXmlElementSipRegion->GetText();

    TiXmlElement *pXmlElementSipIp = pXmlElementSipInfo->FirstChildElement("HI_SipIp");
    if (NULL == pXmlElementSipIp)
    {
        VTDU_LOG_E("ParseXmlHiReg cannot find HI_SipIp: " << szBody);
        doc.Clear();
        strError = "cannot find HI_SipIp";
        return -1;
    }
    stCurHi.strSipIp = pXmlElementSipIp->GetText();

    TiXmlElement *pXmlElementSipPort = pXmlElementSipInfo->FirstChildElement("HI_SipPort");
    if (NULL == pXmlElementSipPort)
    {
        VTDU_LOG_E("ParseXmlHiReg cannot find HI_SipPort: " << szBody);
        strError = "cannot find HI_SipPort";
        return -1;
    }
    std::string strSipPort = pXmlElementSipPort->GetText();
    stCurHi.nSipPort = atoi(strSipPort.c_str());

    TiXmlElement *pXmlElementMaxTransTaskNum = pXmlElementSipInfo->FirstChildElement("HI_MaxTransTaskNum");
    if (NULL == pXmlElementMaxTransTaskNum)
    {
        VTDU_LOG_E("ParseXmlHiReg cannot find HI_MaxTransTaskNum: " << szBody);
        strError = "cannot find HI_MaxTransTaskNum";
        return -1;
    }
    std::string strMaxTransTaskNum = pXmlElementMaxTransTaskNum->GetText();
    stCurHi.nMaxTransTaskNum = atoi(strMaxTransTaskNum.c_str());

    doc.Clear();
    return 0;
}

/**************************************************************************
* name          : ParseXmlHiHeartBeat
* description   : 解析hi心跳信息
* input         : pszBody  hi请求消息体
* output        : stCurHi 解析后的注册信息
* output        : bFindSipInfo 是否查找到sip消息
* return        :  0 正常 小于0异常
* remark        : NA
**************************************************************************/
int XmlParse::ParseXmlHiHeartBeat(const char* szBody, stHiInfo& stCurHi, bool& bFindSipInfo)
{
    TiXmlDocument doc;
    doc.Parse(szBody);

    TiXmlElement *pXmlElementHiInfo = doc.FirstChildElement("IE_HI_INFO");
    if (NULL == pXmlElementHiInfo)
    {
        VTDU_LOG_E("ParseXmlHiHeartBeat cannot find IE_HI_INFO: " << szBody);
        doc.Clear();
        return -1;
    }

    TiXmlElement *pXmlElementSipId = pXmlElementHiInfo->FirstChildElement("HI_SipId");
    if (NULL == pXmlElementSipId)
    {
        VTDU_LOG_E("ParseXmlHiHeartBeat cannot find pXmlElementSipId: " << szBody);
        doc.Clear();
        return -1;
    }
    stCurHi.strSipId = pXmlElementSipId->GetText();

    bFindSipInfo = false;
    do
    {
        TiXmlElement *pXmlElementSipRegion = pXmlElementHiInfo->FirstChildElement("HI_SipRegion");
        if (NULL == pXmlElementSipRegion)
        {
            VTDU_LOG_E("ParseXmlHiHeartBeat cannot find HI_SipRegion: " << szBody);
            break;
        }
        stCurHi.strSipRegion = pXmlElementSipRegion->GetText();

        TiXmlElement *pXmlElementSipIp = pXmlElementHiInfo->FirstChildElement("HI_SipIp");
        if (NULL == pXmlElementSipIp)
        {
            VTDU_LOG_E("ParseXmlHiHeartBeat cannot find pXmlElementSipIp: " << szBody);
            break;
        }
        stCurHi.strSipIp = pXmlElementSipIp->GetText();

        TiXmlElement *pXmlElementSipPort = pXmlElementHiInfo->FirstChildElement("HI_SipPort");
        if (NULL == pXmlElementSipPort)
        {
            VTDU_LOG_E("ParseXmlHiHeartBeat cannot find HI_SipPort: " << szBody);
            break;
        }
        std::string strSipPort = pXmlElementSipPort->GetText();
        stCurHi.nSipPort = atoi(strSipPort.c_str());

        TiXmlElement *pXmlElementMaxTransTaskNum = pXmlElementHiInfo->FirstChildElement("HI_MaxTransTaskNum");
        if (NULL == pXmlElementMaxTransTaskNum)
        {
            VTDU_LOG_E("ParseXmlHiHeartBeat cannot find HI_MaxTransTaskNum: " << szBody);
            break;
        }
        std::string strMaxTransTaskNum = pXmlElementMaxTransTaskNum->GetText();
        stCurHi.nMaxTransTaskNum = atoi(strMaxTransTaskNum.c_str());
        bFindSipInfo = true;
    } while (0);

    doc.Clear();
    return 0;
}

/**************************************************************************
* name          : ParseXmlHiTansRsp
* description   : 解析hi转码回复信息
* input         : pszBody  hi请求消息体
* output        : strHiTaskId 任务id
*                 nHiRecvPort hi码流接收端口
* return        :  0 正常 小于0异常
* remark        : NA
**************************************************************************/
int XmlParse::ParseXmlHiTansRsp(const char* szBody, std::string& strHiTaskId, int& nHiRecvPort)
{
    TiXmlDocument doc;
    doc.Parse(szBody);

    TiXmlElement *pXmlElementStreamInfo = doc.FirstChildElement("VTDU_STREAM_INFO");
    if (NULL == pXmlElementStreamInfo)
    {
        VTDU_LOG_E("ParseXmlHiTansRsp cannot find VTDU_STREAM_INFO: " << szBody);
        doc.Clear();
        return -1;
    }

    TiXmlElement *pXmlElementCuHiRecvPort = pXmlElementStreamInfo->FirstChildElement("HI_RECV_PORT");
    if (NULL == pXmlElementCuHiRecvPort)
    {
        VTDU_LOG_E("ParseXmlHiTansRsp cannot find HI_RECV_PORT: " << szBody);
        doc.Clear();
        return -1;;
    }
    std::string strHiRecvPort = pXmlElementCuHiRecvPort->GetText();
    nHiRecvPort = atoi(strHiRecvPort.c_str());

    TiXmlElement *pXmlElementTaskId = pXmlElementStreamInfo->FirstChildElement("VTDU_TASK_ID");
    if (NULL == pXmlElementTaskId)
    {
        VTDU_LOG_E("ParseXmlHiTansRsp cannot find VTDU_TASK_ID: " << szBody);
        doc.Clear();
        return -1;;
    }
    strHiTaskId = pXmlElementTaskId->GetText();

    doc.Clear();
    return 0;
}

/**************************************************************************
* name          : ParseXmlHiCutout
* description   : 解析hi断流信息
* input         : pszBody  hi请求消息体
* output        : strHiTaskId 任务id
* return        :  0 正常 小于0异常
* remark        : NA
**************************************************************************/
int XmlParse::ParseXmlHiCutout(const char* szBody, std::string& strHiTaskId)
{
    TiXmlDocument doc;
    doc.Parse(szBody);

    TiXmlElement *pXmlElementStreamInfo = doc.FirstChildElement("VTDU_STREAM_INFO");
    if (NULL == pXmlElementStreamInfo)
    {
        VTDU_LOG_E("ParseXmlHiCutout cannot find VTDU_STREAM_INFO: " << szBody);
        doc.Clear();
        return -1;
    }

    TiXmlElement *pXmlElementTaskId = pXmlElementStreamInfo->FirstChildElement("VTDU_TASK_ID");
    if (NULL == pXmlElementTaskId)
    {
        VTDU_LOG_E("ParseXmlHiCutout cannot find VTDU_TASK_ID: " << szBody);
        doc.Clear();
        return -1;
    }
    strHiTaskId = pXmlElementTaskId->GetText();

    doc.Clear();
    return 0;
}