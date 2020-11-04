/**   @file    CommDef.h
*    @note    All Right Reserved.
*    @brief   ���������ļ�
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
#include <time.h>
#include <string.h>
#ifdef WIN32
#include <Windows.h>
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
//****************************ö�ٶ���***************************************

//��Ƶ��������
enum TASK_type
{
    PREVIEW,  //Ԥ��
    PLAYBACK  //�ط�
};

//����ģ������
enum CUTOFF_type
{
    V3,     //v3ƽ̨������豸����
    HiTrans //ת��ģ�����
};

//������
enum Stream_type
{
    UDP_PS,     //PS UDP
    TCP_PS_Initiative, // ps tcp ��������
    TCP_PS_Passive, // ps tcp ������
    UDP_TS,     //TS UDP
    TCP_TS_Initiative, // TS tcp ��������
    TCP_TS_Passive // TS tcp ������
};

//sip�ص���Ϣ����
typedef enum tagESipUAMsg
{
    SIPUA_MSG_UNKNOWN = 0,
    /*ע������Ϣ*/
    SIPUA_MSG_REGISTER_NEW,           /*�յ�ע����Ϣ����ӦEXOSIP_REGISTRATION_NEW����������*/
    SIPUA_MSG_REGISTER_SUCCESS,       /*ע��ɹ�*/
    SIPUA_MSG_REGISTER_FAILURE,       /*ע��ʧ��*/
    SIPUA_MSG_REGISTER_REFRESH,       /*ˢ��ע��, ��ӦEXOSIP_REGISTRATION_REFRESHED��Ϣ����������*/

    /*��������Ϣ*/
    SIPUA_MSG_CALL_INVITE_INCOMING,   /*�յ�INVITE */
    SIPUA_MSG_CALL_ANSWER_200OK,      /*200 ok��Ϣ*/
    SIPUA_MSG_CALL_ANSWER_RSP,        /*������200��Ӧ*/
    SIPUA_MSG_CALL_ACK,               /*ack*/
    SIPUA_MSG_CALL_BYE,               /*bye*/

    /*MESSAGE��Ϣ*/
    SIPUA_MSG_MESSAGE,                //�յ�MESSAGE��Ϣ
    SIPUA_MSG_MESSAGE_ANSWERED,       //�յ�MESSAGE��Ϣ��200 ok��Ӧ
    SIPUA_MSG_MESSAGE_REQUESTFAILURE, //�յ�MESSAGE��Ϣ�Ĵ�����Ӧ

    /*INFO��Ϣ*/
    SIPUA_MSG_INFO,                   //�յ�INFO��Ϣ
    SIPUA_MSG_INFO_ANSWERED,          //�յ�INFO��Ϣ��200 ok��Ӧ
    SIPUA_MSG_INFO_REQUESTFAILURE,    //�յ�INFO��Ϣ�Ĵ�����Ӧ

    /*OPTION��Ϣ*/
    SIPUA_MSG_OPTION,                 //�յ�OPTION��Ϣ
    SIPUA_MSG_OPTION_ANSWERED,        //�յ�OPTION��Ϣ��200 ok��Ӧ
    SIPUA_MSG_OPTION_REQUESTFAILURE,  //�յ�OPTION��Ϣ�Ĵ�����Ӧ
}ESipUAMsg;

//****************************�ṹ����***************************************

