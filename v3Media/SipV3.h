/**   @file    SipV3.h
*    @note    All Right Reserved.
*    @brief   v3ƽ̨ϡ��gbЭ�顣
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
#include "GBRtpPsOverUdpStream.h"
#include "CommDef.h"

/**************************************************************************
* name          : SipUA_Init
* description   : ��ʼ��
* input         : stSipCfg sip������Ϣ
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_Init(const ConfigSipServer& stSipCfg);

/**************************************************************************
* name          : SipUA_Fini
* description   : ����ʼ��
* input         : NA
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_Fini();

/**************************************************************************
* name          : SipUA_StartEventLoop
* description   : ��ʼSIPUA�¼�
* input         : fnCB �ص�����
*                 pParam �û�����
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_StartEventLoop(fnSipUAEventCB fnCB, void *pParam);

/**************************************************************************
* name          : SipUA_StartEventLoop
* description   : ����SIPUA�¼�
* input         : NA
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_StopEventLoop();

/**************************************************************************
* name          : SipUA_InitMessage
* description   : ����sip����
* input         : pszMethod sip�������� OPTION/INFO/MESSAGE
*                 pszSrcId ����sip id
*                 pszSrcIP ���� ip
*                 iSrcPort ���� sip �˿�
*                 pszDestId Ŀ�� sip id
*                 pszDestIP Ŀ�� sip ip
*                 iDestPort Ŀ�� sip �˿�
*                 pszBody sip ��Ϣ��
*                 iBodyLen sip ��Ϣ�峤��
*                 strV3Head sip v3ͷ
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_InitMessage(const char * pszMethod, const char *pszSrcId, const char *pszSrcIP, int iSrcPort, const char *pszDestId, const char *pszDestIP, int iDestPort,
    const char *pszBody, int iBodyLen, std::string strV3Head);

/**************************************************************************
* name          : SipUA_Register
* description   : ���� sipע��
* input         : pszSipId        ����sip id
*                 pszSipRegion    ����sip Region
*                 pszSipAddr      ���� ip
*                 iSipPort        ���� sip �˿�
*                 expires         ע����Ч��
*                 pszServerId     Ŀ�� sip id
*                 pszServerRegion sip������Region
*                 pszServerIP     sip������ ip
*                 iServerPort     sip������ �˿�
*                 pszBody         sip��Ϣ��
*                 iBodyLen        sip��Ϣ�峤��
*                 nRegid          sip ע��id �������-1������ע��
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_Register(const char *pszSipId, const char * pszSipRegion, const char *pszSipAddr, int iSipPort, int expires,
    const char *pszServerId, const char * pszServerRegion, const char *pszServerIP, int iServerPort,
    const char *pszBody, int iBodyLen, int nRegid);

/**************************************************************************
* name          : SipUA_AnswerInfo
* description   : �ظ� sip������Ϣ
* input         : pMsgPtr  �յ���������Ϣ
*                 status   ״̬��
*                 pszBody  sip��Ϣ��
*                 iBodyLen sip��Ϣ�峤��
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_AnswerInfo(void *pMsgPtr, int status, const char *pszBody, int iBodyLen);

/**************************************************************************
* name          : SipUA_GetRequestBodyContent
* description   : ��ȡv3������Ϣ��
* input         : pMsgPtr  �յ���������Ϣ
*                 iMaxBodyLen �����Ϣ����󳤶�
* output        : pszOutBody   �����Ϣ��
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_GetRequestBodyContent(void *pMsgPtr, char *pszOutBody, int iMaxBodyLen);

/**************************************************************************
* name          : SipUA_GetResponseBodyContent
* description   : ��ȡv3�ظ���Ϣ��
* input         : pMsgPtr  �յ���������Ϣ
*                 iMaxBodyLen �����Ϣ����󳤶�
* output        : pszOutBody   �����Ϣ��
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_GetResponseBodyContent(void *pMsgPtr, char *pszOutBody, int iMaxBodyLen);

/**************************************************************************
* name          : SipUA_RspPreviewOk
* description   : �ظ�v3Ԥ������ɹ���Ϣ
* input         : pMsgPtr  �յ���v3 sip������Ϣ
*                 stCurMediaInfo Ԥ��������Ϣ
*                 nInitialUser ���շ�·��
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_RspPreviewOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo, int nInitialUser);

/**************************************************************************
* name          : SipUA_RspPreviewStopOk
* description   : �ظ�v3ֹͣԤ������ɹ���Ϣ
* input         : pMsgPtr  �յ���v3 sip������Ϣ
*                 stCurMediaInfo Ԥ��������Ϣ
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_RspPreviewStopOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo);

/**************************************************************************
* name          : SipUA_RspPlayBackOk
* description   : �ظ�v3�ط�����ɹ���Ϣ
* input         : pMsgPtr  �յ���v3 sip������Ϣ
*                 stCurMediaInfo Ԥ��������Ϣ
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_RspPlayBackOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo);

/**************************************************************************
* name          : SipUA_RspPlayBackStopOk
* description   : �ظ�v3ֹͣ�ط�����ɹ���Ϣ
* input         : pMsgPtr  �յ���v3 sip������Ϣ
*                 stCurMediaInfo Ԥ��������Ϣ
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_RspPlayBackStopOk(void* pMsgPtr, const stMediaInfo &stCurMediaInfo);

/**************************************************************************
* name          : SipUA_RegisterV3
* description   : ����v3 ע������
* input         : configSipServer  vtdu������Ϣ
*                 bNeedBody �Ƿ���Ҫ��װbody
*                 nRegid ע��id �������0 ������ע�����
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_RegisterV3(const ConfigSipServer &configSipServer, bool bNeedBody, int nRegid);

/**************************************************************************
* name          : SipUA_Timeout
* description   : ����v3 ������ʱ
* input         : stCurMediaInfo  ������Ϣ
*                 configSipServer  vtdu������Ϣ
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_Timeout(const stMediaInfo &stCurMediaInfo, const ConfigSipServer &configSipServer);

/**************************************************************************
* name          : SipUA_HeartBeat
* description   : ����v3 ����
* input         : configSipServer  vtdu������Ϣ
*                 nInstreamNum     ������Ƶ��·��
*                 nOutstreamNum    �����Ƶ��·��
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int SipUA_HeartBeat(const ConfigSipServer &configSipServer, int nInstreamNum, int nOutstreamNum);
