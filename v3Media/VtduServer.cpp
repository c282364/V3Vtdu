/**   @file    VtduServer.cpp
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
#include "VtduServer.h"
#include "ConfigFileParse.h"

//sip消息处理函数 //tbd放入无锁队列
void GB28181EventCB(ESipUAMsg msg, void *pMsgPtr, void *pParam)
{
    if (NULL == pMsgPtr || NULL == pParam)
    {
        VTDU_LOG_E("GB28181EventCB, invaild para");
        SipUA_AnswerInfo(pMsgPtr, 400, NULL, 0);
        return;
    }
    VtduServer* poServer = (VtduServer*)pParam;

    switch (msg)
    {
        /************************注册类消息*****************************/
    case SIPUA_MSG_REGISTER_SUCCESS: /*注册成功*/
        poServer->sipClientHandleRegisterSuccess(pMsgPtr);
        break;

    case SIPUA_MSG_REGISTER_FAILURE: /*注册失败*/
        poServer->sipClientHandleRegisterFailed(pMsgPtr);
        break;

    case SIPUA_MSG_INFO: //INFO 消息
        poServer->sipServerSaveInfoReq(pMsgPtr);
        break;
    case SIPUA_MSG_OPTION_REQUESTFAILURE:
        //心跳消息发送失败，判断位sip服务端离线，启动注册线程重新注册 停止心跳线程
        poServer->sipClientHandleOptionFailed();
    default:
        SipUA_AnswerInfo(pMsgPtr, 400, NULL, 0);
        break;
    }
}

void HiEventCB(char *szRecvBuff, int nLen, char* szSendBuff, int* nSendBuf, void *pParam)
{
    if (NULL != szRecvBuff && NULL != szSendBuff && NULL != nSendBuf && NULL != pParam)
    {
        VtduServer* poServer = (VtduServer*)pParam;
        poServer->HandleHiTransMessage(szRecvBuff, nLen, szSendBuff, nSendBuf);
    }
}

//流媒体模块上报消息处理
void StreamInfoCallBack(int nType, std::string strCbInfo, void *pParam)
{
    if (NULL == pParam)
    {
        VTDU_LOG_E("StreamInfoCallBack invaild para");
        return;
    }
    VtduServer* poServer = (VtduServer*)pParam;
    poServer->HandleStreamInfoCallBack(nType, strCbInfo);
}

VtduServer::VtduServer()
    : g_nRegid(-1)
    , g_bStopHeartBeat(true)
    , g_bStopReg(true)
    , g_nLastRegTime(0)
    , g_nInstreamNum(0)
    , g_nOutstreamNum(0)
    , g_bStopHiManager(false)
{

}

VtduServer::~VtduServer()
{
    Fini();
}