//sip ������Ϣ
typedef struct tagConfigSipServer
{
    std::string m_strCfgFile; //�����ļ�·��

    /*����SIP���ò���*/
    std::string m_strSipAddr;
    int m_iSipPort;
    std::string m_strLocalSipId; //"34020100002000000001";
    std::string m_strSipRegion;
    std::string m_strSipDomain;
    std::string m_strPwd;       //"123456";
    int m_iExpires;
    int m_iMonitorKeepalive; //�����¼�ƽ̨��keepalive��ʱʱ��

    /*Զ��SIP���������ò���*/
    std::string m_strServerId;
    std::string m_strServerRegion;
    std::string m_strServerDomain;
    std::string m_strServerIP; //"192.168.2.5"; //
    int m_iServerPort;

    //vtdu��ת��ģ��ͨ�Ŷ˿�
    int m_iVtdu2HiPort;

    //vtdu ��Ƶ���շ��˿ڷ�Χ
    int m_nRecvV3PortBegin;
    int m_nRecvV3PortEnd;
    int m_nSendV3PortBegin;
    int m_nSendV3PortEnd;
}ConfigSipServer;

//ͼ������
typedef struct Ratio
{
    int nWide;
    int nHigh;
}stRatio;

//��Ƶ������Ϣ
typedef struct MediaInfo
{
    //���շ���Ϣ
    std::string  strCuSession; //����Session
    std::string  strCuUserID;  //�����û�id
    std::string  strCuIp;      //����ip
    int nCuport;               //���ն˿�
    std::string strCuPort;     //���ն˿�
    int nCuNat;
    int nCuTranmode;
    int nCuTransType;
    int nCuUserType;
    int nMediaType;
    std::string strCuGBVideoTransMode;
    int nMSCheck;
    //�����ͷ���Ϣ
    std::string strPUID;
    int nPuChannelID;
    int nPuProtocol;
    int nPuStreamType;
    int nPuTransType;
    int nPuFCode;
    int nOpenVideoStreamType;
    std::string strOpenVideoGBVideoTransMode;
    //vtdu������Ϣ
    std::string strVtduRecvIp; //����v3ƽ̨��ip
    std::string strVtduSendIP; //vtdu�������˿�
    int nVtduRecvPort;         //vtdu�������˿�
    int nVtduSendPort;         //vtdu�������˿�
    bool bTrans;               //��Ƶת����Ϣ       
}stMediaInfo;

//ת��ģ����Ϣ
typedef struct HiInfo
{
    std::string  strSipId;
    std::string  strSipRegion;
    std::string  strSipIp;
    int nSipPort;
    int nTansTaskNum;
    int nMaxTransTaskNum;
    long long nHeartBeat;
}stHiInfo;

//ת��������Ϣ
typedef struct HiTaskInfo
{
    void* pStream; //��������
    Stream_type enStreamType;
    std::string strHiSipId; //ת��ģ��id
    int nRecvPort;          //���ն˿�
    std::string strRecvIp;  //����ip
    stMediaInfo stMedia;    //ý����Ϣ
    TASK_type enTaskType; //0 Ԥ�� 1�ط�
}stHiTaskInfo;

//�����ϱ���Ϣ
typedef struct CutOffInfo
{
    std::string strCutOffInfo; //����id
    CUTOFF_type enCutOffType; //0 v3���� 2 hiģ�����
}stCutOffInfo;

//****************************�ص���������***************************************
/*SIPUA�¼��ص�����
@msg: ��Ϣ���Ͳο�ESipUAMsg
@pSdpInfo: ��SDP�ַ����������Ľṹ�壬INVITE��Ϣ�������
�����Ǻ��������ϢΪNULL
@pMsgPtr: ָ����Ϣ���ݵ�ַ��ָ��
@pszBody/iBodyLen: ��Ϣ���ַ���/����
@pParam: �ص������û�����
*/
typedef void(*fnSipUAEventCB)(ESipUAMsg msg, void *pMsgPtr, void *pParam);
typedef void(*fHiEventCB)(char *szRecvBuff, int nLen, char* szSendBuff, int* nSendBuf, void *pParam);


/*��ȡϵͳ���������ĺ�����*/
unsigned long Comm_GetTickCount();

/*��ȡ1970������ǰʱ��ĺ�����*/
long long Comm_GetMilliSecFrom1970();

/*��ȡ1970������ǰʱ�������*/
long long Comm_GetSecFrom1970();
