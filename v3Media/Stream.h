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

//发送任务信息
typedef struct SendClientInfo
{
    int nSendPort;                   //发送端口
    int fdSend;                      //发送socket
    struct sockaddr_in stClientAddr; //客户端地址
}stSendClientInfo;

/**************************************************************************
* name          : StreamInfoCallbackFun
* description   : 回调函数
* input         : int 上报类型 0 v3断流 1hi模块断流
*                 std::string 断流任务id
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
typedef void(*StreamInfoCallbackFun)(int, std::string, void *);

/**************************************************************************
* name          : createFdUdp
* description   : 创建socket
* input         : port 端口
*                 bSend 是否需要发送数据
*                 strIp ip
* output        : NA
* return        : 0表示成功 小于零失败 具体见错误码定义
* remark        : NA
**************************************************************************/
int createFdUdp(int port, bool bSend, const std::string &strIp);

/**************************************************************************
* name          : closeFd
* description   : 销毁socket
* input         : nFd socket句柄
* output        : nFd socket句柄
* return        : 0表示成功 小于零失败 具体见错误码定义
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
    * description   : 启动
    * input         : bTans 是否需要转码
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    virtual int start(bool bTans) = 0;

    /**************************************************************************
    * name          : stop
    * description   : 停止
    * input         : NA
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    virtual int stop() = 0;

    /**************************************************************************
    * name          : addOneSend
    * description   : 添加一个接收方
    * input         : strClientIp 接收方ip
    *                 nClientPort 接收方端口
    *                 strCuUserID 接收方id
    *                 nSendPort   发送端口
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    virtual int addOneSend(const std::string &strClientIp, int nClientPort, const std::string &strCuUserID, int nSendPort) = 0;

    /**************************************************************************
    * name          : DelOneSend
    * description   : 删除一个接收方
    * input         : strCuUserID  接收方id
    *                 nCurSendPort 发送端口
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    virtual int DelOneSend(const std::string &strCuUserID, int &nCurSendPort) = 0;

    /**************************************************************************
    * name          : getRecvPort
    * description   : 获取接收端口
    * input         : NA
    * output        : nRecvPort 发送端口
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    virtual int getRecvPort(int &nRecvPort) = 0;

    /**************************************************************************
    * name          : getOutstreamNum
    * description   : 获取分发路数
    * input         : NA
    * output        : nOutstreamNum 分发路数
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    virtual int getOutstreamNum(int &nOutstreamNum) = 0;

    /**************************************************************************
    * name          : setCallBack
    * description   : 设置回调函数上报流处理层事件到上层管理层
    * input         : StreamInfoCallbackFun  管理层回调函数
    * output        : NA
    * return        : NA
    * remark        : NA
    **************************************************************************/
    void setCallBack(StreamInfoCallbackFun pFuncCb, void* pPara);

    /**************************************************************************
    * name          : getSendPort
    * description   : 获取所有发送端口
    * input         : vecSendPort  发送端口列表
    * output        : NA
    * return        : 0表示成功 小于零失败 具体见错误码定义
    * remark        : NA
    **************************************************************************/
    int getSendPort(std::vector<int>& vecSendPort);

public:
    std::mutex mtSendList; //发送列表锁
    std::map<std::string, stSendClientInfo> m_mapSendList; //发送列表
    StreamInfoCallbackFun m_pFuncCb; //回调函数
    void*                 m_pUserPara;
};

