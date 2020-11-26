/***********************************************************************************
 * Author     : 唐湘楠 (tang.xiangnan@intellif.com)
 * CreateTime : 2019/4/25
 * Copyright (c) 2019  Shenzhen Intellifusion Technologies Co., Ltd.
 * File Desc  : 基本数据类型重定义
***********************************************************************************/

#ifndef _IFDATATYPE_H
#define _IFDATATYPE_H

#include <atomic>

typedef char if_int8;
typedef unsigned char if_uint8;
typedef short if_int16;
typedef unsigned short if_uint16;
typedef long if_int32;
typedef unsigned long if_uint32;
typedef long long if_int64;
typedef unsigned long long if_uint64;
typedef int if_int;
typedef unsigned int if_uint;
typedef if_int64 if_time_t;

template <typename T>
using if_atomic_t = std::atomic<T>;

#define IF_INIT_ATOMIC ATOMIC_VAR_INIT

//#define _USE_BOOST

#ifdef _USE_BOOST
#include "boost/make_shared.hpp"
#include "boost/shared_ptr.hpp"
#include "boost/weak_ptr.hpp"
#include "boost/enable_shared_from_this.hpp"
#include "boost/lexical_cast.hpp"

#ifndef BOOST_ALL_NO_LIB
#define BOOST_ALL_NO_LIB
#endif

//shared_ptr
#define if_make_shared boost::make_shared

#define if_make_pair std::make_pair

#define if_dynamic_pointer_cast boost::dynamic_pointer_cast

#define if_enable_shared_from_this boost::enable_shared_from_this

template <typename T>
using if_shared_ptr = boost::shared_ptr<T>;

template <typename T>
using if_weak_ptr = boost::weak_ptr<T>;

#else

#include <memory>

//shared_ptr
#define if_make_shared std::make_shared

#define if_dynamic_pointer_cast std::dynamic_pointer_cast

template <typename T>
using if_shared_ptr = std::shared_ptr<T>;

template <typename T>
using if_weak_ptr = std::weak_ptr<T>;

#endif

#endif
