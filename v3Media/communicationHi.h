#pragma once
#include "CommDef.h"

class communicationHi
{
public:
    communicationHi();
    virtual ~communicationHi();

    /**************************************************************************
    * name          : Init
    * description   : ��ʼ��
    * input         : configSipServer vtdu������Ϣ
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int Init(const ConfigSipServer &configSipServer);

    /**************************************************************************
    * name          : FIni
    * description   : ����ʼ��
    * input         : NA
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int FIni();

    /**************************************************************************
    * name          : StartEventLoop
    * description   : ��ʼ��Ϣ����
    * input         : fnCB ��Ϣ�ϱ�����
    *                 pParam �û�ָ��
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int StartEventLoop(fHiEventCB fnCB, void *pParam);

    /**************************************************************************
    * name          : threadHiMsgLoop
    * description   : hiģ���ϱ���Ϣ�����߳�
    * input         : pParam �û�����
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    void threadHiMsgLoop();

    /**************************************************************************
    * name          : sendTransReq
    * description   : ����ת������
    * input         : nRecvPort vtdu���ݽ��ն˿�
    *                 strPuInfo ����id
    *                 strHiIp  ת��ģ��ip
    *                 nHiPort  ת��ģ��˿�
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int sendTransReq(int nRecvPort, const std::string &strPuInfo, const std::string &strHiIp, int nHiPort);

    /**************************************************************************
    * name          : sendStopTransReq
    * description   : ����ֹͣת������
    * input         : strPuInfo ����id
    *                 strHiIp  ת��ģ��ip
    *                 nHiPort  ת��ģ��˿�
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int sendStopTransReq(const std::string &strPuInfo, const std::string &strHiIp, int nHiPort);

    /**************************************************************************
    * name          : sendReconnect
    * description   : ������������
    * input         : strHiIp  ת��ģ��ip
    *                 nHiPort  ת��ģ��˿�
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int sendReconnect(const std::string &strHiIp, int nHiPort);

private:
    int m_fd2Vtdu;         //vtdu��hiת��ģ��ͨ��socket
    fHiEventCB m_CBFunc;   //��Ϣ�ص�����
    void *m_pEventCBParam; //�ϲ���
};