/**************************************************************************
* name          : Init
* description   : 初始化
* input         : strCfgFile  配置文件路径
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int VtduServer::Init(const std::string &strCfgFile)
{
    //获取配置信息
    ConfigFileParse oCfg;
    int nRet = oCfg.getCfg(strCfgFile, m_configSipServer);
    if (nRet < 0)
    {
        return -1;
    }

    //收流端口
    {
        std::lock_guard<std::mutex> lock(mtRecvPort);
        for (int i = m_configSipServer.m_nRecvV3PortBegin; i <= m_configSipServer.m_nRecvV3PortEnd;)
        {
            g_vecRecvPort.push_back(i);
            i += 2;
        }
    }

    //发流端口
    {
        std::lock_guard<std::mutex> lock(mtSendV3Port);
        for (int i = m_configSipServer.m_nSendV3PortBegin; i <= m_configSipServer.m_nSendV3PortEnd;)
        {
            g_vecSendV3Port.push_back(i);
            i += 2;
        }
    }

    //sip初始化
    int ret = SipUA_Init(m_configSipServer);
    if (0 > ret)
    {
        return -1;
    }

    //hi模块通信类初始化
    ret = m_oCommunicationHi.Init(m_configSipServer);
    if (0 > ret)
    {
        SipUA_Fini();
        return -1;
    }

    return 0;
}

/**************************************************************************
* name          : Fini
* description   : 反初始化
* input         : NA
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int VtduServer::Fini()
{
    Stop();
    SipUA_Fini();
    m_oCommunicationHi.FIni();
    return 0;
}

/**************************************************************************
* name          : Start
* description   : 启动
* input         : NA
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int VtduServer::Start()
{
    //启动v3 sip交互
    int ret = SipUA_StartEventLoop(GB28181EventCB, this);
    if (0 != ret)
    {
        return -1;
    }

    //启动hi模块交互
    m_oCommunicationHi.StartEventLoop(HiEventCB, this);
    if (0 != ret)
    {
        SipUA_StopEventLoop();
        return -1;
    }

    //注册到v3
    g_nRegid = SipUA_RegisterV3(m_configSipServer, false, g_nRegid);

    //启动转码服务器管理线程，心跳超过60秒未收到 判断离线。停止对应接收线程，注销转码服务器
    static std::thread ThHiManager(&VtduServer::threadHiManager, this);

    //注册v3线程
    static std::thread ThRegLoop(&VtduServer::threadRegLoop, this);

    //v3心跳线程
    static std::thread ThHeartBeat(&VtduServer::threadHeartBeat, this);

    //处理断流回调消息线程
    static std::thread ThCBWorkingThread(&VtduServer::threadStreamCBMsgLoop, this);

    //处理国标请求协议
    static std::thread ThHandleGBEvent(&VtduServer::threadHandleGbEvent, this);

    return 0;
}

/**************************************************************************
* name          : Stop
* description   : 停止
* input         : NA
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int VtduServer::Stop()
{
    //暂时不实现，进程为永不停止
    return 0;
}

/**************************************************************************
* name          : threadHandleGbEvent
* description   : 处理国标请求线程
* input         : pParam 用户参数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
void VtduServer::threadHandleGbEvent()
{
    do 
    {
        int nTaskVecSize = 0;
        stuGbEvent stCurTask;
        memset(stCurTask.szBody, 0, sizeof(stCurTask.szBody));
        {
            std::lock_guard<std::mutex> lock(mtGBtask);
            nTaskVecSize = m_lstGBtask.size();
            if (nTaskVecSize > 0)
            {
                std::list<stuGbEvent>::iterator itor = m_lstGBtask.begin();
                stCurTask.tid = itor->tid;
                stCurTask.nBodyLen = itor->nBodyLen;
                memcpy(stCurTask.szBody, itor->szBody, 4096);
                m_lstGBtask.pop_front();
            }
        }

        if (nTaskVecSize > 0)
        {
            sipServerHandleInfo(stCurTask);
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(5));
        }
    } while (1);
}

/**************************************************************************
* name          : threadHiManager
* description   : 转码模块保活管理线程
* input         : pParam 用户参数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
void VtduServer::threadHiManager()
{
    do
    {
        std::this_thread::sleep_for(std::chrono::seconds(5));
        long long nCurTime = Comm_GetSecFrom1970();
        std::lock_guard<std::mutex> lock(mtRegHi);
        std::map<std::string, stHiInfo>::iterator iotr = g_mapRegHiInfo.begin();
        for (; iotr != g_mapRegHiInfo.end(); )
        {
            VTDU_LOG_I("CHECK HiTrans, id_ip: "<< iotr->second.strSipId << "_" << iotr->second.strSipIp << ", task num: " << iotr->second.nTansTaskNum);
            //15秒没有心跳 判断离线
            if (nCurTime - iotr->second.nHeartBeat > 15)
            {
                VTDU_LOG_I("CHECK HiTrans, Hitans cut off: "<< iotr->second.strSipId << "_" << iotr->second.strSipIp);
                //停止接收 回收该模块所有端口
                std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
                std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.begin();
                for (; itorTask != g_mapVtduPreviewTaskInfo.end();)
                {
                    if (itorTask->second.strHiSipId != "" && itorTask->second.strHiSipId == iotr->first)
                    {
                        Stream *pStream = (Stream *)(itorTask->second.pStream);
                        pStream->stop();
                        //回收所有发送端口
                        std::vector<int> vecSendPort;
                        pStream->getSendPort(vecSendPort);
                        {
                            std::lock_guard<std::mutex> lock(mtSendV3Port);
                            for (std::vector<int>::iterator itorvecSendPort = vecSendPort.begin(); itorvecSendPort != vecSendPort.end(); ++itorvecSendPort)
                            {
                                g_vecSendV3Port.push_back(*itorvecSendPort);
                            }
                        }
                        int nRecvPort = -1;
                        pStream->getRecvPort(nRecvPort);
                        int nOutstreamNum;
                        pStream->getOutstreamNum(nOutstreamNum);
                        //回收接收端口
                        if (-1 != nRecvPort)
                        {
                            std::lock_guard<std::mutex> lock(mtRecvPort);
                            g_vecRecvPort.push_back(nRecvPort);
                        }

                        g_nOutstreamNum -= nOutstreamNum;
                        g_nInstreamNum--;
                        delete pStream;
                        pStream = NULL;
                        //通知v3断流
                        if (itorTask->second.enTaskType == TASK_type::PREVIEW)
                        {
                            stMediaInfo stCurMediaInfo = itorTask->second.stMedia;
                            SipUA_Timeout(stCurMediaInfo, m_configSipServer);
                        }
                        //丢弃任务信息
                        itorTask = g_mapVtduPreviewTaskInfo.erase(itorTask);
                    }
                    else
                    {
                        ++itorTask;
                    }
                }
                iotr = g_mapRegHiInfo.erase(iotr);
            }
            else
            {
                ++iotr;
            }
        }
    } while (!g_bStopHiManager);

    return ;
}

/**************************************************************************
* name          : threadRegLoop
* description   : 注册v3 sip线程
* input         : pParam 用户参数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
void VtduServer::threadRegLoop()
{
    //600秒注册一次
    while (1)
    {
        if (!g_bStopReg)
        {

            g_nRegid = SipUA_RegisterV3(m_configSipServer, false, g_nRegid);

            for (int i = 0; i < 600; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }

    return;
}

/**************************************************************************
* name          : threadHeartBeat
* description   : v3 sip心跳线程
* input         : pParam 用户参数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
void VtduServer::threadHeartBeat()
{
    do
    {
        if (!g_bStopHeartBeat)
        {
            SipUA_HeartBeat(m_configSipServer, g_nInstreamNum, g_nOutstreamNum);
            for (int i = 0; i < 20; i++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(1000));
            }

            //更新注册
            __time64_t nCurRegTime = _time64(NULL);
            if (g_nRegid >= 0 && g_nLastRegTime > 0 && nCurRegTime - g_nLastRegTime > m_configSipServer.m_iExpires - 30)
            {
                g_nRegid = SipUA_RegisterV3(m_configSipServer, false, g_nRegid);
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    } while (1);

    return;
}

/**************************************************************************
* name          : threadStreamCBMsgLoop
* description   : stream模块上报消息处理线程
* input         : pParam 用户参数
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
void VtduServer::threadStreamCBMsgLoop()
{
    while (1)
    {
        bool bGet = false;
        stCutOffInfo stCurCutOffInfo;
        {
            std::lock_guard<std::mutex> lock(mtCutOff);
            if (g_vecCutOff.size() > 0)
            {
                stCurCutOffInfo = g_vecCutOff.back();
                g_vecCutOff.pop_back();
                bGet = true;
            }
        }

        if (bGet)
        {
            VTDU_LOG_I("threadStreamCBMsgLoop get cutoff, strHiTaskId: " << stCurCutOffInfo.strCutOffInfo);
            std::string strHiTaskId = stCurCutOffInfo.strCutOffInfo;
            //按照task id寻找任务信息，释放stream，回收端口
            Stream* pStream = NULL;
            std::string strHiSipId = "";
            stMediaInfo stCurMediaInfo;
            TASK_type enTaskType = TASK_type::PLAYBACK;
            {
                std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
                std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strHiTaskId);
                if (itorTask != g_mapVtduPreviewTaskInfo.end())
                {
                    //父类是否可以这么用 需要测试
                    pStream = (Stream*)(itorTask->second.pStream);
                    stCurMediaInfo = itorTask->second.stMedia;
                    enTaskType = itorTask->second.enTaskType;
                    strHiSipId = itorTask->second.strHiSipId;
                    g_mapVtduPreviewTaskInfo.erase(itorTask);
                }
            }

            if (NULL != pStream)
            {
                pStream->stop();
                //回收所有发送端口
                std::vector<int> vecSendPort;
                pStream->getSendPort(vecSendPort);

                {
                    std::lock_guard<std::mutex> lock(mtSendV3Port);
                    for (std::vector<int>::iterator itorvecSendPort = vecSendPort.begin(); itorvecSendPort != vecSendPort.end(); ++itorvecSendPort)
                    {
                        g_vecSendV3Port.push_back(*itorvecSendPort);
                    }
                }

                //回收接收端口
                int nRecvPort = -1;
                pStream->getRecvPort(nRecvPort);
                if (-1 != nRecvPort)
                {
                    std::lock_guard<std::mutex> lock(mtRecvPort);
                    g_vecRecvPort.push_back(nRecvPort);
                }

                //更新输入输出流路数
                int nOutNum = 0;
                pStream->getOutstreamNum(nOutNum);
                g_nOutstreamNum -= nOutNum;
                g_nInstreamNum--;
                delete pStream;
                pStream = NULL;

                // 通知v3断流
                if (enTaskType == TASK_type::PREVIEW)
                {
                    SipUA_Timeout(stCurMediaInfo, m_configSipServer);
                }
            }

            //通知hi断流
            if (stCurCutOffInfo.enCutOffType == CUTOFF_type::HiTrans)
            {
                //通知hi
                std::string strHiId = "";
                std::string strHiIp = "";
                int nHiPort = 0;

                {
                    std::lock_guard<std::mutex> lock(mtRegHi);
                    std::map<std::string, stHiInfo>::iterator itorRegHi = g_mapRegHiInfo.find(strHiSipId);
                    if (itorRegHi == g_mapRegHiInfo.end())
                    {
                        VTDU_LOG_E("threadStreamCBMsgLoop cannot find hi info, HiSipId::" << strHiSipId);
                    }
                    else
                    {
                        strHiId = itorRegHi->second.strSipId;
                        strHiIp = itorRegHi->second.strSipIp;
                        nHiPort = itorRegHi->second.nSipPort;
                    }
                }

                if (strHiId != "" && strHiIp != "" && nHiPort != 0)
                {
                    m_oCommunicationHi.sendStopTransReq(strHiTaskId, strHiIp, nHiPort);
                }
            }
        }
        else
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(1000));
        }
    }
}

/**************************************************************************
* name          : sipClientHandleRegisterSuccess
* description   : 处理注册成功消息
* input         : pMsgPtr sip消息
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipClientHandleRegisterSuccess(void *pMsgPtr)
{
    VTDU_LOG_I("sipClientHandleRegisterSuccess");
    char szBody[4096] = { 0 };
    int ret = SipUA_GetResponseBodyContent(pMsgPtr, szBody, 4096);
    if (ret <= 0)
    {
        return;
    }
    int nErrorCode = 0;
    int nRet = m_oXmlParse.ParseRegisterSuccessXml(szBody, nErrorCode);
    if (nRet < 0)
    {
        g_bStopHeartBeat = true;
        g_bStopReg = false;
        return;
    }

    if (0 == nErrorCode)
    {
        g_bStopHeartBeat = false;
        g_bStopReg = true;
        g_nLastRegTime = _time64(NULL);
    }
    else
    {
        g_bStopHeartBeat = true;
        g_bStopReg = false;
        VTDU_LOG_E("sipClientHandleRegisterSuccess reg failed! error:" << nErrorCode);
    }
    return;
}

/**************************************************************************
* name          : sipClientHandleRegisterFailed
* description   : 处理注册失败
* input         : pMsgPtr sip消息
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipClientHandleRegisterFailed(void *pMsgPtr)
{
    int nStatus = -1;
    eXosip_event_t *evt = (eXosip_event_t *)pMsgPtr;
    if (evt->response != NULL && evt->response->status_code == 401)
    {
        VTDU_LOG_I("sipClientHandleRegisterFailed 401");
        g_nRegid = SipUA_RegisterV3(m_configSipServer, true, g_nRegid);
    }
    else
    {
        g_nRegid = -1;
        g_bStopHeartBeat = true;
        g_bStopReg = false;
        if (evt->response != NULL)
        {
            VTDU_LOG_E("reg failed status: " << evt->response->status_code);
        }
    }
}

/**************************************************************************
* name          : sipServerSaveInfoReq
* description   : 保存info消息
* input         : pMsgPtr sip消息
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerSaveInfoReq(void *pMsgPtr)
{
    //避免阻塞 放入消息队列

    stuGbEvent stGbEvent;
    memset(stGbEvent.szBody, 0, sizeof(stGbEvent.szBody));

    stGbEvent.tid = ((eXosip_event_t*)pMsgPtr)->tid;
    int ret = SipUA_GetRequestBodyContent(pMsgPtr, stGbEvent.szBody, 4096);
    if (ret <= 0)
    {
        SipUA_AnswerInfo(pMsgPtr, 400, NULL, 0);
        return;
    }
    stGbEvent.nBodyLen = ret;

    {
        std::lock_guard<std::mutex> lock(mtGBtask);
        m_lstGBtask.push_back(stGbEvent);
    }

    VTDU_LOG_I("recv Sip [INFO] Message：" << stGbEvent.szBody);
}

/**************************************************************************
* name          : sipServerHandleInfo
* description   : 处理info消息
* input         : pMsgPtr sip消息
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleInfo(const stuGbEvent &stCurTask)
{
    std::string strCmdType = m_oXmlParse.getMsgCmdTypeV3(stCurTask.szBody, stCurTask.nBodyLen);
    eXosip_event_t evt;
    evt.tid = stCurTask.tid;

    eXosip_event_t* pMsgPtr = &evt;

    if ("MSG_READY_VIDEO_TRANS_REQ" == strCmdType)//启动预览
    {
        sipServerHandleV3TransReady(stCurTask);
    }
    else if ("MSG_START_VIDEO_VTDU_ACK" == strCmdType)//确认预览
    {
        VTDU_LOG_I("recv MSG_START_VIDEO_VTDU_ACK");
        //sipServerHandleV3TransAck(pMsgPtr);
    }
    else if ("MSG_VTDU_STOP_VIDEO_REQ" == strCmdType)//停止预览
    {
        //停止接收发送流，销毁资源，回收端口
        sipServerHandleV3TransStop(stCurTask);
    }
    else if ("MSG_START_FILE_VOD_TASK_REQ" == strCmdType)//启动回放
    {
        sipServerHandleV3FileStart(stCurTask);
    }
    else if ("MSG_STOP_FILE_VOD_TASK_REQ" == strCmdType)//停止回放
    {
        sipServerHandleV3FileStop(stCurTask);
    }
    else
    {
        VTDU_LOG_E("sipServerHandleInfo unsupport Request: " << strCmdType);
        //回复状态
        SipUA_AnswerInfo(pMsgPtr, 400, NULL, 0);
    }
}

/**************************************************************************
* name          : sipClientHandleOptionFailed
* description   : option消息发送失败，option消息作为心跳发送失败判断为服务端掉线
* input         : NA
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipClientHandleOptionFailed()
{
    VTDU_LOG_I("sipClientHandleOptionFailed");
    g_bStopHeartBeat = true;
    g_bStopReg = false;
    g_nRegid = -1;
}

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
void VtduServer::HandleHiTransMessage(char *pMsgPtr, int nLen, char* szSendBuff, int* nSendBuf)
{
    if (NULL == pMsgPtr)
    {
        VTDU_LOG_E("HandleHiTransMessage pMsgPtr is NUL");
        return;
    }
    std::string strCmdType = m_oXmlParse.getMsgCmdTypeV3(pMsgPtr, nLen);
    if ("MSG_HI_REG" == strCmdType)//注册
    {
        VTDU_LOG_I("HandleHiTransMessage handle MSG_HI_REG,body: "<< pMsgPtr);
        HandleHiRegister(pMsgPtr, szSendBuff, nSendBuf);
    }
    else if ("MSG_HI_HEART" == strCmdType)//心跳
    {
        HandleHiHeartBeat(pMsgPtr, szSendBuff, nSendBuf);
    }
    else if ("MSG_HI_RECVPORT_RSP" == strCmdType) //转码请求回复
    {
        HandleHiTansRsp(pMsgPtr, szSendBuff, nSendBuf);
    }
    else if ("MSG_HI_CUTOUT" == strCmdType || "MSG_HI_SELECT_FAILED" == strCmdType) //断流
    {
        VTDU_LOG_I("HandleHiTransMessage handle MSG_HI_CUTOUT,body: "<< pMsgPtr);
        HandleHiCutout(pMsgPtr, szSendBuff, nSendBuf);
    }
    else
    {
        VTDU_LOG_E("HandleHiTransMessage unsupport Request:" << pMsgPtr);
        return;
    }
}

/**************************************************************************
* name          : HandleStreamInfoCallBack
* description   : 处理流模块上报消息
* input         : nType     消息类型 0：v3断流 1hi模块断流
*                 strCbInfo 消息内容
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::HandleStreamInfoCallBack(int nType, std::string strCbInfo)
{
    stCutOffInfo stCurCutOffInfo;
    stCurCutOffInfo.strCutOffInfo = strCbInfo;
    std::lock_guard<std::mutex> lock(mtCutOff);
    switch (nType)
    {
    case 0: //v3断流
        stCurCutOffInfo.enCutOffType = CUTOFF_type::V3;
        g_vecCutOff.push_back(stCurCutOffInfo);
        VTDU_LOG_I("HandleStreamInfoCallBack v3 cut off, task: " << stCurCutOffInfo.strCutOffInfo);
        break;
    case 1: // hi模块断流
        stCurCutOffInfo.enCutOffType = CUTOFF_type::HiTrans;
        g_vecCutOff.push_back(stCurCutOffInfo);
        VTDU_LOG_I("HandleStreamInfoCallBack Hi cut off, task: " << stCurCutOffInfo.strCutOffInfo);
        break;
    default:
        VTDU_LOG_E("StreamInfoCallBack unsuport type:" << nType);
        break;
    }
}

/**************************************************************************
* name          : sipServerHandleV3TransReady
* description   : 处理v3预览请求
* input         : pMsgPtr   v3消息
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleV3TransReady(const stuGbEvent &stCurTask)
{
    int nStatus = 400;
    std::string strError = "";
    stMediaInfo stCurMediaInfo;
    stCurMediaInfo.bTrans = false;
    char pszBody[4096] = { 0 };
    int iBodyLen = 0;

    eXosip_event_t evt;
    evt.tid = stCurTask.tid;

    eXosip_event_t* pMsgPtr = &evt;

    nStatus = m_oXmlParse.ParseXmlTransReady(stCurTask.szBody, stCurMediaInfo, strError);

    stCurMediaInfo.strVtduRecvIp = m_configSipServer.m_strSipAddr;
    stCurMediaInfo.strVtduSendIP = m_configSipServer.m_strSipAddr;

    if (nStatus != 200)
    {
        iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
        //回复状态
        SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
        return;
    }
    else
    {
        //只处理udp tcp暂时不支持
        if (stCurMediaInfo.nCuTransType != 0
            || (stCurMediaInfo.nCuUserType == 1 && stCurMediaInfo.strCuGBVideoTransMode != "UDP")
            || stCurMediaInfo.nPuTransType != 0)
        {
            strError = "unsupport Protocol";
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            //回复状态
            SipUA_AnswerInfo(pMsgPtr, 400, pszBody, iBodyLen);
            VTDU_LOG_E("HandleHiTransMessage unsupport Protocol:" << stCurMediaInfo.nCuTransType << "_" << stCurMediaInfo.nCuUserType << "_" << stCurMediaInfo.strCuGBVideoTransMode << "_" << stCurMediaInfo.nPuTransType);	
			return;
        }

        //非国标的私有流不转码
        if (stCurMediaInfo.nPuProtocol != 1)
        {
            VTDU_LOG_I("非国标的私有流不转码 nPuProtocol: " << stCurMediaInfo.nPuProtocol)
            stCurMediaInfo.bTrans = false;
        }
        
        int nInitialUser = 1;
        bool bTrans = stCurMediaInfo.bTrans;
        //遍历存在的任务，
         //如果有视频源相同且目标分辨率相同的任务(暂时用是否转码，后续按要求转到对应分辨率)，把当前任务添加到已存在任务，添加一个发送。
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%d_%d_%d", stCurMediaInfo.strPUID.c_str(), stCurMediaInfo.nPuChannelID, stCurMediaInfo.nPuStreamType, bTrans);
        std::string strPuInfo = (std::string)szPuInfo;

        bool bExitTask = false;
        Stream *poStream = NULL;
        int nExitTaskRecvPort = 0;
        std::string strExitTaskRecvIp = "";

        {
            std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
            std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strPuInfo);
            if (itorTask != g_mapVtduPreviewTaskInfo.end())
            {
                bExitTask = true;
                poStream = (Stream *)itorTask->second.pStream;
                nExitTaskRecvPort = itorTask->second.nRecvPort;
                strExitTaskRecvIp = itorTask->second.strRecvIp;
                poStream->getOutstreamNum(nInitialUser);
            }
        }

        //查找可使用端口
        int nSendV3Port = -1;

        int nRet = GetSendPort(nSendV3Port, strError);
        if (nRet < 0)
        {
            nStatus = 400;
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            return;
        }

        stCurMediaInfo.nVtduSendPort = nSendV3Port;

        //添加任务
        if (bExitTask)
        {
            int nRet = poStream->addOneSend(stCurMediaInfo.strCuIp, stCurMediaInfo.nCuport, stCurMediaInfo.strCuUserID, stCurMediaInfo.nVtduSendPort);
            if (0 > nRet)
            {
                nStatus = 400;
                strError = "GBRtpPsOverUdpStream addOneSend failed!";
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                std::lock_guard<std::mutex> lock(mtSendV3Port);
                g_vecSendV3Port.push_back(nSendV3Port);
                return;
            }
            g_nOutstreamNum++;

            stCurMediaInfo.nVtduRecvPort = nExitTaskRecvPort;
            stCurMediaInfo.strVtduRecvIp = strExitTaskRecvIp;
            SipUA_RspPreviewOk(pMsgPtr, stCurMediaInfo, nInitialUser);
            return;
        }
        else//创建任务
        {
            //查找可使用端口
            int nRecvPort = -1;
            int nRet = GetRecvPort(nRecvPort, strError);
            if (nRet < 0)
            {
                nStatus = 400;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);

                std::lock_guard<std::mutex> lock(mtSendV3Port);
                g_vecSendV3Port.push_back(nSendV3Port);
                return;
            }

            stCurMediaInfo.nVtduRecvPort = nRecvPort;

            GBRtpPsOverUdpStream* pStreamHanlde = new(std::nothrow) GBRtpPsOverUdpStream(strPuInfo, nRecvPort, m_configSipServer.m_strSipAddr);
            if (NULL == pStreamHanlde)
            {
                nStatus = 400;
                strError = "New GBRtpPsOverUdpStream object failed!";
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());

                RecoverPairPort(nSendV3Port, nRecvPort);
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                return;
            }
            pStreamHanlde->setCallBack(StreamInfoCallBack, this);
            nRet = pStreamHanlde->start(bTrans);
            if (nRet < 0)
            {
                nStatus = 400;
                strError = "GBRtpPsOverUdpStream start failed!";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());

                RecoverPairPort(nSendV3Port, nRecvPort);

                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                return;
            }

            nRet = pStreamHanlde->addOneSend(stCurMediaInfo.strCuIp.c_str(), stCurMediaInfo.nCuport, stCurMediaInfo.strCuUserID, nSendV3Port);
            if (nRet < 0)
            {
                pStreamHanlde->stop();
                nStatus = 400;
                strError = "GBRtpPsOverUdpStream start failed!";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                RecoverPairPort(nSendV3Port, nRecvPort);

                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                return;
            }

            int nHiRecvPort = -1;
            stHiInfo stCurHiInfo;
            stCurHiInfo.nTansTaskNum = 10000;
            stCurHiInfo.strSipId = "";
            //如果 需要转码 从已注册hi 按负载情况获取 hi ip和端口
            if (bTrans)
            {
                //tbd 获取两个 如果第一个掉线了 使用备用的。
                nRet = GetOneHi(stCurHiInfo, strError);
                if (nRet < 0)
                {
                    pStreamHanlde->stop();
                    delete pStreamHanlde;
                    nStatus = 400;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                    RecoverPairPort(nSendV3Port, nRecvPort);
                }

                //tbd sip协议通知对应hi模块 告知接收端口获取hi模块接收端口， 等待 3秒后如果没有回应 重新选择一个hi请求一次)
                int ret = m_oCommunicationHi.sendTransReq(nRecvPort, strPuInfo, stCurHiInfo.strSipIp, stCurHiInfo.nSipPort);
                if (ret <= 0)
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    strError = "send task to hi failed!";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);

                    RecoverPairPort(nSendV3Port, nRecvPort);
                    return;
                }
                //hi接收端口
                for (int nWait = 0; nWait < 60; nWait++)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    {
                        std::lock_guard<std::mutex> lock(mtGetHirecvPort);
                        std::map<std::string, int>::iterator itorRecvHi = g_mapGetHirecvPort.find(strPuInfo);
                        if (itorRecvHi != g_mapGetHirecvPort.end())
                        {
                            nHiRecvPort = itorRecvHi->second;
                            g_mapGetHirecvPort.erase(itorRecvHi);
                            break;
                        }
                    }
                }
                if (nHiRecvPort == -1)
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    VTDU_LOG_E("get hi recv port failed, hi info: " << stCurHiInfo.strSipIp << ":" << stCurHiInfo.nSipPort);
                    strError = "get hi recv port failed";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    //通知取消任务
                    m_oCommunicationHi.sendStopTransReq(strPuInfo, stCurHiInfo.strSipIp, stCurHiInfo.nSipPort);
                    SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);

                    RecoverPairPort(nSendV3Port, nRecvPort);
                    return;
                }

                {
                    std::lock_guard<std::mutex> lock(mtRegHi);
                    g_mapRegHiInfo[stCurHiInfo.strSipId].nTansTaskNum = stCurHiInfo.nTansTaskNum + 1;
                }

                stCurMediaInfo.strVtduRecvIp = stCurHiInfo.strSipIp;
                stCurMediaInfo.nVtduRecvPort = nHiRecvPort;
            }

            stHiTaskInfo stTask;
            stTask.strHiSipId = stCurHiInfo.strSipId;
            stTask.pStream = (void*)pStreamHanlde;
            stTask.enStreamType = Stream_type::UDP_PS;
            stTask.nRecvPort = stCurMediaInfo.nVtduRecvPort;
            stTask.strRecvIp = stCurMediaInfo.strVtduRecvIp;
            stTask.stMedia = stCurMediaInfo;
            stTask.enTaskType = TASK_type::PREVIEW;

            {
                std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
                g_mapVtduPreviewTaskInfo[strPuInfo] = stTask;
            }

            SipUA_RspPreviewOk(pMsgPtr, stCurMediaInfo, nInitialUser);

            g_nInstreamNum++;
            g_nOutstreamNum++;
        }
    }
}

/**************************************************************************
* name          : sipServerHandleV3TransStop
* description   : 处理v3停止预览请求
* input         : pMsgPtr   v3消息
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleV3TransStop(const stuGbEvent &stCurTask)
{
    int nStatus = 400;
    std::string strError = "";
    stMediaInfo stCurMediaInfo;
    stCurMediaInfo.bTrans = false;

    eXosip_event_t evt;
    evt.tid = stCurTask.tid;

    eXosip_event_t* pMsgPtr = &evt;

    nStatus = m_oXmlParse.ParseXmlTransStop(stCurTask.szBody, stCurMediaInfo, strError);

    char pszBody[4096] = { 0 };
    int iBodyLen = 0;
    if (nStatus != 200)
    {
        iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
        SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
        return;
    }
    else
    {
        bool bTrans = stCurMediaInfo.bTrans;
        //遍历存在的任务，
         //如果有视频源相同且目标分辨率相同的任务(暂时用是否转码，后续按要求转到对应分辨率)，删除一个发送，如果是最后一个，删除任务
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%d_%d_%d", stCurMediaInfo.strPUID.c_str(), stCurMediaInfo.nPuChannelID, stCurMediaInfo.nPuStreamType, bTrans);
        std::string strPuInfo = (std::string)szPuInfo;

        bool bExitTask = false;
        Stream *pStream = NULL;
        std::string strHiSipId = "";

        {
            std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
            std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strPuInfo);
            if (itorTask != g_mapVtduPreviewTaskInfo.end())
            {
                bExitTask = true;
                pStream = (Stream *)(itorTask->second.pStream);
                strHiSipId = itorTask->second.strHiSipId;
            }
        }

        if (bExitTask && pStream != NULL)
        {
            //回收发送端口
            int nCurSendPort = -1;
            int nRet = pStream->DelOneSend(stCurMediaInfo.strCuUserID, nCurSendPort);
            if (0 > nRet)
            {
                //printf("no task:%s\n", );
                //没有对应任务 不做任何事情
            }
            else if (0 == nRet) //转发任务结束
            {
                //清理回收资源
                   //关闭端口
                pStream->stop();
                //回收端口
                if (nCurSendPort != -1)
                {
                    RecoverPort(PORT_type::SEND, nCurSendPort);
                }

                int nRecvPort = -1;
                pStream->getRecvPort(nRecvPort);

                RecoverPort(PORT_type::RECEIVE, nRecvPort);
                delete pStream;
                pStream = NULL;

                if (bTrans)
                {
                    //通知hi
                    std::string strHiId = "";
                    std::string strHiIp = "";
                    int nHiPort = 0;

                    {
                        std::lock_guard<std::mutex> lock(mtRegHi);
                        std::map<std::string, stHiInfo>::iterator itorRegHi = g_mapRegHiInfo.find(strHiSipId);
                        if (itorRegHi == g_mapRegHiInfo.end())
                        {
                            nStatus = 400;
                            strError = "get msg body failed";
                            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                            return;
                        }
                        else
                        {
                            strHiId = itorRegHi->second.strSipId;
                            strHiIp = itorRegHi->second.strSipIp;
                            nHiPort = itorRegHi->second.nSipPort;
                        }
                    }

                    //改成socket
                    m_oCommunicationHi.sendStopTransReq(strPuInfo, strHiIp, nHiPort);
                }
                g_nInstreamNum--;
                g_nOutstreamNum--;
            }
            else
            {
                //回收端口
                if (nCurSendPort != -1)
                {
                    RecoverPort(PORT_type::SEND, nCurSendPort);
                }
                g_nOutstreamNum--;
            }
        }
        SipUA_RspPreviewStopOk(pMsgPtr, stCurMediaInfo);
    }
}


/**************************************************************************
* name          : sipServerHandleV3FileStart
* description   : 处理v3回放请求
* input         : pMsgPtr   v3消息
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleV3FileStart(const stuGbEvent &stCurTask)
{
    int nStatus = 400;
    std::string strError = "";
    stMediaInfo stCurMediaInfo;
    stCurMediaInfo.bTrans = false;

    eXosip_event_t evt;
    evt.tid = stCurTask.tid;

    eXosip_event_t* pMsgPtr = &evt;
    nStatus = m_oXmlParse.ParseXmlV3FileStart(stCurTask.szBody, stCurMediaInfo, strError);

    char pszBody[4096] = { 0 };
    int iBodyLen = 0;
    if (nStatus != 200)
    {
        iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
        SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
    }
    else
    {
        //非国标的私有流不转码
        if (stCurMediaInfo.nPuProtocol != 1)
        {
            VTDU_LOG_I("非国标的私有流不转码 nPuProtocol: " << stCurMediaInfo.nPuProtocol);
            stCurMediaInfo.bTrans = false;
        }
        bool bTrans = stCurMediaInfo.bTrans;
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%s_%d", stCurMediaInfo.strCuSession.c_str(), stCurMediaInfo.strCuUserID.c_str(), bTrans);
        std::string strPuInfo = (std::string)szPuInfo;
        //回放不需要重复任务判断
        bool bExitTask = false;
        GBRtpPsOverUdpStream *poStream = NULL;

        //查找可使用端口
        int nRecvPort = -1;
        int nSendV3Port = -1;

        //获取发送端口
        int nRet = GetSendPort(nSendV3Port, strError);
        if (nRet < 0)
        {
            nStatus = 400;
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            return;
        }

        //获取接收端口
        nRet = GetRecvPort(nRecvPort, strError);
        if (nRet < 0)
        {
            nStatus = 400;
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);

            std::lock_guard<std::mutex> lock(mtSendV3Port);
            g_vecSendV3Port.push_back(nSendV3Port);
            return;
        }

        stCurMediaInfo.nVtduRecvPort = nRecvPort;
        stCurMediaInfo.nVtduSendPort = nSendV3Port;

        //启动
        GBRtpPsOverUdpStream* pStreamHanlde = new(std::nothrow) GBRtpPsOverUdpStream(strPuInfo, nRecvPort, m_configSipServer.m_strSipAddr);
        if (NULL == pStreamHanlde)
        {
            nStatus = 400;
            strError = "New GBRtpPsOverUdpStream object failed!";
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());

            RecoverPairPort(nSendV3Port, nRecvPort);
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            return;
        }
        pStreamHanlde->setCallBack(StreamInfoCallBack, this);
        nRet = pStreamHanlde->start(bTrans);
        if (nRet < 0)
        {
            nStatus = 400;
            strError = "GBRtpPsOverUdpStream start failed!";
            delete pStreamHanlde;
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            RecoverPairPort(nSendV3Port, nRecvPort);
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            return;
        }

        nRet = pStreamHanlde->addOneSend(stCurMediaInfo.strCuIp.c_str(), stCurMediaInfo.nCuport, stCurMediaInfo.strCuUserID, nSendV3Port);
        if (nRet < 0)
        {
            pStreamHanlde->stop();
            nStatus = 400;
            strError = "GBRtpPsOverUdpStream start failed!";
            delete pStreamHanlde;
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            RecoverPairPort(nSendV3Port, nRecvPort);
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            return;
        }

        int nHiRecvPort = -1;
        stHiInfo stCurHiInfo;
        stCurHiInfo.nTansTaskNum = 10000;
        stCurHiInfo.strSipId = "";
        //如果 需要转码 从已注册hi 按负载情况获取 hi ip和端口
        if (bTrans)
        {
            nRet = GetOneHi(stCurHiInfo, strError);
            if (nRet < 0)
            {
                pStreamHanlde->stop();
                delete pStreamHanlde;
                nStatus = 400;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                RecoverPairPort(nSendV3Port, nRecvPort);
            }

            //sip协议通知对应hi模块 告知接收端口获取hi模块接收端口， 等待 3秒后如果没有回应 重新选择一个 直接失败 换一个hi重新请求一次)
            int ret = m_oCommunicationHi.sendTransReq(nRecvPort, strPuInfo, stCurHiInfo.strSipIp, stCurHiInfo.nSipPort);
            if (ret <= 0)
            {
                pStreamHanlde->stop();
                nStatus = 400;
                strError = "send task to hi failed!";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                RecoverPairPort(nSendV3Port, nRecvPort);
                return;
            }

            //hi接收端口
            for (int nWait = 0; nWait < 60; nWait++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
                {
                    std::lock_guard<std::mutex> lock(mtGetHirecvPort);
                    std::map<std::string, int>::iterator itorRecvHi = g_mapGetHirecvPort.find(strPuInfo);
                    if (itorRecvHi != g_mapGetHirecvPort.end())
                    {
                        nHiRecvPort = itorRecvHi->second;
                        g_mapGetHirecvPort.erase(itorRecvHi);
                        break;
                    }
                }
            }
            if (nHiRecvPort == -1)
            {
                pStreamHanlde->stop();
                nStatus = 400;
                VTDU_LOG_E("get hi recv port failed, Hi info: " << stCurHiInfo.strSipIp << ":" << stCurHiInfo.nSipPort);
                strError = "get hi recv port failed";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                //通知取消任务
                m_oCommunicationHi.sendStopTransReq(strPuInfo, stCurHiInfo.strSipIp, stCurHiInfo.nSipPort);

                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                RecoverPairPort(nSendV3Port, nRecvPort);
                return;
            }

            {
                std::lock_guard<std::mutex> lock(mtRegHi);
                g_mapRegHiInfo[stCurHiInfo.strSipId].nTansTaskNum = stCurHiInfo.nTansTaskNum + 1;
            }

            stCurMediaInfo.strVtduRecvIp = stCurHiInfo.strSipIp;
            stCurMediaInfo.nVtduRecvPort = nHiRecvPort;
        }

        stHiTaskInfo stTask;
        stTask.strHiSipId = stCurHiInfo.strSipId;
        stTask.pStream = pStreamHanlde;
        stTask.enStreamType = Stream_type::UDP_PS;
        stTask.enTaskType = TASK_type::PLAYBACK;
        stTask.stMedia = stCurMediaInfo;

        {
            std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
            g_mapVtduPreviewTaskInfo[strPuInfo] = stTask;
        }

        SipUA_RspPlayBackOk(pMsgPtr, stCurMediaInfo);

        g_nInstreamNum++;
        g_nOutstreamNum++;
    }
}

/**************************************************************************
* name          : sipServerHandleV3FileStop
* description   : 处理v3停止回放请求
* input         : pMsgPtr   v3消息
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleV3FileStop(const stuGbEvent &stCurTask)
{
    int nStatus = 400;
    std::string strError = "";
    stMediaInfo stCurMediaInfo;
    stCurMediaInfo.bTrans = false;

    eXosip_event_t evt;
    evt.tid = stCurTask.tid;

    eXosip_event_t* pMsgPtr = &evt;
    nStatus = m_oXmlParse.ParseXmlV3FileStop(stCurTask.szBody, stCurMediaInfo, strError);

    char pszBody[4096] = { 0 };
    int iBodyLen = 0;
    if (nStatus != 200)
    {
        iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
    }
    else
    {
        bool bTrans = stCurMediaInfo.bTrans;
        //遍历存在的任务，
         //如果有视频源相同且目标分辨率相同的任务(暂时用是否转码，后续按要求转到对应分辨率)，删除一个发送，如果是最后一个，删除任务
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%s_%d", stCurMediaInfo.strCuSession.c_str(), stCurMediaInfo.strCuUserID.c_str(), bTrans);
        std::string strPuInfo = (std::string)szPuInfo;

        bool bExitTask = false;
        Stream *pStream = NULL;
        std::string strHiSipId = "";

        {
            std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
            std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strPuInfo);
            if (itorTask != g_mapVtduPreviewTaskInfo.end())
            {
                bExitTask = true;
                pStream = (Stream *)(itorTask->second.pStream);
                strHiSipId = itorTask->second.strHiSipId;
            }
        }


        if (bExitTask && pStream != NULL)
        {
            //回收发送端口
            int nCurSendPort = -1;
            int nRet = pStream->DelOneSend(stCurMediaInfo.strCuUserID, nCurSendPort);
            if (0 > nRet)
            {
                //没有对应任务 不做任何事情
            }
            else if (0 == nRet) //转发任务结束
            {
                //清理回收资源
                   //关闭端口
                pStream->stop();
                //回收端口
                if (nCurSendPort != -1)
                {
                    RecoverPort(PORT_type::SEND, nCurSendPort);
                }

                int nRecvPort = -1;
                pStream->getRecvPort(nRecvPort);

                RecoverPort(PORT_type::RECEIVE, nRecvPort);
                delete pStream;
                pStream = NULL;
                if (bTrans)
                {
                    //通知hi
                    std::string strHiId = "";
                    std::string strHiIp = "";
                    int nHiPort = 0;
                    {
                        std::lock_guard<std::mutex> lock(mtRegHi);
                        std::map<std::string, stHiInfo>::iterator itorRegHi = g_mapRegHiInfo.find(strHiSipId);
                        if (itorRegHi == g_mapRegHiInfo.end())
                        {
                            VTDU_LOG_E("sipServerHandleV3FileStop,cannot find hi info, HiSipId:: " << strHiSipId);
                        }
                        else
                        {
                            strHiId = itorRegHi->second.strSipId;
                            strHiIp = itorRegHi->second.strSipIp;
                            nHiPort = itorRegHi->second.nSipPort;
                        }
                    }

                    if (strHiId != "" && strHiIp != "" && nHiPort != 0)
                    {
                        m_oCommunicationHi.sendStopTransReq(strPuInfo, strHiIp, nHiPort);
                    }
                }
                g_nInstreamNum--;
                g_nOutstreamNum--;
            }
            else
            {
                if (nCurSendPort != -1)
                {
                    RecoverPort(PORT_type::SEND, nCurSendPort);
                }
                g_nOutstreamNum--;
            }
        }

        SipUA_RspPlayBackStopOk(pMsgPtr, stCurMediaInfo);
    }
}

/**************************************************************************
* name          : RecoverPairPort
* description   : 回收一对收发端口
* input         : nPortSend  发送端口
                : nPortRecv 接收
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::RecoverPairPort(int nPortSend, int nPortRecv)
{

    {
        std::lock_guard<std::mutex> lock(mtSendV3Port);
        g_vecSendV3Port.push_back(nPortSend);
    }
    
    std::lock_guard<std::mutex> lock(mtRecvPort);
    g_vecRecvPort.push_back(nPortRecv);
}

/**************************************************************************
* name          : RecoverPort
* description   : 回收端口
* input         : enPortType  端口类型
                : nPort 端口
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::RecoverPort(PORT_type enPortType, int nPort)
{
    if (enPortType == PORT_type::SEND)
    {
        std::lock_guard<std::mutex> lock(mtSendV3Port);
        g_vecSendV3Port.push_back(nPort);
    }
    else
    {
        std::lock_guard<std::mutex> lock(mtRecvPort);
        g_vecRecvPort.push_back(nPort);
    }

}

/**************************************************************************
* name          : GetOneHi
* description   : 获取一个已注册的hi模块
* input         : NA
* output        : stCurHiInfo hi信息
* return        : 0正常 小于0 异常
* remark        : NA
**************************************************************************/
int VtduServer::GetOneHi(stHiInfo &stCurHiInfo, std::string &strError)
{
    std::lock_guard<std::mutex> lock(mtRegHi);
    if (0 == g_mapRegHiInfo.size())
    {
        VTDU_LOG_E("have no trans module reg");
        strError = "have no trans module reg";
        return -1;
    }
    std::map<std::string, stHiInfo>::iterator itorRegHi = g_mapRegHiInfo.begin();
    for (; itorRegHi != g_mapRegHiInfo.end(); ++itorRegHi)
    {
        if (itorRegHi->second.nTansTaskNum == 0)
        {
            stCurHiInfo.nTansTaskNum = itorRegHi->second.nTansTaskNum;
            stCurHiInfo.strSipIp = itorRegHi->second.strSipIp;
            stCurHiInfo.nSipPort = itorRegHi->second.nSipPort;
            stCurHiInfo.strSipId = itorRegHi->second.strSipId;
            stCurHiInfo.strSipRegion = itorRegHi->second.strSipRegion;
            break;
        }
        if (itorRegHi->second.nTansTaskNum < stCurHiInfo.nTansTaskNum && itorRegHi->second.nTansTaskNum < itorRegHi->second.nMaxTransTaskNum)
        {
            stCurHiInfo.nTansTaskNum = itorRegHi->second.nTansTaskNum;
            stCurHiInfo.strSipIp = itorRegHi->second.strSipIp;
            stCurHiInfo.nSipPort = itorRegHi->second.nSipPort;
            stCurHiInfo.strSipId = itorRegHi->second.strSipId;
            stCurHiInfo.strSipRegion = itorRegHi->second.strSipRegion;
        }
    }
    if (stCurHiInfo.strSipId == "")
    {
        VTDU_LOG_E("get Hi failed");
        strError = "get Hi failed!";
        return -1;
    }
    return 0;
}

