// v3Media.cpp : 定义控制台应用程序的入口点。
//

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
#include "GB28181Stream.h"
#include "SipV3.h"
#include "communicationHi.h"
#include "XmlParse.h"
#include "VtduServer.h"

#define ENABLE_MAIN_SOCKET


int _tmain(int argc, _TCHAR* argv[])
{
#ifdef WIN32
    WSADATA wsd;
    int err;
    err = WSAStartup(MAKEWORD(2, 2), &wsd);
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

    do
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
    } while (1);

	return 0;
}

