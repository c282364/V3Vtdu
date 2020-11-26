#ifndef _LOG_H
#define _LOG_H
#include <sstream>
#include "CommDef.h"

/************************************************************************/
/* 日志模块中的相关信息                                                  */
/************************************************************************/
#define LOG_MODULE "VTDU"

// 正常的打印，有模块名(推荐)
#define INTELLIF_LOG_I(moduleName, info)  {std::stringstream tmpInfo; tmpInfo << "["<< moduleName << "] - " << __FILE__ << ":" << __LINE__ << " " << info;iflog::writeLog(tmpInfo.str(), iflog::E_LOG_LEVEL_INFO);}
#define INTELLIF_LOG_W(moduleName, info)  {std::stringstream tmpInfo; tmpInfo << "["<< moduleName << "] - " << __FILE__ << ":" << __LINE__ << " " << info;iflog::writeLog(tmpInfo.str(), iflog::E_LOG_LEVEL_WARN);}
#define INTELLIF_LOG_E(moduleName, info)  {std::stringstream tmpInfo; tmpInfo << "["<< moduleName << "] - " << __FILE__ << ":" << __LINE__ << " " << info;iflog::writeLog(tmpInfo.str(), iflog::E_LOG_LEVEL_ERROR);}
#define INTELLIF_LOG_D(moduleName, info)  {std::stringstream tmpInfo; tmpInfo << "["<< moduleName << "] - " << __FILE__ << ":" << __LINE__ << " " << info;iflog::writeLog(tmpInfo.str(), iflog::E_LOG_LEVEL_DEBUG);}
#define INTELLIF_LOG_V(moduleName, info)  {std::stringstream tmpInfo; tmpInfo << "["<< moduleName << "] - " << __FILE__ << ":" << __LINE__ << " " << info;iflog::writeLog(tmpInfo.str(), iflog::E_LOG_LEVEL_VERBOSE);}
#define INTELLIF_LOG_F(moduleName, info)  {std::stringstream tmpInfo; tmpInfo << "["<< moduleName << "] - " << __FILE__ << ":" << __LINE__ << " " << info;iflog::writeLog(tmpInfo.str(), iflog::E_LOG_LEVEL_FATAL);}

#define VTDU_LOG_I(info)  INTELLIF_LOG_I(LOG_MODULE, info)
#define VTDU_LOG_W(info)  INTELLIF_LOG_W(LOG_MODULE, info)
#define VTDU_LOG_E(info)  INTELLIF_LOG_E(LOG_MODULE, info)
#define VTDU_LOG_D(info)  INTELLIF_LOG_D(LOG_MODULE, info)
#define VTDU_LOG_V(info)  INTELLIF_LOG_V(LOG_MODULE, info)
#define VTDU_LOG_F(info)  INTELLIF_LOG_V(LOG_MODULE, info)


namespace iflog
{
	enum ELogLevel
	{
		E_LOG_LEVEL_VERBOSE = 0,		// 详细的日志级
		E_LOG_LEVEL_DEBUG = 10,		// debug级别的日
		E_LOG_LEVEL_INFO = 20,		// info级别的打
        E_LOG_LEVEL_WARN = 30,		// warn级别的打
        E_LOG_LEVEL_ERROR = 40,		// error 级别的打
        E_LOG_LEVEL_FATAL = 50,       // 致命的错误
	};

	// 设置日志的配置文件，如果不设置，则使用默认配
	void setLogConfigFile(const std::string strFileConfig);

	// 设置日志级别
	void setLogLevel(int iLogLevel);

	// 设置过滤关键
	void setFilterWord(const std::string strKeyWord);

	// 写日志
	void writeLog(const std::string &strInfo, int iLogLevel = E_LOG_LEVEL_INFO);
}

#endif
