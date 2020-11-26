/***********************************************************************************
* Author     : xx.xx(xx.xx@intellif.com)
* CreateTime : 
* Copyright (c) 2019, Shenzhen Intellifusion Technologies Co., Ltd.
* File Desc  : what?（是什么） why(为什么这样设计，设计思想) demo(对于一些公共模块，需提供使用示例)
***********************************************************************************/

#ifndef _LOGMANAGER_H
#define _LOGMANAGER_H
#include <atomic>
#include <vector>
#include <mutex>
#include <condition_variable>
#include "log.h"
#include "CommDef.h"
#include "log4cplus/logger.h"
#include "log4cplus/configurator.h"
#include "log4cplus/helpers/stringhelper.h"
#include "log4cplus/loggingmacros.h"
#include "log4cplus/fileappender.h"
using namespace log4cplus;

class CLogManager
{
public:
	static CLogManager *getInstance();

	// 设置日志的配置文件，如果不设置，则使用默认配置
	if_int setLogConfigFile(const std::string strConfigFile);

	// 设置日志级别
	void setLogLevel(if_int iLogLevel);

	// 设置过滤关键字
	void setFilterWord(const std::string strKeyWord);

	// 写日志
	void writeLog(const std::string &strInfo, if_int iLogLevel = iflog::E_LOG_LEVEL_INFO);

private:
	CLogManager();

	// 初始化
	void init();

	// 初始化日志配置
	bool initConfig();

private:
	const std::string			m_strDefaultConfig = "log4cplus.cfg";
	if_int						m_iLogLevel = iflog::E_LOG_LEVEL_INFO;			// 日志级别
	Logger						m_Logger;							// 日志的模块类
	std::string					m_strConfigFile;						// 配置文件名称
	std::mutex					m_mutexHandleLock;						// 句柄的锁
	std::string					m_strKeyWord;

	std::string					m_strConfieFileText = \
		"/*\n"
		"*/\n"
		"#Default level is DEBUG, but you can use others in your favor\n"
		"log4cplus.rootLogger=DEBUG,ALL_MSGS,ERROR_MSGS,FATAL_MSGS,D\n\n\n"
		"log4cplus.appender.D=log4cplus::ConsoleAppender\n"
		"log4cplus.appender.D.layout=log4cplus::PatternLayout\n"
		"log4cplus.appender.D.layout.ConversionPattern=[%D{%Y-%m-%d %H:%M:%S,%Q}] [%t] %-5p %m%n\n"		
		"log4cplus.appender.ALL_MSGS=log4cplus::RollingFileAppender\n"
		"log4cplus.appender.ALL_MSGS.MaxFileSize=10MB\n"
		"log4cplus.appender.ALL_MSGS.CreateDirs=true\n"
		"log4cplus.appender.ALL_MSGS.MaxBackupIndex=30\n"
		"log4cplus.appender.ALL_MSGS.File=logs/IFaasTools.log\n"
		"log4cplus.appender.ALL_MSGS.layout=log4cplus::PatternLayout\n"
		"log4cplus.appender.ALL_MSGS.layout.ConversionPattern=[%D{%Y-%m-%d %H:%M:%S,%Q}] [%t] %-5p %m%n\n\n"
		"#Range\n"		
		"log4cplus.appender.ALL_MSGS.filters.1=log4cplus::spi::LogLevelRangeFilter\n"
		"log4cplus.appender.ALL_MSGS.filters.1.LogLevelMin=TRACE\n"
		"log4cplus.appender.ALL_MSGS.filters.1.LogLevelMax=FATAL\n"
		"log4cplus.appender.ALL_MSGS.filters.1.AcceptOnMatch=true\n"
		"log4cplus.appender.ALL_MSGS.filters.2=log4cplus::spi::DenyAllFilter\n\n"
		"log4cplus.appender.ERROR_MSGS=log4cplus::RollingFileAppender\n"
		"log4cplus.appender.ERROR_MSGS.MaxFileSize=10MB\n"
		"log4cplus.appender.ERROR_MSGS.CreateDirs=true\n"
		"log4cplus.appender.ERROR_MSGS.MaxBackupIndex=10\n"
		"log4cplus.appender.ERROR_MSGS.File=logs/IFaasTools.error\n"
		"log4cplus.appender.ERROR_MSGS.layout=log4cplus::PatternLayout\n"
		"log4cplus.appender.ERROR_MSGS.layout.ConversionPattern=[%D{%Y-%m-%d %H:%M:%S,%Q}] [%t] %-5p %m%n\n"
		"#Match\n"
		"log4cplus.appender.ERROR_MSGS.filters.1=log4cplus::spi::LogLevelMatchFilter\n"
		"log4cplus.appender.ERROR_MSGS.filters.1.LogLevelToMatch=ERROR\n"
		"log4cplus.appender.ERROR_MSGS.filters.1.AcceptOnMatch=true\n"
		"log4cplus.appender.ERROR_MSGS.filters.2=log4cplus::spi::DenyAllFilter\n"
		"log4cplus.appender.FATAL_MSGS=log4cplus::RollingFileAppender\n"
		"log4cplus.appender.FATAL_MSGS.MaxFileSize=10MB\n"
		"log4cplus.appender.FATAL_MSGS.CreateDirs=true\n"
		"log4cplus.appender.FATAL_MSGS.MaxBackupIndex=10\n"
		"log4cplus.appender.FATAL_MSGS.File=logs/IFaasTools.fatal\n"
		"log4cplus.appender.FATAL_MSGS.layout=log4cplus::PatternLayout\n"
		"log4cplus.appender.FATAL_MSGS.layout.ConversionPattern=[%D{%Y-%m-%d %H:%M:%S,%Q}] [%t] %-5p %m%n\n"
		"log4cplus.appender.FATAL_MSGS.filters.1=log4cplus::spi::LogLevelMatchFilter\n"
		"log4cplus.appender.FATAL_MSGS.filters.1.LogLevelToMatch=FATAL\n"
		"log4cplus.appender.FATAL_MSGS.filters.1.AcceptOnMatch=true\n"
		"log4cplus.appender.FATAL_MSGS.filters.2=log4cplus::spi::DenyAllFilter\n";
};

#endif