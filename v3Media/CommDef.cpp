/**   @file    CommDef.cpp
*    @note    All Right Reserved.
*    @brief   公共定义文件
*    @author   陈晓焱
*    @date     2020/10/29
*
*    @note    下面的note和warning为可选项目
*    @note
*    @note    历史记录：
*    @note    V0.0.1  创建
*
*    @warning 这里填写本文件相关的警告信息
*/
#include "CommDef.h"

/*获取系统开机以来的毫秒数*/
unsigned long Comm_GetTickCount()
{
#ifdef WIN32
    return  GetTickCount();
#else
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (ts.tv_sec * 1000 + ts.tv_nsec / 1000000);
#endif
}

/*获取1970年至当前时间的毫秒数*/
long long Comm_GetMilliSecFrom1970()
{
    long long i64MilliSec = 0;
#ifdef WIN32
    i64MilliSec = (long long)_time64(NULL);
    i64MilliSec = i64MilliSec * 1000;
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    i64MilliSec = (long long)(tv.tv_sec * 1000);
    i64MilliSec += tv.tv_usec / 1000;
    return i64MilliSec;
#endif
    return i64MilliSec;
}

/*获取1970年至当前时间的秒数*/
long long Comm_GetSecFrom1970()
{
    long long i64Sec = 0;
#ifdef WIN32
    i64Sec = (long long)_time64(NULL);
#else
    struct timeval tv;
    gettimeofday(&tv, NULL);
    i64Sec = (long long)(tv.tv_sec);
    return i64Sec;
#endif
    return i64Sec;
}