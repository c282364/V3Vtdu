#include "ConfigFileParse.h"
#include "inifile.h"
using namespace inifile;

ConfigFileParse::ConfigFileParse()
{

}

ConfigFileParse::~ConfigFileParse()
{

}

int ConfigFileParse::getCfg(const std::string &strCfgFile, ConfigSipServer& stConfigSipServer)
{
    int nRet = -1;
    inifile::IniFile *pCfgFile;
    pCfgFile = new(std::nothrow) IniFile();
    if (NULL == pCfgFile)
    {
        VTDU_LOG_E("new IniFile failed");
        return nRet;
    }

    //句柄多线程调用注意加锁
    int nRetOpen = pCfgFile->openini(strCfgFile, IFACE_INI_PARAM_TYPE_NAME);
    if (nRetOpen != 0)
    {
        VTDU_LOG_E("open ini file failed: " << strCfgFile);
        delete pCfgFile;
        pCfgFile = NULL;
        return nRet;
    }

    do 
    {
        //本地sip配置
//sipid
        int dwRet = -1;
        if (pCfgFile->hasKey("local", "sipid"))
        {
            stConfigSipServer.m_strLocalSipId = pCfgFile->getStringValue("local", "sipid", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no local sipid");
            break;
        }

        //sip region
        if (pCfgFile->hasKey("local", "region"))
        {
            stConfigSipServer.m_strSipRegion = pCfgFile->getStringValue("local", "region", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no local region");
            break;
        }

        //sip domain
        if (pCfgFile->hasKey("local", "domain"))
        {
            stConfigSipServer.m_strSipDomain = pCfgFile->getStringValue("local", "domain", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no local domain");
            break;
        }

        //sip sipaddr
        if (pCfgFile->hasKey("local", "sipaddr"))
        {
            stConfigSipServer.m_strSipAddr = pCfgFile->getStringValue("local", "sipaddr", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no local sipaddr");
            break;
        }

        //sip sipport
        if (pCfgFile->hasKey("local", "sipport"))
        {
            stConfigSipServer.m_iSipPort = pCfgFile->getIntValue("local", "sipport", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no local sipport");
            break;
        }

        //sip Expires
        stConfigSipServer.m_iExpires = 3600;
        if (pCfgFile->hasKey("local", "expires"))
        {
            stConfigSipServer.m_iExpires = pCfgFile->getIntValue("local", "expires", dwRet);
        }

        //sip服务端配置
        //sipid
        if (pCfgFile->hasKey("remote", "sipid"))
        {
            stConfigSipServer.m_strServerId = pCfgFile->getStringValue("remote", "sipid", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no remote sipid");
            break;
        }

        //sip region
        if (pCfgFile->hasKey("remote", "region"))
        {
            stConfigSipServer.m_strServerRegion = pCfgFile->getStringValue("remote", "region", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no remote region");
            break;
        }

        //sip domain
        if (pCfgFile->hasKey("remote", "domain"))
        {
            stConfigSipServer.m_strServerDomain = pCfgFile->getStringValue("remote", "domain", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no remote domain");
            break;
        }

        //sipaddr
        if (pCfgFile->hasKey("remote", "sipaddr"))
        {
            stConfigSipServer.m_strServerIP = pCfgFile->getStringValue("remote", "sipaddr", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no remote sipaddr");
            break;
        }

        //sipport
        if (pCfgFile->hasKey("remote", "sipport"))
        {
            stConfigSipServer.m_iServerPort = pCfgFile->getIntValue("remote", "sipport", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no remote sipport");
            break;
        }

        //和 hi转码模块通信端口
        if (pCfgFile->hasKey("stream", "Vtdu2HiPort"))
        {
            stConfigSipServer.m_iVtdu2HiPort = pCfgFile->getIntValue("stream", "Vtdu2HiPort", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no Vtdu2HiPort");
            break;
        }

        //接收流端口范围
        if (pCfgFile->hasKey("stream", "RecvV3PortBegin"))
        {
            stConfigSipServer.m_nRecvV3PortBegin = pCfgFile->getIntValue("stream", "RecvV3PortBegin", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no RecvV3PortBegin");
            break;
        }

        if (pCfgFile->hasKey("stream", "RecvV3PortEnd"))
        {
            stConfigSipServer.m_nRecvV3PortEnd = pCfgFile->getIntValue("stream", "RecvV3PortEnd", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no RecvV3PortEnd");
            break;
        }

        //发送流端口范围
        if (pCfgFile->hasKey("stream", "SendV3PortBegin"))
        {
            stConfigSipServer.m_nSendV3PortBegin = pCfgFile->getIntValue("stream", "SendV3PortBegin", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no SendV3PortBegin");
            break;
        }

        if (pCfgFile->hasKey("stream", "SendV3PortEnd"))
        {
            stConfigSipServer.m_nSendV3PortEnd = pCfgFile->getIntValue("stream", "SendV3PortEnd", dwRet);
        }
        else
        {
            VTDU_LOG_E("ini file have no SendV3PortEnd");
            break;
        }

        nRet = 0;
    } while (0);

    delete pCfgFile;
    pCfgFile = NULL;

    return nRet;

    ////122242000100000079
    //stConfigSipServer.m_strLocalSipId = "122205106000000001";
    ////stConfigSipServer.m_strSipRegion = "1222051060";
    //stConfigSipServer.m_strSipRegion = "122205";
    //stConfigSipServer.m_strSipDomain = "122205";
    //stConfigSipServer.m_strSipAddr = "33.112.211.50";
    //stConfigSipServer.m_iSipPort = 5063;
    //stConfigSipServer.m_iExpires = 3600;

    //stConfigSipServer.m_strServerId = "122205105000000001";
    ////stConfigSipServer.m_strServerRegion = "1222051060";
    //stConfigSipServer.m_strServerRegion = "122205";
    //stConfigSipServer.m_strServerDomain = "122205";
    //stConfigSipServer.m_strServerIP = "33.112.211.40";
    //stConfigSipServer.m_iServerPort = 5062;

    //stConfigSipServer.m_iVtdu2HiPort = 5065;

    //stConfigSipServer.m_nRecvV3PortBegin = 40000;
    //stConfigSipServer.m_nRecvV3PortEnd = 44998;

    //stConfigSipServer.m_nSendV3PortBegin = 45000;
    //stConfigSipServer.m_nSendV3PortEnd = 49998;

    return 0;
}

