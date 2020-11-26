/**   @file    VtduServer.cpp
*    @note    All Right Reserved.
*    @brief   vtdu����������ģ��
*    @author   ������
*    @date     2020/10/29
*
*    @note    �����note��warningΪ��ѡ��Ŀ
*    @note
*    @note    ��ʷ��¼��
*    @note    V0.0.1  ����
*
*    @warning ������д���ļ���صľ�����Ϣ
*/
#include "VtduServer.h"
#include "ConfigFileParse.h"

//sip��Ϣ������
void GB28181EventCB(ESipUAMsg msg, void *pMsgPtr, void *pParam)
{
    if (NULL == pMsgPtr || NULL == pParam)
    {
        VTDU_LOG_E("GB28181EventCB, invaild para");
        return;
    }
    VtduServer* poServer = (VtduServer*)pParam;
    switch (msg)
    {
        /************************ע������Ϣ*****************************/
    case SIPUA_MSG_REGISTER_SUCCESS: /*ע��ɹ�*/
        poServer->sipClientHandleRegisterSuccess(pMsgPtr);
        break;

    case SIPUA_MSG_REGISTER_FAILURE: /*ע��ʧ��*/
        poServer->sipClientHandleRegisterFailed(pMsgPtr);
        break;

    case SIPUA_MSG_INFO: //INFO ��Ϣ
        poServer->sipServerHandleInfo(pMsgPtr);
        break;
    case SIPUA_MSG_OPTION_REQUESTFAILURE:
        //������Ϣ����ʧ�ܣ��ж�λsip��������ߣ�����ע���߳�����ע�� ֹͣ�����߳�
        poServer->sipClientHandleOptionFailed();
    default:
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

//��ý��ģ���ϱ���Ϣ����
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
* description   : ��ʼ��
* input         : strCfgFile  �����ļ�·��
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int VtduServer::Init(const std::string &strCfgFile)
{
    //��ȡ������Ϣ
    ConfigFileParse oCfg;
    int nRet = oCfg.getCfg(strCfgFile, m_configSipServer);
    if (nRet < 0)
    {
        return -1;
    }

    //�����˿�
    mtRecvPort.lock();
    for (int i = m_configSipServer.m_nRecvV3PortBegin; i <= m_configSipServer.m_nRecvV3PortEnd;)
    {
        g_vecRecvPort.push_back(i);
        i += 2;
    }
    mtRecvPort.unlock();

    //�����˿�
    mtSendV3Port.lock();
    for (int i = m_configSipServer.m_nSendV3PortBegin; i <= m_configSipServer.m_nSendV3PortEnd;)
    {
        g_vecSendV3Port.push_back(i);
        i += 2;
    }
    mtSendV3Port.unlock();

    //sip��ʼ��
    int ret = SipUA_Init(m_configSipServer);
    if (0 > ret)
    {
        return -1;
    }

    //hiģ��ͨ�����ʼ��
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
* description   : ����ʼ��
* input         : NA
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
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
* description   : ����
* input         : NA
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int VtduServer::Start()
{
    //����v3 sip����
    int ret = SipUA_StartEventLoop(GB28181EventCB, this);
    if (0 != ret)
    {
        return -1;
    }

    //����hiģ�齻��
    m_oCommunicationHi.StartEventLoop(HiEventCB, this);
    if (0 != ret)
    {
        SipUA_StopEventLoop();
        return -1;
    }

    //ע�ᵽv3
    g_nRegid = SipUA_RegisterV3(m_configSipServer, false, g_nRegid);

    //����ת������������̣߳���������60��δ�յ� �ж����ߡ�ֹͣ��Ӧ�����̣߳�ע��ת�������
    static std::thread ThHiManager(&VtduServer::threadHiManager, this);

    //ע��v3�߳�
    static std::thread ThRegLoop(&VtduServer::threadRegLoop, this);

    //v3�����߳�
    static std::thread ThHeartBeat(&VtduServer::threadHeartBeat, this);

    //��������ص���Ϣ�߳�
    static std::thread ThCBWorkingThread(&VtduServer::threadStreamCBMsgLoop, this);

    return 0;
}

/**************************************************************************
* name          : Stop
* description   : ֹͣ
* input         : NA
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int VtduServer::Stop()
{
    //��ʱ��ʵ�֣�����Ϊ����ֹͣ
    return 0;
}

/**************************************************************************
* name          : threadHiManager
* description   : ת��ģ�鱣������߳�
* input         : pParam �û�����
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
void VtduServer::threadHiManager()
{
    do
    {
        long long nCurTime = Comm_GetSecFrom1970();
        mtRegHi.lock();
        std::map<std::string, stHiInfo>::iterator iotr = g_mapRegHiInfo.begin();
        for (; iotr != g_mapRegHiInfo.end(); )
        {
            VTDU_LOG_I("CHECK HiTrans, id_ip: ", iotr->second.strSipId << "_" << iotr->second.strSipIp<< ", task num: " << iotr->second.nTansTaskNum);
            //15��û������ �ж�����
            if (nCurTime - iotr->second.nHeartBeat > 15)
            {
                VTDU_LOG_I("CHECK HiTrans, Hitans cut off: ", iotr->second.strSipId << "_" << iotr->second.strSipIp);
                //ֹͣ���� ���ո�ģ�����ж˿�
                mtVtduPreviewTask.lock();
                std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.begin();
                for (; itorTask != g_mapVtduPreviewTaskInfo.end();)
                {
                    if (itorTask->second.strHiSipId != "" && itorTask->second.strHiSipId == iotr->first)
                    {
                        Stream *pStream = (Stream *)(itorTask->second.pStream);
                        pStream->stop();
                        //�������з��Ͷ˿�
                        std::vector<int> vecSendPort;
                        pStream->getSendPort(vecSendPort);
                        mtSendV3Port.lock();
                        for (std::vector<int>::iterator itorvecSendPort = vecSendPort.begin(); itorvecSendPort != vecSendPort.end(); ++itorvecSendPort)
                        {
                            g_vecSendV3Port.push_back(*itorvecSendPort);
                        }
                        mtSendV3Port.unlock();

                        int nRecvPort = -1;
                        pStream->getRecvPort(nRecvPort);
                        int nOutstreamNum;
                        pStream->getOutstreamNum(nOutstreamNum);
                        //���ս��ն˿�
                        if (-1 != nRecvPort)
                        {
                            mtRecvPort.lock();
                            g_vecRecvPort.push_back(nRecvPort);
                            mtRecvPort.unlock();
                        }

                        g_nOutstreamNum -= nOutstreamNum;
                        g_nInstreamNum--;
                        delete pStream;
                        pStream = NULL;
                        //֪ͨv3����
                        if (itorTask->second.enTaskType == TASK_type::PREVIEW)
                        {
                            stMediaInfo stCurMediaInfo = itorTask->second.stMedia;
                            SipUA_Timeout(stCurMediaInfo, m_configSipServer);
                        }
                        //����������Ϣ
                        itorTask = g_mapVtduPreviewTaskInfo.erase(itorTask);
                    }
                    else
                    {
                        ++itorTask;
                    }
                }
                mtVtduPreviewTask.unlock();
                iotr = g_mapRegHiInfo.erase(iotr);
            }
            else
            {
                ++iotr;
            }
        }
        mtRegHi.unlock();

        std::this_thread::sleep_for(std::chrono::seconds(5));

    } while (!g_bStopHiManager);

    return ;
}

/**************************************************************************
* name          : threadRegLoop
* description   : ע��v3 sip�߳�
* input         : pParam �û�����
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
void VtduServer::threadRegLoop()
{
    //600��ע��һ��
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
* description   : v3 sip�����߳�
* input         : pParam �û�����
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
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

            //����ע��
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
* description   : streamģ���ϱ���Ϣ�����߳�
* input         : pParam �û�����
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
void VtduServer::threadStreamCBMsgLoop()
{
    while (1)
    {
        bool bGet = false;
        stCutOffInfo stCurCutOffInfo;
        mtCutOff.lock();
        if (g_vecCutOff.size() > 0)
        {
            stCurCutOffInfo = g_vecCutOff.back();
            g_vecCutOff.pop_back();
            bGet = true;
        }
        mtCutOff.unlock();

        if (bGet)
        {
            VTDU_LOG_I("threadStreamCBMsgLoop get cutoff, strHiTaskId: " << stCurCutOffInfo.strCutOffInfo);
            std::string strHiTaskId = stCurCutOffInfo.strCutOffInfo;
            //����task idѰ��������Ϣ���ͷ�stream�����ն˿�
            Stream* pStream = NULL;
            std::string strHiSipId = "";
            stMediaInfo stCurMediaInfo;
            TASK_type enTaskType = TASK_type::PLAYBACK;
            mtVtduPreviewTask.lock();
            std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strHiTaskId);
            if (itorTask != g_mapVtduPreviewTaskInfo.end())
            {
                //�����Ƿ������ô�� ��Ҫ����
                pStream = (Stream*)(itorTask->second.pStream);
                stCurMediaInfo = itorTask->second.stMedia;
                enTaskType = itorTask->second.enTaskType;
                strHiSipId = itorTask->second.strHiSipId;
                g_mapVtduPreviewTaskInfo.erase(itorTask);
            }
            mtVtduPreviewTask.unlock();
            if (NULL != pStream)
            {
                pStream->stop();
                //�������з��Ͷ˿�
                std::vector<int> vecSendPort;
                pStream->getSendPort(vecSendPort);
                mtSendV3Port.lock();
                for (std::vector<int>::iterator itorvecSendPort = vecSendPort.begin(); itorvecSendPort != vecSendPort.end(); ++itorvecSendPort)
                {
                    g_vecSendV3Port.push_back(*itorvecSendPort);
                }
                mtSendV3Port.unlock();

                //���ս��ն˿�
                int nRecvPort = -1;
                pStream->getRecvPort(nRecvPort);
                if (-1 != nRecvPort)
                {
                    mtRecvPort.lock();
                    g_vecRecvPort.push_back(nRecvPort);
                    mtRecvPort.unlock();
                }

                //�������������·��
                int nOutNum = 0;
                pStream->getOutstreamNum(nOutNum);
                g_nOutstreamNum -= nOutNum;
                g_nInstreamNum--;
                delete pStream;
                pStream = NULL;

                // ֪ͨv3����
                if (enTaskType == TASK_type::PREVIEW)
                {
                    SipUA_Timeout(stCurMediaInfo, m_configSipServer);
                }
            }

            //֪ͨhi����
            if (stCurCutOffInfo.enCutOffType == CUTOFF_type::HiTrans)
            {
                //֪ͨhi
                std::string strHiId = "";
                std::string strHiIp = "";
                int nHiPort = 0;
                mtRegHi.lock();
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
                mtRegHi.unlock();
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
* description   : ����ע��ɹ���Ϣ
* input         : pMsgPtr sip��Ϣ
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
* description   : ����ע��ʧ��
* input         : pMsgPtr sip��Ϣ
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
* name          : sipServerHandleInfo
* description   : ����info��Ϣ
* input         : pMsgPtr sip��Ϣ
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleInfo(void *pMsgPtr)
{
    VTDU_LOG_I("recv Sip [INFO] Message");

    char szBody[4096] = { 0 };
    int ret = SipUA_GetRequestBodyContent(pMsgPtr, szBody, 4096);
    if (ret <= 0)
    {
        return;
    }

    std::string strCmdType = m_oXmlParse.getMsgCmdTypeV3(szBody, ret);
    if ("MSG_READY_VIDEO_TRANS_REQ" == strCmdType)//����Ԥ��
    {
        sipServerHandleV3TransReady(pMsgPtr);
    }
    else if ("MSG_START_VIDEO_VTDU_ACK" == strCmdType)//ȷ��Ԥ��
    {
        VTDU_LOG_I("recv MSG_START_VIDEO_VTDU_ACK");
        //sipServerHandleV3TransAck(pMsgPtr);
    }
    else if ("MSG_VTDU_STOP_VIDEO_REQ" == strCmdType)//ֹͣԤ��
    {
        //ֹͣ���շ�������������Դ�����ն˿�
        sipServerHandleV3TransStop(pMsgPtr);
    }
    else if ("MSG_START_FILE_VOD_TASK_REQ" == strCmdType)//�����ط�
    {
        sipServerHandleV3FileStart(pMsgPtr);
    }
    else if ("MSG_STOP_FILE_VOD_TASK_REQ" == strCmdType)//ֹͣ�ط�
    {
        sipServerHandleV3FileStop(pMsgPtr);
    }
    else
    {
        VTDU_LOG_E("sipServerHandleInfo unsupport Request: " << strCmdType);
        //�ظ�״̬
        SipUA_AnswerInfo(pMsgPtr, 400, NULL, 0);
    }
}

/**************************************************************************
* name          : sipClientHandleOptionFailed
* description   : option��Ϣ����ʧ�ܣ�option��Ϣ��Ϊ��������ʧ���ж�Ϊ����˵���
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
* description   : Hiת���Ͽ��ϱ���Ϣ����
* input         : pMsgPtr    �յ�����Ϣ
*                 nLen       �յ�����Ϣ����
* output        : szSendBuff �ظ���Ϣ
*                 nSendBuf   �ظ���Ϣ����
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
    if ("MSG_HI_REG" == strCmdType)//ע��
    {
        VTDU_LOG_I("HandleHiTransMessage handle MSG_HI_REG,body: ", pMsgPtr);
        HandleHiRegister(pMsgPtr, szSendBuff, nSendBuf);
    }
    else if ("MSG_HI_HEART" == strCmdType)//����
    {
        HandleHiHeartBeat(pMsgPtr, szSendBuff, nSendBuf);
    }
    else if ("MSG_HI_RECVPORT_RSP" == strCmdType) //ת������ظ�
    {
        HandleHiTansRsp(pMsgPtr, szSendBuff, nSendBuf);
    }
    else if ("MSG_HI_CUTOUT" == strCmdType || "MSG_HI_SELECT_FAILED" == strCmdType) //����
    {
        VTDU_LOG_I("HandleHiTransMessage handle MSG_HI_CUTOUT,body: ", pMsgPtr);
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
* description   : ������ģ���ϱ���Ϣ
* input         : nType     ��Ϣ���� 0��v3���� 1hiģ�����
*                 strCbInfo ��Ϣ����
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::HandleStreamInfoCallBack(int nType, std::string strCbInfo)
{
    stCutOffInfo stCurCutOffInfo;
    stCurCutOffInfo.strCutOffInfo = strCbInfo;
    switch (nType)
    {
    case 0: //v3����
        stCurCutOffInfo.enCutOffType = CUTOFF_type::V3;
        mtCutOff.lock();
        g_vecCutOff.push_back(stCurCutOffInfo);
        mtCutOff.unlock();
        VTDU_LOG_I("HandleStreamInfoCallBack v3 cut off, task: " << stCurCutOffInfo.strCutOffInfo);
        break;
    case 1: // hiģ�����
        stCurCutOffInfo.enCutOffType = CUTOFF_type::HiTrans;
        mtCutOff.lock();
        g_vecCutOff.push_back(stCurCutOffInfo);
        mtCutOff.unlock();
        VTDU_LOG_I("HandleStreamInfoCallBack Hi cut off, task: " << stCurCutOffInfo.strCutOffInfo);
        break;
    default:
        VTDU_LOG_E("StreamInfoCallBack unsuport type:" << nType);
        break;
    }
}

/**************************************************************************
* name          : sipServerHandleV3TransReady
* description   : ����v3Ԥ������
* input         : pMsgPtr   v3��Ϣ
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleV3TransReady(void *pMsgPtr)
{
    int nStatus = 400;
    std::string strError = "";
    stMediaInfo stCurMediaInfo;
    stCurMediaInfo.bTrans = false;
    do
    {
        char szBody[4096] = { 0 };
        int ret = SipUA_GetRequestBodyContent(pMsgPtr, szBody, 4096);
        if (ret <= 0)
        {
            strError = "get msg body failed";
            break;
        }
        VTDU_LOG_I("Handle V3 TransReady,body: ", szBody);
        nStatus = m_oXmlParse.ParseXmlTransReady(szBody, stCurMediaInfo, strError);
    } while (0);

    stCurMediaInfo.strVtduRecvIp = m_configSipServer.m_strSipAddr;
    stCurMediaInfo.strVtduSendIP = m_configSipServer.m_strSipAddr;

    char pszBody[4096] = { 0 };
    int iBodyLen = 0;
    if (nStatus != 200)
    {
        iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
        //�ظ�״̬
        SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
        return;
    }
    else
    {
        if (stCurMediaInfo.nCuTransType != 0
            || (stCurMediaInfo.nCuUserType == 1 && stCurMediaInfo.strCuGBVideoTransMode != "UDP")
            || stCurMediaInfo.nPuTransType != 0)
        {
            strError = "unsupport Protocol";
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            //�ظ�״̬
            SipUA_AnswerInfo(pMsgPtr, 400, pszBody, iBodyLen);
            VTDU_LOG_E("HandleHiTransMessage unsupport Protocol:" << stCurMediaInfo.nCuTransType << "_" << stCurMediaInfo.nCuUserType << "_" << stCurMediaInfo.strCuGBVideoTransMode << "_" << stCurMediaInfo.nPuTransType);	
			return;
        }

        //�ǹ����˽������ת��
        if (stCurMediaInfo.nPuProtocol != 1)
        {
            stCurMediaInfo.bTrans = false;
        }
        
        int nInitialUser = 1;
        bool bTrans = stCurMediaInfo.bTrans;
        //�������ڵ�����
         //�������ƵԴ��ͬ��Ŀ��ֱ�����ͬ������(��ʱ���Ƿ�ת�룬������Ҫ��ת����Ӧ�ֱ���)���ѵ�ǰ������ӵ��Ѵ����������һ�����͡�
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%d_%d_%d", stCurMediaInfo.strPUID.c_str(), stCurMediaInfo.nPuChannelID, stCurMediaInfo.nPuStreamType, bTrans);
        std::string strPuInfo = (std::string)szPuInfo;

        bool bExitTask = false;
        Stream *poStream = NULL;
        int nExitTaskRecvPort = 0;
        std::string strExitTaskRecvIp = "";

        mtVtduPreviewTask.lock();
        std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strPuInfo);
        if (itorTask != g_mapVtduPreviewTaskInfo.end())
        {
            bExitTask = true;
            poStream = (Stream *)itorTask->second.pStream;
            nExitTaskRecvPort = itorTask->second.nRecvPort;
            strExitTaskRecvIp = itorTask->second.strRecvIp;
            poStream->getOutstreamNum(nInitialUser);
        }
        mtVtduPreviewTask.unlock();


        //���ҿ�ʹ�ö˿�
        int nSendV3Port = -1;

        mtSendV3Port.lock();
        if (g_vecSendV3Port.size() == 0)
        {
            nStatus = 400;
            VTDU_LOG_E("g_vecSendV3Port is NULL");
            strError = "SendV3Port is use up";
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            mtSendV3Port.unlock();
            return;
        }
        std::vector<int>::iterator itorSendV3 = g_vecSendV3Port.begin();
        nSendV3Port = *itorSendV3;
        g_vecSendV3Port.erase(itorSendV3);
        mtSendV3Port.unlock();

        stCurMediaInfo.nVtduSendPort = nSendV3Port;

        //�������
        if (bExitTask)
        {
            int nRet = poStream->addOneSend(stCurMediaInfo.strCuIp, stCurMediaInfo.nCuport, stCurMediaInfo.strCuUserID, stCurMediaInfo.nVtduSendPort);
            if (0 > nRet)
            {
                nStatus = 400;
                strError = "GBRtpPsOverUdpStream addOneSend failed!";
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();
                return;
            }
            g_nOutstreamNum++;

            stCurMediaInfo.nVtduRecvPort = nExitTaskRecvPort;
            stCurMediaInfo.strVtduRecvIp = strExitTaskRecvIp;
            SipUA_RspPreviewOk(pMsgPtr, stCurMediaInfo, nInitialUser);
            return;
        }
        else//��������
        {
            //���ҿ�ʹ�ö˿�
            int nRecvPort = -1;
            mtRecvPort.lock();
            if (g_vecRecvPort.size() == 0)
            {
                nStatus = 400;
                VTDU_LOG_E("g_vecRecvHiPort is NULL");
                strError = "RecvHiPort is use up";
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                mtRecvPort.unlock();
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();
                return;
            }
            std::vector<int>::iterator itorRecvHi = g_vecRecvPort.begin();
            nRecvPort = *itorRecvHi;
            g_vecRecvPort.erase(itorRecvHi);
            mtRecvPort.unlock();


            stCurMediaInfo.nVtduRecvPort = nRecvPort;

            GBRtpPsOverUdpStream* pStreamHanlde = new(std::nothrow) GBRtpPsOverUdpStream(strPuInfo, nRecvPort, m_configSipServer.m_strSipAddr);
            if (NULL == pStreamHanlde)
            {
                nStatus = 400;
                strError = "New GBRtpPsOverUdpStream object failed!";
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                return;
            }
            pStreamHanlde->setCallBack(StreamInfoCallBack, this);
            int nRet = pStreamHanlde->start(bTrans);
            if (nRet < 0)
            {
                nStatus = 400;
                strError = "GBRtpPsOverUdpStream start failed!";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
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
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                return;
            }

            std::string strHiId = "";
            std::string strHiRegion = "";
            std::string strHiIp = "";
            int nHiPort = -1;
            int nHiRecvPort = -1;
            std::string strHisipId = "";
            //��� ��Ҫת�� ����ע��hi �����������ȡ hi ip�Ͷ˿�
            if (bTrans)
            {
                int nCurHiTaskNum = 10000;
                mtRegHi.lock();
                if (0 == g_mapRegHiInfo.size())
                {
                    pStreamHanlde->stop();
                    delete pStreamHanlde;
                    nStatus = 400;
                    VTDU_LOG_E("g_mapRegHiInfo is NULL");
                    strError = "have no trans module reg";
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                    mtRegHi.unlock();
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nSendV3Port);
                    mtSendV3Port.unlock();

                    mtRecvPort.lock();
                    g_vecRecvPort.push_back(nRecvPort);
                    mtRecvPort.unlock();
                    return;
                }
                std::map<std::string, stHiInfo>::iterator itorRegHi = g_mapRegHiInfo.begin();
                for (; itorRegHi != g_mapRegHiInfo.end(); ++itorRegHi)
                {
                    if (itorRegHi->second.nTansTaskNum == 0)
                    {
                        nCurHiTaskNum = itorRegHi->second.nTansTaskNum;
                        strHiIp = itorRegHi->second.strSipIp;
                        nHiPort = itorRegHi->second.nSipPort;
                        strHiId = itorRegHi->second.strSipId;
                        strHiRegion = itorRegHi->second.strSipRegion;
                        strHisipId = itorRegHi->first;
                        break;
                    }
                    if (itorRegHi->second.nTansTaskNum < nCurHiTaskNum && itorRegHi->second.nTansTaskNum < itorRegHi->second.nMaxTransTaskNum)
                    {
                        nCurHiTaskNum = itorRegHi->second.nTansTaskNum;
                        strHiIp = itorRegHi->second.strSipIp;
                        nHiPort = itorRegHi->second.nSipPort;
                        strHiId = itorRegHi->second.strSipId;
                        strHiRegion = itorRegHi->second.strSipRegion;
                        strHisipId = itorRegHi->first;
                    }
                }
                if (strHisipId == "")
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    VTDU_LOG_E("get Hi failed");
                    strError = "get Hi failed!";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nSendV3Port);
                    mtSendV3Port.unlock();

                    mtRecvPort.lock();
                    g_vecRecvPort.push_back(nRecvPort);
                    mtRecvPort.unlock();
                    return;
                }
                mtRegHi.unlock();
                //sipЭ��֪ͨ��Ӧhiģ�� ��֪���ն˿ڻ�ȡhiģ����ն˿ڣ� �ȴ� 10������û�л�Ӧ ����ѡ��һ�� ֱ��ʧ�� ��һ��hi��������һ��)
                int ret = m_oCommunicationHi.sendTransReq(nRecvPort, strPuInfo, strHiIp, nHiPort);
                if (ret <= 0)
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    strError = "send task to hi failed!";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nSendV3Port);
                    mtSendV3Port.unlock();

                    mtRecvPort.lock();
                    g_vecRecvPort.push_back(nRecvPort);
                    mtRecvPort.unlock();
                    return;
                }
                //hi���ն˿�
                for (int nWait = 0; nWait < 6; nWait++)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    mtGetHirecvPort.lock();
                    std::map<std::string, int>::iterator itorRecvHi = g_mapGetHirecvPort.find(strPuInfo);
                    if (itorRecvHi != g_mapGetHirecvPort.end())
                    {
                        nHiRecvPort = itorRecvHi->second;
                        g_mapGetHirecvPort.erase(itorRecvHi);
                        break;
                    }
                    mtGetHirecvPort.unlock();
                }
                if (nHiRecvPort == -1)
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    VTDU_LOG_E("get hi recv port failed, hi info: " << strHiIp << ":" << nHiPort);
                    strError = "get hi recv port failed";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    //֪ͨȡ������
                    m_oCommunicationHi.sendStopTransReq(strPuInfo, strHiIp, nHiPort);
                    SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nSendV3Port);
                    mtSendV3Port.unlock();

                    mtRecvPort.lock();
                    g_vecRecvPort.push_back(nRecvPort);
                    mtRecvPort.unlock();
                    return;
                }

                mtRegHi.lock();
                g_mapRegHiInfo[strHisipId].nTansTaskNum = nCurHiTaskNum + 1;
                mtRegHi.unlock();
                stCurMediaInfo.strVtduRecvIp = strHiIp;
                stCurMediaInfo.nVtduRecvPort = nHiRecvPort;
            }

            stHiTaskInfo stTask;
            stTask.strHiSipId = strHiId;
            stTask.pStream = (void*)pStreamHanlde;
            stTask.enStreamType = Stream_type::UDP_PS;
            stTask.nRecvPort = stCurMediaInfo.nVtduRecvPort;
            stTask.strRecvIp = stCurMediaInfo.strVtduRecvIp;
            stTask.stMedia = stCurMediaInfo;
            stTask.enTaskType = TASK_type::PREVIEW;
            mtVtduPreviewTask.lock();
            g_mapVtduPreviewTaskInfo[strPuInfo] = stTask;
            mtVtduPreviewTask.unlock();

            SipUA_RspPreviewOk(pMsgPtr, stCurMediaInfo, nInitialUser);

            g_nInstreamNum++;
            g_nOutstreamNum++;
        }
    }
}

/**************************************************************************
* name          : sipServerHandleV3TransStop
* description   : ����v3ֹͣԤ������
* input         : pMsgPtr   v3��Ϣ
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleV3TransStop(void *pMsgPtr)
{
    int nStatus = 400;
    std::string strError = "";
    stMediaInfo stCurMediaInfo;
    stCurMediaInfo.bTrans = false;
    do
    {
        char szBody[4096] = { 0 };
        int ret = SipUA_GetRequestBodyContent(pMsgPtr, szBody, 4096);
        if (ret <= 0)
        {
            strError = "get msg body failed";
            break;
        }
        VTDU_LOG_I("Handle V3 Trans STOP,body: ", szBody);
        nStatus = m_oXmlParse.ParseXmlTransStop(szBody, stCurMediaInfo, strError);
    } while (0);

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
        //�������ڵ�����
         //�������ƵԴ��ͬ��Ŀ��ֱ�����ͬ������(��ʱ���Ƿ�ת�룬������Ҫ��ת����Ӧ�ֱ���)��ɾ��һ�����ͣ���������һ����ɾ������
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%d_%d_%d", stCurMediaInfo.strPUID.c_str(), stCurMediaInfo.nPuChannelID, stCurMediaInfo.nPuStreamType, bTrans);
        std::string strPuInfo = (std::string)szPuInfo;

        bool bExitTask = false;
        Stream *pStream = NULL;
        std::string strHiSipId = "";

        mtVtduPreviewTask.lock();
        std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strPuInfo);
        if (itorTask != g_mapVtduPreviewTaskInfo.end())
        {
            bExitTask = true;
            pStream = (Stream *)(itorTask->second.pStream);
            strHiSipId = itorTask->second.strHiSipId;
        }
        mtVtduPreviewTask.unlock();

        if (bExitTask && pStream != NULL)
        {
            //���շ��Ͷ˿�
            int nCurSendPort = -1;
            int nRet = pStream->DelOneSend(stCurMediaInfo.strCuUserID, nCurSendPort);
            if (0 > nRet)
            {
                //printf("no task:%s\n", );
                //û�ж�Ӧ���� �����κ�����
            }
            else if (0 == nRet) //ת���������
            {
                //���������Դ
                   //�رն˿�
                pStream->stop();
                //���ն˿�
                if (nCurSendPort != -1)
                {
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nCurSendPort);
                    mtSendV3Port.unlock();
                }

                int nRecvPort = -1;
                pStream->getRecvPort(nRecvPort);
                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                delete pStream;
                pStream = NULL;

                if (bTrans)
                {
                    //֪ͨhi
                    std::string strHiId = "";
                    std::string strHiIp = "";
                    int nHiPort = 0;
                    mtRegHi.lock();
                    std::map<std::string, stHiInfo>::iterator itorRegHi = g_mapRegHiInfo.find(strHiSipId);
                    if (itorRegHi == g_mapRegHiInfo.end())
                    {
                        mtRegHi.unlock();
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
                    mtRegHi.unlock();

                    //�ĳ�socket
                    m_oCommunicationHi.sendStopTransReq(strPuInfo, strHiIp, nHiPort);
                }
                g_nInstreamNum--;
                g_nOutstreamNum--;
            }
            else
            {
                //���ն˿�
                if (nCurSendPort != -1)
                {
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nCurSendPort);
                    mtSendV3Port.unlock();
                }
                g_nOutstreamNum--;
            }
        }
        SipUA_RspPreviewStopOk(pMsgPtr, stCurMediaInfo);
    }
}


/**************************************************************************
* name          : sipServerHandleV3FileStart
* description   : ����v3�ط�����
* input         : pMsgPtr   v3��Ϣ
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleV3FileStart(void *pMsgPtr)
{
    int nStatus = 400;
    std::string strError = "";
    stMediaInfo stCurMediaInfo;
    stCurMediaInfo.bTrans = false;
    do
    {
        char szBody[4096] = { 0 };
        int ret = SipUA_GetRequestBodyContent(pMsgPtr, szBody, 4096);
        if (ret <= 0)
        {
            strError = "get msg body failed";
            break;
        }
        VTDU_LOG_I("Handle V3 File Start,body: ", szBody);
        nStatus = m_oXmlParse.ParseXmlV3FileStart(szBody, stCurMediaInfo, strError);
    } while (0);

    char pszBody[4096] = { 0 };
    int iBodyLen = 0;
    if (nStatus != 200)
    {
        iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
        SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
    }
    else
    {
        //�ǹ����˽������ת��
        if (stCurMediaInfo.nPuProtocol != 1)
        {
            stCurMediaInfo.bTrans = false;
        }
        bool bTrans = stCurMediaInfo.bTrans;
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%s_%d", stCurMediaInfo.strCuSession.c_str(), stCurMediaInfo.strCuUserID.c_str(), bTrans);
        std::string strPuInfo = (std::string)szPuInfo;
        //�طŲ���Ҫ�ظ������ж�
        bool bExitTask = false;
        GBRtpPsOverUdpStream *poStream = NULL;

        //���ҿ�ʹ�ö˿�
        int nRecvPort = -1;
        int nSendV3Port = -1;

        mtSendV3Port.lock();
        if (g_vecSendV3Port.size() == 0)
        {
            nStatus = 400;
            VTDU_LOG_E("g_vecSendV3Port is NULL");
            strError = "SendV3Port is use up";
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            mtSendV3Port.unlock();
            return;
        }
        std::vector<int>::iterator itorSendV3 = g_vecSendV3Port.begin();
        nSendV3Port = *itorSendV3;
        g_vecSendV3Port.erase(itorSendV3);
        mtSendV3Port.unlock();

        mtRecvPort.lock();
        if (g_vecRecvPort.size() == 0)
        {
            nStatus = 400;
            VTDU_LOG_E("g_vecRecvHiPort is NULL");
            strError = "RecvHiPort is use up";
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            mtRecvPort.unlock();
            mtSendV3Port.lock();
            g_vecSendV3Port.push_back(nSendV3Port);
            mtSendV3Port.unlock();
            return;
        }
        std::vector<int>::iterator itorRecvHi = g_vecRecvPort.begin();
        nRecvPort = *itorRecvHi;
        g_vecRecvPort.erase(itorRecvHi);
        mtRecvPort.unlock();


        stCurMediaInfo.nVtduRecvPort = nRecvPort;

        stCurMediaInfo.nVtduSendPort = nSendV3Port;

        //����
        GBRtpPsOverUdpStream* pStreamHanlde = new(std::nothrow) GBRtpPsOverUdpStream(strPuInfo, nRecvPort, m_configSipServer.m_strSipAddr);
        if (NULL == pStreamHanlde)
        {
            nStatus = 400;
            strError = "New GBRtpPsOverUdpStream object failed!";
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            mtSendV3Port.lock();
            g_vecSendV3Port.push_back(nSendV3Port);
            mtSendV3Port.unlock();

            mtRecvPort.lock();
            g_vecRecvPort.push_back(nRecvPort);
            mtRecvPort.unlock();
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            return;
        }
        pStreamHanlde->setCallBack(StreamInfoCallBack, this);
        int nRet = pStreamHanlde->start(bTrans);
        if (nRet < 0)
        {
            nStatus = 400;
            strError = "GBRtpPsOverUdpStream start failed!";
            delete pStreamHanlde;
            iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
            mtSendV3Port.lock();
            g_vecSendV3Port.push_back(nSendV3Port);
            mtSendV3Port.unlock();

            mtRecvPort.lock();
            g_vecRecvPort.push_back(nRecvPort);
            mtRecvPort.unlock();
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
            mtSendV3Port.lock();
            g_vecSendV3Port.push_back(nSendV3Port);
            mtSendV3Port.unlock();

            mtRecvPort.lock();
            g_vecRecvPort.push_back(nRecvPort);
            mtRecvPort.unlock();
            SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
            return;
        }

        std::string strHiId = "";
        std::string strHiRegion = "";
        std::string strHiIp = "";
        int nHiPort = -1;
        int nHiRecvPort = -1;
        std::string strHisipId = "";
        //��� ��Ҫת�� ����ע��hi �����������ȡ hi ip�Ͷ˿�
        if (bTrans)
        {
            int nCurHiTaskNum = 10000;
            mtRegHi.lock();
            if (0 == g_mapRegHiInfo.size())
            {
                nStatus = 400;
                VTDU_LOG_E("have no trans module reg");
                strError = "have no trans module reg";
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                mtRegHi.unlock();
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                return;
            }
            std::map<std::string, stHiInfo>::iterator itorRegHi = g_mapRegHiInfo.begin();
            for (; itorRegHi != g_mapRegHiInfo.end(); ++itorRegHi)
            {
                if (itorRegHi->second.nTansTaskNum == 0)
                {
                    nCurHiTaskNum = itorRegHi->second.nTansTaskNum;
                    strHiIp = itorRegHi->second.strSipIp;
                    nHiPort = itorRegHi->second.nSipPort;
                    strHiId = itorRegHi->second.strSipId;
                    strHiRegion = itorRegHi->second.strSipRegion;
                    strHisipId = itorRegHi->first;
                    break;
                }
                if (itorRegHi->second.nTansTaskNum < nCurHiTaskNum && itorRegHi->second.nTansTaskNum < itorRegHi->second.nMaxTransTaskNum)
                {
                    nCurHiTaskNum = itorRegHi->second.nTansTaskNum;
                    strHiIp = itorRegHi->second.strSipIp;
                    nHiPort = itorRegHi->second.nSipPort;
                    strHiId = itorRegHi->second.strSipId;
                    strHiRegion = itorRegHi->second.strSipRegion;
                    strHisipId = itorRegHi->first;
                }
            }
            if (strHisipId == "")
            {
                pStreamHanlde->stop();
                nStatus = 400;
                VTDU_LOG_E("get Hi failed");
                strError = "get Hi failed!";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                return;
            }
            mtRegHi.unlock();

            //sipЭ��֪ͨ��Ӧhiģ�� ��֪���ն˿ڻ�ȡhiģ����ն˿ڣ� �ȴ� 3������û�л�Ӧ ����ѡ��һ�� ֱ��ʧ�� ��һ��hi��������һ��)
            int ret = m_oCommunicationHi.sendTransReq(nRecvPort, strPuInfo, strHiIp, nHiPort);
            if (ret <= 0)
            {
                pStreamHanlde->stop();
                nStatus = 400;
                strError = "send task to hi failed!";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                return;
            }
            //hi���ն˿�
            for (int nWait = 0; nWait < 6; nWait++)
            {
                std::this_thread::sleep_for(std::chrono::milliseconds(500));
                mtGetHirecvPort.lock();
                std::map<std::string, int>::iterator itorRecvHi = g_mapGetHirecvPort.find(strPuInfo);
                if (itorRecvHi != g_mapGetHirecvPort.end())
                {
                    nHiRecvPort = itorRecvHi->second;
                    g_mapGetHirecvPort.erase(itorRecvHi);
                    break;
                }
                mtGetHirecvPort.unlock();
            }
            if (nHiRecvPort == -1)
            {
                pStreamHanlde->stop();
                nStatus = 400;
                VTDU_LOG_E("get hi recv port failed, Hi info: " << strHiIp << ":" << nHiPort);
                strError = "get hi recv port failed";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                //֪ͨȡ������
                m_oCommunicationHi.sendStopTransReq(strPuInfo, strHiIp, nHiPort);

                SipUA_AnswerInfo(pMsgPtr, nStatus, pszBody, iBodyLen);
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                return;
            }

            mtRegHi.lock();
            g_mapRegHiInfo[strHisipId].nTansTaskNum = nCurHiTaskNum + 1;
            mtRegHi.unlock();
            stCurMediaInfo.strVtduRecvIp = strHiIp;
            stCurMediaInfo.nVtduRecvPort = nHiRecvPort;
        }

        stHiTaskInfo stTask;
        stTask.strHiSipId = strHiId;
        stTask.pStream = pStreamHanlde;
        stTask.enStreamType = Stream_type::UDP_PS;
        stTask.enTaskType = TASK_type::PLAYBACK;
        stTask.stMedia = stCurMediaInfo;
        mtVtduPreviewTask.lock();
        g_mapVtduPreviewTaskInfo[strPuInfo] = stTask;
        mtVtduPreviewTask.unlock();

        SipUA_RspPlayBackOk(pMsgPtr, stCurMediaInfo);

        g_nInstreamNum++;
        g_nOutstreamNum++;
    }
}

/**************************************************************************
* name          : sipServerHandleV3FileStop
* description   : ����v3ֹͣ�ط�����
* input         : pMsgPtr   v3��Ϣ
* output        : NA
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::sipServerHandleV3FileStop(void *pMsgPtr)
{
    int nStatus = 400;
    std::string strError = "";
    stMediaInfo stCurMediaInfo;
    stCurMediaInfo.bTrans = false;
    do
    {
        char szBody[4096] = { 0 };
        int ret = SipUA_GetRequestBodyContent(pMsgPtr, szBody, 4096);
        if (ret <= 0)
        {
            strError = "get msg body failed";
            break;
        }
        VTDU_LOG_I("Handle V3 File Stop,body: ", szBody);
        nStatus = m_oXmlParse.ParseXmlV3FileStop(szBody, stCurMediaInfo, strError);
    } while (0);

    char pszBody[4096] = { 0 };
    int iBodyLen = 0;
    if (nStatus != 200)
    {
        iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
    }
    else
    {
        bool bTrans = stCurMediaInfo.bTrans;
        //�������ڵ�����
         //�������ƵԴ��ͬ��Ŀ��ֱ�����ͬ������(��ʱ���Ƿ�ת�룬������Ҫ��ת����Ӧ�ֱ���)��ɾ��һ�����ͣ���������һ����ɾ������
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%s_%d", stCurMediaInfo.strCuSession.c_str(), stCurMediaInfo.strCuUserID.c_str(), bTrans);
        std::string strPuInfo = (std::string)szPuInfo;

        bool bExitTask = false;
        Stream *pStream = NULL;
        std::string strHiSipId = "";

        mtVtduPreviewTask.lock();
        std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strPuInfo);
        if (itorTask != g_mapVtduPreviewTaskInfo.end())
        {
            bExitTask = true;
            pStream = (Stream *)(itorTask->second.pStream);
            strHiSipId = itorTask->second.strHiSipId;
        }
        mtVtduPreviewTask.unlock();

        if (bExitTask && pStream != NULL)
        {
            //���շ��Ͷ˿�
            int nCurSendPort = -1;
            int nRet = pStream->DelOneSend(stCurMediaInfo.strCuUserID, nCurSendPort);
            if (0 > nRet)
            {
                //û�ж�Ӧ���� �����κ�����
            }
            else if (0 == nRet) //ת���������
            {
                //���������Դ
                   //�رն˿�
                pStream->stop();
                //���ն˿�
                if (nCurSendPort != -1)
                {
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nCurSendPort);
                    mtSendV3Port.unlock();
                }

                int nRecvPort = -1;
                pStream->getRecvPort(nRecvPort);
                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                delete pStream;
                pStream = NULL;
                if (bTrans)
                {
                    //֪ͨhi
                    std::string strHiId = "";
                    std::string strHiIp = "";
                    int nHiPort = 0;
                    mtRegHi.lock();
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
                    mtRegHi.unlock();
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
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nCurSendPort);
                    mtSendV3Port.unlock();
                }
                g_nOutstreamNum--;
            }
        }

        SipUA_RspPlayBackStopOk(pMsgPtr, stCurMediaInfo);
    }
}

/**************************************************************************
* name          : HandleHiRegister
* description   : ����hiע������
* input         : pMsgPtr   hiע����Ϣ
* output        : szSendBuff ��Ҫ�ظ�hi����Ϣ
*                 nSendBufLen ��Ҫ�ظ�hi����Ϣ����
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

        mtRegHi.lock();
        g_mapRegHiInfo[stCurHi.strSipId] = stCurHi;
        mtRegHi.unlock();
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
* description   : ����hi������Ϣ
* input         : pMsgPtr   hi������Ϣ
* output        : szSendBuff ��Ҫ�ظ�hi����Ϣ
*                 nSendBufLen ��Ҫ�ظ�hi����Ϣ����
* return        : NA
* remark        : NA
**************************************************************************/
void VtduServer::HandleHiHeartBeat(char *pMsgPtr, char* szSendBuff, int* nSendBufLen)
{
    *nSendBufLen = 0;
    stHiInfo stCurHi;
    if (pMsgPtr)
    {
        bool bFindSipInfo = false;   //�Ƿ���sip��Ϣ
        bool bNeedReconnect = false; //�Ƿ�֪ͨ����
        int nRet = m_oXmlParse.ParseXmlHiHeartBeat(pMsgPtr, stCurHi, bFindSipInfo);

        __time64_t nCurTime = _time64(NULL);
        mtRegHi.lock();
        std::map<std::string, stHiInfo>::iterator iotr = g_mapRegHiInfo.find(stCurHi.strSipId);
        if (iotr != g_mapRegHiInfo.end())
        {
            iotr->second.nHeartBeat = nCurTime;
        }
        else//���ûע�����������������vtdu���ߺ����������յ���hiģ��������ֱ�ӷ���ע���б� ��ո������ ֪ͨhi ������Դ 
        {
            if (bFindSipInfo)
            {
                bNeedReconnect = true;
                stCurHi.nHeartBeat = nCurTime;
                stCurHi.nTansTaskNum = 0;
                g_mapRegHiInfo[stCurHi.strSipId] = stCurHi;
            }
        }
        mtRegHi.unlock();

        if (bNeedReconnect)
        {
            m_oCommunicationHi.sendReconnect(stCurHi.strSipIp, stCurHi.nSipPort);
        }
        
        return;
    }
}

/**************************************************************************
* name          : HandleHiTansRsp
* description   : ����hiת��ظ���Ϣ
* input         : pMsgPtr   hi�ظ���Ϣ
* output        : szSendBuff ��Ҫ�ظ�hi����Ϣ
*                 nSendBufLen ��Ҫ�ظ�hi����Ϣ����
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
        mtGetHirecvPort.lock();
        g_mapGetHirecvPort[strHiTaskId] = nHiRecvPort;
        mtGetHirecvPort.unlock();

        return;
    }
}

/**************************************************************************
* name          : HandleHiCutout
* description   : ����hi������Ϣ
* input         : pMsgPtr   hi�ظ���Ϣ
* output        : szSendBuff ��Ҫ�ظ�hi����Ϣ
*                 nSendBufLen ��Ҫ�ظ�hi����Ϣ����
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

        //����task idѰ��������Ϣ���ͷ�stream�����ն˿�
        Stream* pStream = NULL;
        std::string strHiSipId = "";
        stMediaInfo stCurMediaInfo;
        TASK_type enTaskType = TASK_type::PLAYBACK;
        mtVtduPreviewTask.lock();
        std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strHiTaskId);
        if (itorTask != g_mapVtduPreviewTaskInfo.end())
        {
            pStream = (Stream*)(itorTask->second.pStream);
            stCurMediaInfo = itorTask->second.stMedia;
            enTaskType = itorTask->second.enTaskType;
            g_mapVtduPreviewTaskInfo.erase(itorTask);
        }
        mtVtduPreviewTask.unlock();
        if (NULL != pStream)
        {
            pStream->stop();
            //�������з��Ͷ˿�
            std::vector<int> vecSendPort;
            pStream->getSendPort(vecSendPort);
            mtSendV3Port.lock();
            for (std::vector<int>::iterator itorvecSendPort = vecSendPort.begin(); itorvecSendPort != vecSendPort.end(); ++itorvecSendPort)
            {
                g_vecSendV3Port.push_back(*itorvecSendPort);
            }
            mtSendV3Port.unlock();

            //���ս��ն˿�
            int nRecvPort = -1;
            pStream->getRecvPort(nRecvPort);
            if (-1 != nRecvPort)
            {
                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
            }

            //�������������·��
            int nOutNum = 0;
            pStream->getOutstreamNum(nOutNum);
            g_nOutstreamNum -= nOutNum;
            g_nInstreamNum--;
            delete pStream;
            pStream = NULL;

            // ֪ͨv3���� //�طŴ�ȷ��
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
* description   : ����v3Ԥ������ ���Ժ���
* input         : pMsgPtr   v3��Ϣ
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
        //�������ڵ�����
         //�������ƵԴ��ͬ��Ŀ��ֱ�����ͬ������(��ʱ���Ƿ�ת�룬������Ҫ��ת����Ӧ�ֱ���)���ѵ�ǰ������ӵ��Ѵ����������һ�����͡�
        char szPuInfo[100] = { 0 };
        sprintf(szPuInfo, "%s_%d_%d_%d", stCurMediaInfo.strPUID.c_str(), stCurMediaInfo.nPuChannelID, stCurMediaInfo.nPuStreamType, bTrans);
        std::string strPuInfo = (std::string)szPuInfo;

        bool bExitTask = false;
        Stream *poStream = NULL;
        int nExitTaskRecvPort = 0;
        std::string strExitTaskRecvIp = "";

        mtVtduPreviewTask.lock();
        std::map<std::string, stHiTaskInfo>::iterator itorTask = g_mapVtduPreviewTaskInfo.find(strPuInfo);
        if (itorTask != g_mapVtduPreviewTaskInfo.end())
        {
            bExitTask = true;
            poStream = (Stream *)itorTask->second.pStream;
            nExitTaskRecvPort = itorTask->second.nRecvPort;
            strExitTaskRecvIp = itorTask->second.strRecvIp;
            poStream->getOutstreamNum(nInitialUser);
        }
        mtVtduPreviewTask.unlock();


        //���ҿ�ʹ�ö˿�
        int nSendV3Port = -1;

        mtSendV3Port.lock();
        if (g_vecSendV3Port.size() == 0)
        {
            nStatus = 400;
            VTDU_LOG_E("g_vecSendV3Port is NULL");
            mtSendV3Port.unlock();
            return;
        }
        std::vector<int>::iterator itorSendV3 = g_vecSendV3Port.begin();
        nSendV3Port = *itorSendV3;
        g_vecSendV3Port.erase(itorSendV3);
        mtSendV3Port.unlock();

        stCurMediaInfo.nVtduSendPort = nSendV3Port;

        //�������
        if (bExitTask)
        {
            int nRet = poStream->addOneSend(stCurMediaInfo.strCuIp, stCurMediaInfo.nCuport, stCurMediaInfo.strCuUserID, stCurMediaInfo.nVtduSendPort);
            if (0 > nRet)
            {
                nStatus = 400;
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();
                return;
            }
            g_nOutstreamNum++;

            stCurMediaInfo.nVtduRecvPort = nExitTaskRecvPort;
            stCurMediaInfo.strVtduRecvIp = strExitTaskRecvIp;
            return;
        }
        else//��������
        {
            //���ҿ�ʹ�ö˿�
            int nRecvPort = -1;
            mtRecvPort.lock();
            if (g_vecRecvPort.size() == 0)
            {
                nStatus = 400;
                VTDU_LOG_E("g_vecRecvHiPort is NULL");
                mtRecvPort.unlock();
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();
                return;
            }
            std::vector<int>::iterator itorRecvHi = g_vecRecvPort.begin();
            nRecvPort = *itorRecvHi;
            g_vecRecvPort.erase(itorRecvHi);
            mtRecvPort.unlock();


            stCurMediaInfo.nVtduRecvPort = nRecvPort;

            GBRtpPsOverUdpStream* pStreamHanlde = new(std::nothrow) GBRtpPsOverUdpStream(strPuInfo, nRecvPort, m_configSipServer.m_strSipAddr);
            if (NULL == pStreamHanlde)
            {
                nStatus = 400;
                strError = "New GBRtpPsOverUdpStream object failed!";
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                return;
            }
            pStreamHanlde->setCallBack(StreamInfoCallBack, this);
            int nRet = pStreamHanlde->start(bTrans);
            if (nRet < 0)
            {
                nStatus = 400;
                strError = "GBRtpPsOverUdpStream start failed!";
                delete pStreamHanlde;
                iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
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
                mtSendV3Port.lock();
                g_vecSendV3Port.push_back(nSendV3Port);
                mtSendV3Port.unlock();

                mtRecvPort.lock();
                g_vecRecvPort.push_back(nRecvPort);
                mtRecvPort.unlock();
                return;
            }

            std::string strHiId = "";
            std::string strHiRegion = "";
            std::string strHiIp = "";
            int nHiPort = -1;
            int nHiRecvPort = -1;
            std::string strHisipId = "";
            //��� ��Ҫת�� ����ע��hi �����������ȡ hi ip�Ͷ˿�
            if (bTrans)
            {
                int nCurHiTaskNum = 10000;
                mtRegHi.lock();
                if (0 == g_mapRegHiInfo.size())
                {
                    pStreamHanlde->stop();
                    delete pStreamHanlde;
                    nStatus = 400;
                    VTDU_LOG_E("g_mapRegHiInfo is NULL");
                    strError = "have no trans module reg";
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    mtRegHi.unlock();
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nSendV3Port);
                    mtSendV3Port.unlock();

                    mtRecvPort.lock();
                    g_vecRecvPort.push_back(nRecvPort);
                    mtRecvPort.unlock();
                    return;
                }
                std::map<std::string, stHiInfo>::iterator itorRegHi = g_mapRegHiInfo.begin();
                for (; itorRegHi != g_mapRegHiInfo.end(); ++itorRegHi)
                {
                    if (itorRegHi->second.nTansTaskNum == 0)
                    {
                        nCurHiTaskNum = itorRegHi->second.nTansTaskNum;
                        strHiIp = itorRegHi->second.strSipIp;
                        nHiPort = itorRegHi->second.nSipPort;
                        strHiId = itorRegHi->second.strSipId;
                        strHiRegion = itorRegHi->second.strSipRegion;
                        strHisipId = itorRegHi->first;
                        break;
                    }
                    if (itorRegHi->second.nTansTaskNum < nCurHiTaskNum && itorRegHi->second.nTansTaskNum < itorRegHi->second.nMaxTransTaskNum)
                    {
                        nCurHiTaskNum = itorRegHi->second.nTansTaskNum;
                        strHiIp = itorRegHi->second.strSipIp;
                        nHiPort = itorRegHi->second.nSipPort;
                        strHiId = itorRegHi->second.strSipId;
                        strHiRegion = itorRegHi->second.strSipRegion;
                        strHisipId = itorRegHi->first;
                    }
                }
                if (strHisipId == "")
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    VTDU_LOG_E("get Hi failed");
                    strError = "get Hi failed!";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nSendV3Port);
                    mtSendV3Port.unlock();

                    mtRecvPort.lock();
                    g_vecRecvPort.push_back(nRecvPort);
                    mtRecvPort.unlock();
                    return;
                }
                mtRegHi.unlock();
                //sipЭ��֪ͨ��Ӧhiģ�� ��֪���ն˿ڻ�ȡhiģ����ն˿ڣ� �ȴ� 10������û�л�Ӧ ����ѡ��һ�� ֱ��ʧ�� ��һ��hi��������һ��)
                int ret = m_oCommunicationHi.sendTransReq(nRecvPort, strPuInfo, strHiIp, nHiPort);
                if (ret <= 0)
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    strError = "send task to hi failed!";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nSendV3Port);
                    mtSendV3Port.unlock();

                    mtRecvPort.lock();
                    g_vecRecvPort.push_back(nRecvPort);
                    mtRecvPort.unlock();
                    return;
                }
                //hi���ն˿�
                for (int nWait = 0; nWait < 6; nWait++)
                {
                    std::this_thread::sleep_for(std::chrono::milliseconds(500));
                    mtGetHirecvPort.lock();
                    std::map<std::string, int>::iterator itorRecvHi = g_mapGetHirecvPort.find(strPuInfo);
                    if (itorRecvHi != g_mapGetHirecvPort.end())
                    {
                        nHiRecvPort = itorRecvHi->second;
                        g_mapGetHirecvPort.erase(itorRecvHi);
                        mtGetHirecvPort.unlock();
                        break;
                    }
                    mtGetHirecvPort.unlock();
                }
                if (nHiRecvPort == -1)
                {
                    pStreamHanlde->stop();
                    nStatus = 400;
                    VTDU_LOG_E("get hi recv port failed, hi info: " << strHiIp << ":" << nHiPort);
                    strError = "get hi recv port failed";
                    delete pStreamHanlde;
                    iBodyLen = sprintf(pszBody, "error info: %s", strError.c_str());
                    //֪ͨȡ������
                    m_oCommunicationHi.sendStopTransReq(strPuInfo, strHiIp, nHiPort);
                    mtSendV3Port.lock();
                    g_vecSendV3Port.push_back(nSendV3Port);
                    mtSendV3Port.unlock();

                    mtRecvPort.lock();
                    g_vecRecvPort.push_back(nRecvPort);
                    mtRecvPort.unlock();
                    return;
                }

                mtRegHi.lock();
                g_mapRegHiInfo[strHisipId].nTansTaskNum = nCurHiTaskNum + 1;
                mtRegHi.unlock();
                stCurMediaInfo.strVtduRecvIp = strHiIp;
                stCurMediaInfo.nVtduRecvPort = nHiRecvPort;
            }

            stHiTaskInfo stTask;
            stTask.strHiSipId = strHiId;
            stTask.pStream = (void*)pStreamHanlde;
            stTask.enStreamType = Stream_type::UDP_PS;
            stTask.nRecvPort = stCurMediaInfo.nVtduRecvPort;
            stTask.strRecvIp = stCurMediaInfo.strVtduRecvIp;
            stTask.stMedia = stCurMediaInfo;
            stTask.enTaskType = TASK_type::PREVIEW;
            mtVtduPreviewTask.lock();
            g_mapVtduPreviewTaskInfo[strPuInfo] = stTask;
            mtVtduPreviewTask.unlock();

            g_nInstreamNum++;
            g_nOutstreamNum++;
        }
    }
}
