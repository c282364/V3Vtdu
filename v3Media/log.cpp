#include "log.h"
#include "LogManager.h"

// 设置日志的配置文件，如果不设置，则使用默认配置
void iflog::setLogConfigFile(const std::string strFileConfig)
{
	CLogManager::getInstance()->setLogConfigFile(strFileConfig);
}

// 设置日志级别
void iflog::setLogLevel(int iLogLevel)
{
    CLogManager::getInstance()->setLogLevel(iLogLevel);
}

// 设置过滤关键字
void iflog::setFilterWord(const std::string strKeyWord)
{
    CLogManager::getInstance()->setFilterWord(strKeyWord);
}

// 写日志
void iflog::writeLog(const std::string &strInfo, int iLogLevel)
{
    CLogManager::getInstance()->writeLog(strInfo, iLogLevel);
}