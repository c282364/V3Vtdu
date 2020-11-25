#pragma once
#ifdef WIN32
#include <process.h>
#include <ctime>
#include <cstdlib>
#else
#include <unistd.h>
#include <sys/time.h>
#include <pthread.h>
#include <semaphore.h>
#endif
#include <string>
#include <map>
#include <vector>
#include <mutex>
#include "CommDef.h"

//����������Ϣ
typedef struct SendClientInfo
{
    int nSendPort;                   //���Ͷ˿�
    int fdSend;                      //����socket
    struct sockaddr_in stClientAddr; //�ͻ��˵�ַ
}stSendClientInfo;

/**************************************************************************
* name          : StreamInfoCallbackFun
* description   : �ص�����
* input         : int �ϱ����� 0 v3���� 1hiģ�����
*                 std::string ��������id
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
typedef void(*StreamInfoCallbackFun)(int, std::string, void *);

/**************************************************************************
* name          : createFdUdp
* description   : ����socket
* input         : port �˿�
*                 bSend �Ƿ���Ҫ��������
*                 strIp ip
* output        : NA
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int createFdUdp(int port, bool bSend, const std::string &strIp);

/**************************************************************************
* name          : closeFd
* description   : ����socket
* input         : nFd socket���
* output        : nFd socket���
* return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
* remark        : NA
**************************************************************************/
int closeFd(int &nFd);

class Stream
{
public:
    Stream();
    virtual ~Stream();

    /**************************************************************************
    * name          : start
    * description   : ����
    * input         : bTans �Ƿ���Ҫת��
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    virtual int start(bool bTans) = 0;

    /**************************************************************************
    * name          : stop
    * description   : ֹͣ
    * input         : NA
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    virtual int stop() = 0;

    /**************************************************************************
    * name          : addOneSend
    * description   : ���һ�����շ�
    * input         : strClientIp ���շ�ip
    *                 nClientPort ���շ��˿�
    *                 strCuUserID ���շ�id
    *                 nSendPort   ���Ͷ˿�
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    virtual int addOneSend(const std::string &strClientIp, int nClientPort, const std::string &strCuUserID, int nSendPort) = 0;

    /**************************************************************************
    * name          : DelOneSend
    * description   : ɾ��һ�����շ�
    * input         : strCuUserID  ���շ�id
    *                 nCurSendPort ���Ͷ˿�
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    virtual int DelOneSend(const std::string &strCuUserID, int &nCurSendPort) = 0;

    /**************************************************************************
    * name          : getRecvPort
    * description   : ��ȡ���ն˿�
    * input         : NA
    * output        : nRecvPort ���Ͷ˿�
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    virtual int getRecvPort(int &nRecvPort) = 0;

    /**************************************************************************
    * name          : getOutstreamNum
    * description   : ��ȡ�ַ�·��
    * input         : NA
    * output        : nOutstreamNum �ַ�·��
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    virtual int getOutstreamNum(int &nOutstreamNum) = 0;

    /**************************************************************************
    * name          : setCallBack
    * description   : ���ûص������ϱ���������¼����ϲ�����
    * input         : StreamInfoCallbackFun  �����ص�����
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void setCallBack(StreamInfoCallbackFun pFuncCb, void* pPara);

    /**************************************************************************
    * name          : getSendPort
    * description   : ��ȡ���з��Ͷ˿�
    * input         : vecSendPort  ���Ͷ˿��б�
    * output        : NA
    * return        : 0��ʾ�ɹ� С����ʧ�� ����������붨��
    * remark        : NA
    **************************************************************************/
    int getSendPort(std::vector<int>& vecSendPort);

public:
    std::mutex mtSendList; //�����б���
    std::map<std::string, stSendClientInfo> m_mapSendList; //�����б�
    StreamInfoCallbackFun m_pFuncCb; //�ص�����
    void*                 m_pUserPara;
};

