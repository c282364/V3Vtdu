/**   @file    XmlParse.h
*    @note    All Right Reserved.
*    @brief   xml������
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
    * description   : ��ȡv3������Ϣ
    * input         : pszBody   v3������Ϣ��
    *                 iBodyLen  v3������Ϣ�峤��
    * output        : NA
    * return        : ��Ϣ����
    * remark        : NA
    **************************************************************************/
    std::string getMsgCmdTypeV3(const char *pszBody, int iBodyLen);

    /**************************************************************************
    * name          : ParseXmlTransReady
    * description   : ����v3Ԥ��������Ϣ
    * input         : pszBody   v3������Ϣ��
    * output        : stCurMediaInfo �������������Ϣ
    *                 strError       ������Ϣ
    * return        : 200 ���� ����200�쳣
    * remark        : NA
    **************************************************************************/
    int ParseXmlTransReady(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError);

    /**************************************************************************
    * name          : ParseXmlTransStop
    * description   : ����v3ֹͣԤ��������Ϣ
    * input         : pszBody   v3������Ϣ��
    * output        : stCurMediaInfo �������������Ϣ
    *                 strError       ������Ϣ
    * return        : 200 ���� ����200�쳣
    * remark        : NA
    **************************************************************************/
    int ParseXmlTransStop(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError);

    /**************************************************************************
    * name          : ParseRegisterSuccessXml
    * description   : ����v3ע��ɹ���Ϣ
    * input         : pszBody   v3������Ϣ��
    * output        : nErrorcode ������
    * return        : 200 ���� ����200�쳣
    * remark        : NA
    **************************************************************************/
    int ParseRegisterSuccessXml(const char* szBody, int &nErrorcode);

    /**************************************************************************
    * name          : ParseXmlV3FileStart
    * description   : ����v3�ط�������Ϣ
    * input         : pszBody   v3������Ϣ��
    * output        : stCurMediaInfo �������������Ϣ
    *                 strError       ������Ϣ
    * return        : 200 ���� ����200�쳣
    * remark        : NA
    **************************************************************************/
    int ParseXmlV3FileStart(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError);

    /**************************************************************************
    * name          : ParseXmlV3FileStop
    * description   : ����v3ֹͣ�ط�������Ϣ
    * input         : pszBody   v3������Ϣ��
    * output        : stCurMediaInfo �������������Ϣ
    *                 strError       ������Ϣ
    * return        : 200 ���� ����200�쳣
    * remark        : NA
    **************************************************************************/
    int ParseXmlV3FileStop(const char* szBody, stMediaInfo& stCurMediaInfo, std::string &strError);

    /**************************************************************************
    * name          : ParseXmlHiReg
    * description   : ����hiע��������Ϣ
    * input         : pszBody  hi������Ϣ��
    * output        : stCurHi �������ע����Ϣ
    *                 strError       ������Ϣ
    * return        :  0 ���� С��0�쳣
    * remark        : NA
    **************************************************************************/
    int ParseXmlHiReg(const char* szBody, stHiInfo& stCurHi, std::string &strError);

    /**************************************************************************
    * name          : ParseXmlHiHeartBeat
    * description   : ����hi������Ϣ
    * input         : pszBody  hi������Ϣ��
    * output        : stCurHi �������ע����Ϣ
    *                 bFindSipInfo �Ƿ���ҵ�sip��Ϣ
    * return        :  0 ���� С��0�쳣
    * remark        : NA
    **************************************************************************/
    int ParseXmlHiHeartBeat(const char* szBody, stHiInfo& stCurHi, bool& bFindSipInfo);

    /**************************************************************************
    * name          : ParseXmlHiTansRsp
    * description   : ����hiת��ظ���Ϣ
    * input         : pszBody  hi������Ϣ��
    * output        : strHiTaskId ����id
    *                 nHiRecvPort hi�������ն˿�
    * return        :  0 ���� С��0�쳣
    * remark        : NA
    **************************************************************************/
    int ParseXmlHiTansRsp(const char* szBody, std::string& strHiTaskId, int& nHiRecvPort);

    /**************************************************************************
    * name          : ParseXmlHiCutout
    * description   : ����hi������Ϣ
    * input         : pszBody  hi������Ϣ��
    * output        : strHiTaskId ����id
    * return        :  0 ���� С��0�쳣
    * remark        : NA
    **************************************************************************/
    int ParseXmlHiCutout(const char* szBody, std::string& strHiTaskId);

        
    

    

};

