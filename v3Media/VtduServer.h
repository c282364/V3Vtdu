/**   @file    VtduServer.h
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
#include <thread>
#include "GB28181Stream.h"
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
    * description   : ��ʼ��
    * input         : strCfgFile  �����ļ�·��
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int Init(const std::string &strCfgFile);

    /**************************************************************************
    * name          : Fini
    * description   : ����ʼ��
    * input         : NA
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int Fini();

    /**************************************************************************
    * name          : Start
    * description   : ����
    * input         : NA
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int Start();

    /**************************************************************************
    * name          : Stop
    * description   : ֹͣ
    * input         : NA
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int Stop();

    /**************************************************************************
    * name          : sipClientHandleRegisterSuccess
    * description   : ����ע��ɹ���Ϣ
    * input         : pMsgPtr sip��Ϣ
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipClientHandleRegisterSuccess(void *pMsgPtr);

    /**************************************************************************
    * name          : sipClientHandleRegisterFailed
    * description   : ����ע��ʧ��
    * input         : pMsgPtr sip��Ϣ
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipClientHandleRegisterFailed(void *pMsgPtr);

    /**************************************************************************
    * name          : sipServerHandleInfo
    * description   : ����info��Ϣ
    * input         : pMsgPtr sip��Ϣ
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleInfo(void *pMsgPtr);

    /**************************************************************************
    * name          : sipClientHandleOptionFailed
    * description   : option��Ϣ����ʧ�ܣ�option��Ϣ��Ϊ��������ʧ���ж�Ϊ����˵���
    * input         : pMsgPtr sip��Ϣ
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipClientHandleOptionFailed();

    /**************************************************************************
    * name          : sipClientHandleOptionFailed
    * description   : option��Ϣ����ʧ�ܣ�option��Ϣ��Ϊ��������ʧ���ж�Ϊ����˵���
    * input         : pMsgPtr    �յ�����Ϣ
    *                 nLen       �յ�����Ϣ����
    * output        : szSendBuff �ظ���Ϣ
    *                 nSendBuf   �ظ���Ϣ����
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiTransMessage(char *pMsgPtr, int nLen, char* szSendBuff, int* nSendBuf);

    /**************************************************************************
    * name          : HandleStreamInfoCallBack
    * description   : ������ģ���ϱ���Ϣ
    * input         : nType     ��Ϣ���� 0��v3���� 1hiģ�����
    *                 strCbInfo ��Ϣ����
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleStreamInfoCallBack(int nType, std::string strCbInfo);

    /**************************************************************************
    * name          : HandleHiRegister
    * description   : ����hiע������
    * input         : pMsgPtr   hiע����Ϣ
    * output        : szSendBuff ��Ҫ�ظ�hi����Ϣ
    *                 nSendBufLen ��Ҫ�ظ�hi����Ϣ����
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiRegister(char *pMsgPtr, char* szSendBuff, int* nSendBufLen);

    /**************************************************************************
    * name          : HandleHiHeartBeat
    * description   : ����hi������Ϣ
    * input         : pMsgPtr   hi������Ϣ
    * output        : szSendBuff ��Ҫ�ظ�hi����Ϣ
    *                 nSendBufLen ��Ҫ�ظ�hi����Ϣ����
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiHeartBeat(char *pMsgPtr, char* szSendBuff, int* nSendBufLen);

    /**************************************************************************
    * name          : HandleHiTansRsp
    * description   : ����hiת��ظ���Ϣ
    * input         : pMsgPtr   hi�ظ���Ϣ
    * output        : szSendBuff ��Ҫ�ظ�hi����Ϣ
    *                 nSendBufLen ��Ҫ�ظ�hi����Ϣ����
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiTansRsp(char *pMsgPtr, char* szSendBuff, int* nSendBufLen);

    /**************************************************************************
    * name          : HandleHiCutout
    * description   : ����hi������Ϣ
    * input         : pMsgPtr   hi�ظ���Ϣ
    * output        : szSendBuff ��Ҫ�ظ�hi����Ϣ
    *                 nSendBufLen ��Ҫ�ظ�hi����Ϣ����
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void HandleHiCutout(char *pMsgPtr, char* szSendBuff, int* nSendBufLen);

    /**************************************************************************
    * name          : threadHiManager
    * description   : ת��ģ�鱣������߳�
    * input         : pParam �û�����
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    void threadHiManager();

    /**************************************************************************
    * name          : threadRegLoop
    * description   : ע��v3 sip�߳�
    * input         : pParam �û�����
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    void threadRegLoop();

    /**************************************************************************
    * name          : threadHeartBeat
    * description   : v3 sip�����߳�
    * input         : pParam �û�����
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    void threadHeartBeat();

    /**************************************************************************
    * name          : threadStreamCBMsgLoop
    * description   : ��Ƶ������ģ���ϱ���Ϣ�����߳�
    * input         : pParam �û�����
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    void threadStreamCBMsgLoop();

 private:
    /**************************************************************************
    * name          : sipServerHandleV3TransReady
    * description   : ����v3Ԥ������
    * input         : pMsgPtr   v3��Ϣ
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleV3TransReady(void *pMsgPtr);

    /**************************************************************************
    * name          : sipServerHandleV3TransStop
    * description   : ����v3ֹͣԤ������
    * input         : pMsgPtr   v3��Ϣ
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleV3TransStop(void *pMsgPtr);

    /**************************************************************************
    * name          : sipServerHandleV3FileStart
    * description   : ����v3�ط�����
    * input         : pMsgPtr   v3��Ϣ
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleV3FileStart(void *pMsgPtr);

    /**************************************************************************
    * name          : sipServerHandleV3FileStop
    * description   : ����v3ֹͣ�ط�����
    * input         : pMsgPtr   v3��Ϣ
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void sipServerHandleV3FileStop(void *pMsgPtr);


private:
    ConfigSipServer m_configSipServer;//vtdu����

    std::vector<int> g_vecSendV3Port;                             //���Ͷ˿��б�
    std::vector<int> g_vecRecvPort;                               //���ն˿��б�
    std::map<std::string, stHiInfo> g_mapRegHiInfo;               //��ע��ת��ģ���б�
    std::map<std::string, stHiTaskInfo> g_mapVtduPreviewTaskInfo; //��Ƶ�����б�
    std::map<std::string, int> g_mapGetHirecvPort;                //hiת��ģ���ϱ����ն˿��б�
    std::vector<stCutOffInfo>g_vecCutOff;                         //����ģ���б�

    std::mutex mtRegHi;           //��ע��ת��ģ���б���
    std::mutex mtVtduPreviewTask; //��Ƶ�����б���
    std::mutex mtSendV3Port;      //���Ͷ˿��б���
    std::mutex mtRecvPort;        //���ն˿��б���
    std::mutex mtGetHirecvPort;   //hiת��ģ���ϱ����ն˿��б���
    std::mutex mtCutOff;          //����ģ���б���

    int g_nRegid;     //v3 sipע��id
    bool g_bStopHeartBeat; //v3�����߳�ֹͣ��־
    bool g_bStopReg;       //v3ע���߳�ֹͣ��־
    __time64_t g_nLastRegTime; //��һ��ע��v3ƽ̨ʱ��

    int g_nInstreamNum;    //������Ƶ·��
    int g_nOutstreamNum;   //�����Ƶ·��
    bool g_bStopHiManager; //ת��ģ������߳�ֹͣ��־

    communicationHi m_oCommunicationHi; //hiģ�齻����
    XmlParse        m_oXmlParse;        //xml������
};