/**************************************************************************
* name          : GetSendPort
* description   : 获取一个vtdu发流端口
* input         : NA
* output        : nSendV3Port 发流端口
                  strError 错误信息
* return        : 0正常 小于0 异常
* remark        : NA
**************************************************************************/
int VtduServer::GetSendPort(int &nSendV3Port, std::string &strError)
{
    std::lock_guard<std::mutex> lock(mtSendV3Port);
    if (g_vecSendV3Port.size() == 0)
    {
        VTDU_LOG_E("g_vecSendV3Port is NULL");
        strError = "SendV3Port is use up";
        return -1;
    }

    std::vector<int>::iterator itorSendV3 = g_vecSendV3Port.begin();
    nSendV3Port = *itorSendV3;
    g_vecSendV3Port.erase(itorSendV3);

    return 0;
}

/**************************************************************************
* name          : GetRecvPort
* description   : 获取一个vtdu收流端口
* input         : NA
* output        : nRecvV3Port 收流端口
                  strError 错误信息
* return        : 0正常 小于0 异常
* remark        : NA
**************************************************************************/
int VtduServer::GetRecvPort(int &nRecvV3Port, std::string &strError)
{
    std::lock_guard<std::mutex> lock(mtRecvPort);
    if (g_vecRecvPort.size() == 0)
    {
        VTDU_LOG_E("g_vecRecvHiPort is NULL");
        strError = "RecvHiPort is use up";
        return -1;
    }
    std::vector<int>::iterator itorRecvHi = g_vecRecvPort.begin();
    nRecvV3Port = *itorRecvHi;
    g_vecRecvPort.erase(itorRecvHi);
    return 0;
}

