/**   @file    CommDef.cpp
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
#include "CommDef.h"

/*��ȡϵͳ���������ĺ�����*/
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

/*��ȡ1970������ǰʱ��ĺ�����*/
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

/*��ȡ1970������ǰʱ�������*/
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