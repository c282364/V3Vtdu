// v3Media.cpp : 定义控制台应用程序的入口点。
//

#include "stdafx.h"
#include <iostream>
#include <eXosip2/eXosip.h>
#include <stdio.h>
#include <stdlib.h>
#ifdef WIN32
#include <process.h>
#else
#include <pthread.h>
#endif
#include "tinyxml.h"
#include <string>
#include <vector>
#include <mutex>
#include <map>
#include "GBRtpPsStream.h"
#include "SipV3.h"
#include "communicationHi.h"
#include "XmlParse.h"
#include "VtduServer.h"

#define ENABLE_MAIN_SOCKET


int _tmain(int argc, _TCHAR* argv[])
{
#ifdef RTP_SOCKETTYPE_WINSOCK
    WSADATA wsd;
    WSAStartup(MAKEWORD(2, 2), &wsd);
#endif // WIN32

    VtduServer oVtduServer;
    int nRet = oVtduServer.Init("VtduServer.ini");
    if (nRet < 0)
    {
        return -1;
    }

    nRet = oVtduServer.Start();
    if (nRet < 0)
    {
        oVtduServer.Fini();
        return -1;
    }

    std::string strCuUserID = "10086";
    int nCuport = 20000;

    std::string strPUID = "10087";

    int nPuChannelID = 1;
    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        //test 发送任务 推流
        if (0)
        {
            for (int i = 0; i < 1; i++)
            {
                stMediaInfo stCurMediaInfo;
                stCurMediaInfo.strCuUserID = strCuUserID; //ch
                strCuUserID = strCuUserID + "0";
                stCurMediaInfo.strCuIp = "127.0.0.1";
                stCurMediaInfo.nCuport = nCuport; //ch
                nCuport += 1;
                stCurMediaInfo.nCuNat = 1;
                stCurMediaInfo.nCuTransType = 1;
                stCurMediaInfo.nCuUserType = 1;
                stCurMediaInfo.strCuGBVideoTransMode = "1";
                stCurMediaInfo.nMSCheck = 0;

                stCurMediaInfo.strPUID = strPUID; //ch
                strPUID = strPUID + "0";
                stCurMediaInfo.nPuChannelID = nPuChannelID; //ch
                nPuChannelID += 1;
                stCurMediaInfo.nPuProtocol = 1;
                stCurMediaInfo.nPuStreamType = 0;
                stCurMediaInfo.nPuTransType = 0;
                stCurMediaInfo.nPuFCode = 1;
                stCurMediaInfo.strVtduRecvIp = "127.0.0.1";
                stCurMediaInfo.strVtduSendIP = "127.0.0.1";
                stCurMediaInfo.nOpenVideoStreamType = 0;
                stCurMediaInfo.strOpenVideoGBVideoTransMode = "UDP";

                stCurMediaInfo.bTrans = false;
                oVtduServer.sipServerHandleV3TransReadyTest(stCurMediaInfo);
                //Sleep(500);
            }
        }
    } while (1);

	return 0;
}