/**************************************************************************
* name          : HandleHiRegister
* description   : 处理hi注册请求
* input         : pMsgPtr   hi注册消息
* output        : szSendBuff 需要回复hi的消息
*                 nSendBufLen 需要回复hi的消息长度
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::HandleHiRegister(char *pMsgPtr, char* szSendBuff, int* nSendBufLen)
{
    *nSendBufLen = 0;
    std::string strError = "";
    if (pMsgPtr)
    {
        stHiInfo stCurHi;
        int nRet = m_oXmlParse.ParseXmlHiReg(pMsgPtr, stCurHi, strError);
        if (0 > nRet)
        {
            *nSendBufLen = sprintf(szSendBuff,
                "<VTDU_HEADER>\r\n"
                "<MessageType>MSG_HI_REG_RSP</MessageType>\r\n"
                "</VTDU_HEADER>\r\n"
                "<HI_Rsp_Info>\r\n"
                "<HI_ErrorCode>-1</HI_ErrorCode>\r\n"
                "<HI_ErrorInfo>%s</HI_ErrorInfo>\r\n"
                "</HI_Rsp_Info>\r\n", strError.c_str());
            return;
        }

        stCurHi.nHeartBeat = _time64(NULL);
        stCurHi.nTansTaskNum = 0;

        {
            std::lock_guard<std::mutex> lock(mtRegHi);
            g_mapRegHiInfo[stCurHi.strSipId] = stCurHi;
        }

        *nSendBufLen = sprintf(szSendBuff,
            "<VTDU_HEADER>\r\n"
            "<MessageType>MSG_HI_REG_RSP</MessageType>\r\n"
            "</VTDU_HEADER>\r\n"
            "<HI_Rsp_Info>\r\n"
            "<HI_ErrorCode>0</HI_ErrorCode>\r\n"
            "<HI_ErrorInfo></HI_ErrorInfo>\r\n"
            "</HI_Rsp_Info>\r\n");
    }
    else
    {
        VTDU_LOG_E("HandleHiRegister body is null.");
        strError = "cannot find HandleHiRegister body";
        *nSendBufLen = sprintf(szSendBuff,
            "<VTDU_HEADER>\r\n"
            "<MessageType>MSG_HI_REG_RSP</MessageType>\r\n"
            "</VTDU_HEADER>\r\n"
            "<HI_Rsp_Info>\r\n"
            "<HI_ErrorCode>-1</HI_ErrorCode>\r\n"
            "<HI_ErrorInfo>%s</HI_ErrorInfo>\r\n"
            "</HI_Rsp_Info>\r\n", strError.c_str());
        return;
    }

    return;
}

/**************************************************************************
* name          : HandleHiHeartBeat
* description   : 处理hi心跳消息
* input         : pMsgPtr   hi心跳消息
* output        : szSendBuff 需要回复hi的消息
*                 nSendBufLen 需要回复hi的消息长度
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::HandleHiHeartBeat(char *pMsgPtr, char* szSendBuff, int* nSendBufLen)
{
    *nSendBufLen = 0;
    stHiInfo stCurHi;
    if (pMsgPtr)
    {
        bool bFindSipInfo = false;   //是否有sip信息
        bool bNeedReconnect = false; //是否通知重连
        int nRet = m_oXmlParse.ParseXmlHiHeartBeat(pMsgPtr, stCurHi, bFindSipInfo);

        __time64_t nCurTime = _time64(NULL);
        {
            std::lock_guard<std::mutex> lock(mtRegHi);
            std::map<std::string, stHiInfo>::iterator iotr = g_mapRegHiInfo.find(stCurHi.strSipId);
            if (iotr != g_mapRegHiInfo.end())
            {
                iotr->second.nHeartBeat = nCurTime;
            }
            else//如果没注册就有心跳，可能是vtdu掉线后重新上线收到的hi模块心跳，直接放入注册列表 清空负载情况 通知hi 销毁资源 
            {
                if (bFindSipInfo)
                {
                    bNeedReconnect = true;
                    stCurHi.nHeartBeat = nCurTime;
                    stCurHi.nTansTaskNum = 0;
                    g_mapRegHiInfo[stCurHi.strSipId] = stCurHi;
                }
            }
        }

        if (bNeedReconnect)
        {
            m_oCommunicationHi.sendReconnect(stCurHi.strSipIp, stCurHi.nSipPort);
        }
        
        return;
    }
}

/**************************************************************************
* name          : HandleHiTansRsp
* description   : 处理hi转码回复消息
* input         : pMsgPtr   hi回复消息
* output        : szSendBuff 需要回复hi的消息
*                 nSendBufLen 需要回复hi的消息长度
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::HandleHiTansRsp(char *pMsgPtr, char* szSendBuff, int* nSendBufLen)
{
    *nSendBufLen = 0;
    if (pMsgPtr)
    {
        std::string strHiTaskId = "";
        int nHiRecvPort = -1;
        int nRet = m_oXmlParse.ParseXmlHiTansRsp(pMsgPtr, strHiTaskId, nHiRecvPort);
        if (nRet < 0)
        {
            return;
        }
        std::lock_guard<std::mutex> lock(mtGetHirecvPort);
        g_mapGetHirecvPort[strHiTaskId] = nHiRecvPort;

        return;
    }
}

/**************************************************************************
* name          : HandleHiCutout
* description   : 处理hi断流消息
* input         : pMsgPtr   hi回复消息
* output        : szSendBuff 需要回复hi的消息
*                 nSendBufLen 需要回复hi的消息长度
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::HandleHiCutout(char *pMsgPtr, char* szSendBuff, int* nSendBufLen)
{
    *nSendBufLen = 0;
    if (pMsgPtr)
    {
        std::string strHiTaskId = "";
        int nRet = m_oXmlParse.ParseXmlHiCutout(pMsgPtr, strHiTaskId);
        if (nRet < 0)
        {
            return;
        }

        //按照task id寻找任务信息，释放stream，回收端口
        Stream* pStream = NULL;
        std::string strHiSipId = "";
        stMediaInfo stCurMediaInfo;
        TASK_type enTaskType = TASK_type::PLAYBACK;

        {
            std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
            std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strHiTaskId);
            if (itorTask != g_mapVtduPreviewTaskInfo.end())
            {
                pStream = (Stream*)(itorTask->second.pStream);
                stCurMediaInfo = itorTask->second.stMedia;
                enTaskType = itorTask->second.enTaskType;
                g_mapVtduPreviewTaskInfo.erase(itorTask);
            }
        }

        if (NULL != pStream)
        {
            pStream->stop();
            //回收所有发送端口
            std::vector<int> vecSendPort;
            pStream->getSendPort(vecSendPort);

            {
                std::lock_guard<std::mutex> lock(mtSendV3Port);
                for (std::vector<int>::iterator itorvecSendPort = vecSendPort.begin(); itorvecSendPort != vecSendPort.end(); ++itorvecSendPort)
                {
                    g_vecSendV3Port.push_back(*itorvecSendPort);
                }
            }

            //回收接收端口
            int nRecvPort = -1;
            pStream->getRecvPort(nRecvPort);
            if (-1 != nRecvPort)
            {
                RecoverPort(PORT_type::RECEIVE, nRecvPort);
            }

            //更新输入输出流路数
            int nOutNum = 0;
            pStream->getOutstreamNum(nOutNum);
            g_nOutstreamNum -= nOutNum;
            g_nInstreamNum--;
            delete pStream;
            pStream = NULL;

            // 通知v3断流 //回放待确认
            if (enTaskType == TASK_type::PREVIEW)
            {
                SipUA_Timeout(stCurMediaInfo, m_configSipServer);
            }
        }

        return;
    }
}

/**************************************************************************
* name          : sipServerHandleV3TransReadyTest
* description   : 处理v3预览请求 测试函数
* input         : pMsgPtr   v3消息
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleV3TransReadyTest(stMediaInfo &stCurMediaInfo)
{
    int nStatus = 200;
    std::string strError = "";
   // stCurMediaInfo.bTrans = false;

    stCurMediaInfo.strVtduRecvIp = m_configSipServer.m_strSipAddr;
    stCurMediaInfo.strVtduSendIP = m_configSipServer.m_strSipAddr;

    char pszBody[4096] = { 0 };
    int iBodyLen = 0;
    if (nStatus != 200)
    {
        return;
    }
    else
    {
        int nInitialUser = 1;
        bool bTrans = stCurMediaInfo.bTrans;
        //遍历存在的任务，
         //如果有视频源相同且目标分辨率相同的任务(暂时用是否转码，后续按要求转到对应分辨率)，把当前任务添加到已存在任务，添加一个发送。
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%d_%d_%d", stCurMediaInfo.strPUID.c_str(), stCurMediaInfo.nPuChannelID, stCurMediaInfo.nPuStreamType, bTrans);
        std::string strPuInfo = (std::string)szPuInfo;

        bool bExitTask = false;
        Stream *poStream = NULL;
        int nExitTaskRecvPort = 0;
        std::string strExitTaskRecvIp = "";

        {
            std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
            std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strPuInfo);
            if (itorTask != g_mapVtduPreviewTaskInfo.end())
            {
                bExitTask = true;
                poStream = (Stream *)itorTask->second.pStream;
                nExitTaskRecvPort = itorTask->second.nRecvPort;
                strExitTaskRecvIp = itorTask->second.strRecvIp;
                poStream->getOutstreamNum(nInitialUser);
            }
        }

        //查找可使用端口
        int nSendV3Port = -1;

        //获取发送端口
        int nRet = GetSendPort(nSendV3Port, strError);
        if (nRet < 0)
        {
            return;
        }

        stCurMediaInfo.nVtduSendPort = nSendV3Port;

        //添加任务
        if (bExitTask)
        {
            int nRet = poStream->addOneSend(stCurMediaInfo.strCuIp, stCurMediaInfo.nCuport, stCurMediaInfo.strCuUserID, stCurMediaInfo.nVtduSendPort);
            if (0 > nRet)
            {
                nStatus = 400;
                RecoverPort(PORT_type::SEND, nSendV3Port);
                return;
            }
            g_nOutstreamNum++;

            stCurMediaInfo.nVtduRecvPort = nExitTaskRecvPort;
            stCurMediaInfo.strVtduRecvIp = strExitTaskRecvIp;
            return;
        }
        else//创建任务
        {
            //查找可使用端口
            int nRecvPort = -1;

            //获取接收端口
            int nRet = GetRecvPort(nRecvPort, strError);
            if (nRet < 0)
            {
                std::lock_guard<std::mutex> lock(mtSendV3Port);
                g_vecSendV3Port.push_back(nSendV3Port);
                return;
            }

            stCurMediaInfo.nVtduRecvPort = nRecvPort;

            GBRtpPsOverUdpStream* pStreamHanlde = new(std::nothrow) GBRtpPsOverUdpStream(strPuInfo, nRecvPort, m_configSipServer.m_strSipAddr);
            if (NULL == pStreamHanlde)
            {
                nStatus = 400;
                strError = "New GBRtpPsOverUdpStream object failed!";
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());

                RecoverPairPort(nSendV3Port, nRecvPort);
                return;
            }
            pStreamHanlde->setCallBack(StreamInfoCallBack, this);
            nRet = pStreamHanlde->start(bTrans);
            if (nRet < 0)
            {
                nStatus = 400;
                strError = "GBRtpPsOverUdpStream start failed!";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                RecoverPairPort(nSendV3Port, nRecvPort);
                return;
            }

            nRet = pStreamHanlde->addOneSend(stCurMediaInfo.strCuIp.c_str(), stCurMediaInfo.nCuport, stCurMediaInfo.strCuUserID, nSendV3Port);
            if (nRet < 0)
            {
                pStreamHanlde->stop();
                nStatus = 400;
                strError = "GBRtpPsOverUdpStream start failed!";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                RecoverPairPort(nSendV3Port, nRecvPort);
                return;
            }

            int nHiRecvPort = -1;
            stHiInfo stCurHiInfo;
            stCurHiInfo.nTansTaskNum = 10000;
            stCurHiInfo.strSipId = "";
            //如果 需要转码 从已注册hi 按负载情况获取 hi ip和端口
            if (bTrans)
            {
                nRet = GetOneHi(stCurHiInfo, strError);
                if (nRet < 0)
                {
                    pStreamHanlde->stop();
                    delete pStreamHanlde;
                    RecoverPairPort(nSendV3Port, nRecvPort);
                }

                //sip协议通知对应hi模块 告知接收端口获取hi模块接收端口， 等待 10秒后如果没有回应 重新选择一个 直接失败 换一个hi重新请求一次)
                int ret = m_oCommunicationHi.sendTransReq(nRecvPort, strPuInfo, stCurHiInfo.strSipIp, stCurHiInfo.nSipPort);
                if (ret <= 0)
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    strError = "send task to hi failed!";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    RecoverPairPort(nSendV3Port, nRecvPort);
                    return;
                }
                //hi接收端口
                for (int nWait = 0; nWait < 60; nWait++)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(50));
                    {
                        std::lock_guard<std::mutex> lock(mtGetHirecvPort);
                        std::map<std::string, int>::iterator itorRecvHi = g_mapGetHirecvPort.find(strPuInfo);
                        if (itorRecvHi != g_mapGetHirecvPort.end())
                        {
                            nHiRecvPort = itorRecvHi->second;
                            g_mapGetHirecvPort.erase(itorRecvHi);
                            break;
                        }
                    }
                }
                if (nHiRecvPort == -1)
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    VTDU_LOG_E("get hi recv port failed, hi info: " << stCurHiInfo.strSipIp << ":" << stCurHiInfo.nSipPort);
                    strError = "get hi recv port failed";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    //通知取消任务
                    m_oCommunicationHi.sendStopTransReq(strPuInfo, stCurHiInfo.strSipIp, stCurHiInfo.nSipPort);
                    RecoverPairPort(nSendV3Port, nRecvPort);
                    return;
                }

                {
                    std::lock_guard<std::mutex> lock(mtRegHi);
                    g_mapRegHiInfo[stCurHiInfo.strSipId].nTansTaskNum = stCurHiInfo.nTansTaskNum + 1;
                }
                stCurMediaInfo.strVtduRecvIp = stCurHiInfo.strSipIp;
                stCurMediaInfo.nVtduRecvPort = nHiRecvPort;
            }

            stHiTaskInfo stTask;
            stTask.strHiSipId = stCurHiInfo.strSipId;
            stTask.pStream = (void*)pStreamHanlde;
            stTask.enStreamType = Stream_type::UDP_PS;
            stTask.nRecvPort = stCurMediaInfo.nVtduRecvPort;
            stTask.strRecvIp = stCurMediaInfo.strVtduRecvIp;
            stTask.stMedia = stCurMediaInfo;
            stTask.enTaskType = TASK_type::PREVIEW;

            {
                std::lock_guard<std::mutex> lock(mtVtduPreviewTask);
                g_mapVtduPreviewTaskInfo[strPuInfo] = stTask;
            }

            g_nInstreamNum++;
            g_nOutstreamNum++;
        }
    }
}
