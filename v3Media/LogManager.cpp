#include <cstdlib>
#include <stdio.h>

#ifdef WIN32
#include <io.h>
#else
#include <unistd.h>

#define _access access
#endif

#include "LogManager.h"

CLogManager::CLogManager()
{
}

CLogManager *CLogManager::getInstance()
{
	static CLogManager *manager = NULL;
	if (NULL == manager)
	{
		manager = new CLogManager();
		manager->init();
	}
	return manager;
}

void CLogManager::init()
{
	Logger root = Logger::getRoot();
	m_Logger = Logger::getInstance(LOG4CPLUS_TEXT("intellif"));
}

// 初始化日志配置
bool CLogManager::initConfig()
{
	static bool isInit = false;
	if (isInit == true)
	{
		return isInit;
	}

	// 如果没有默认配置，则写入默认配置
	if (m_strConfigFile.empty() || _access(m_strConfigFile.c_str(), 0) != 0)
	{
		m_strConfigFile = m_strDefaultConfig;

		if (_access(m_strConfigFile.c_str(), 0) != 0)
		{
			FILE *fp = fopen(m_strConfigFile.c_str(), "w+");
			if (fp == NULL)
			{
				printf("create log config %s fail....\n", m_strConfigFile.c_str());
				return false;
			}
			fwrite(m_strConfieFileText.c_str(), 1, m_strConfieFileText.size(), fp);
			fclose(fp);
			fp = NULL;
		}
	}

	PropertyConfigurator::doConfigure(LOG4CPLUS_TEXT(m_strConfigFile));

	isInit = true;
	return isInit;
}

// 设置日志的配置文件，如果不设置，则使用默认配置
if_int CLogManager::setLogConfigFile(const std::string strConfigFile)
{
	if (!m_strConfigFile.empty() || strConfigFile.empty())
	{
		return -1;
	}
	m_strConfigFile = strConfigFile;

	initConfig();
	return 0;
}

// 设置日志级别
void CLogManager::setLogLevel(if_int iLogLevel)
{
	m_iLogLevel = iLogLevel;
}

// 设置过滤关键字
void CLogManager::setFilterWord(const std::string strKeyWord)
{
	m_strKeyWord = strKeyWord;
}

// 写日志
void CLogManager::writeLog(const std::string &strInfo, if_int iLogLevel)
{
	if (m_strKeyWord.empty())		// 没有设置关键字
	{
		// 级别较低的日志，不写入
		if (iLogLevel < m_iLogLevel)
		{
			return;
		}	
	}
	else	// 设置了关键字，根据关键字过滤
	{
		if (strInfo.find(m_strKeyWord) <= 0 && iLogLevel < m_iLogLevel)
		{
			return;
		}
	}
	
	// 配置日志
	if (!initConfig())
	{
		printf("write log fail, init log conf fail, log info:%s\n", strInfo.c_str());
		return;
	}

	// 写入日志
	switch (iLogLevel)
	{
	case iflog::E_LOG_LEVEL_VERBOSE:
		LOG4CPLUS_TRACE(m_Logger, strInfo);
		break;
	case iflog::E_LOG_LEVEL_DEBUG:
		LOG4CPLUS_DEBUG(m_Logger, strInfo);
		break;
	case iflog::E_LOG_LEVEL_INFO:
		LOG4CPLUS_INFO(m_Logger, strInfo);
		break;
	case iflog::E_LOG_LEVEL_ERROR:
		LOG4CPLUS_ERROR(m_Logger, strInfo);
		break;
    case iflog::E_LOG_LEVEL_FATAL:
        LOG4CPLUS_FATAL(m_Logger, strInfo);
        break;
	default:
		LOG4CPLUS_INFO(m_Logger, strInfo);
		break;
	}
}
