/***********************************************************************************
 * Author     : 唐湘楠 (tang.xiangnan@intellif.com)
 * CreateTime : 2019/4/25
 * Copyright (c) 2019  Shenzhen Intellifusion Technologies Co., Ltd.
 * File Desc  : 容器类型重定义
***********************************************************************************/

#ifndef _IFCONTAINER_H
#define _IFCONTAINER_H

#include <vector>
#include <list>
#include <set>
#include <map>
#include <string>
#include <unordered_map>
#include <utility>

//vector
template <typename T>
using if_vector = std::vector<T>;

//list
template <typename T>
using if_list = std::list<T>;

//set
template <typename T>
using if_set = std::set<T>;

//map
template <typename TK, typename TV>
using if_map = std::map<TK, TV>;

template <typename TK, typename TV>
using if_unordered_map = std::unordered_map<TK, TV>;

template <typename T1, typename T2>
using if_pair = std::pair<T1, T2>;

#endif
