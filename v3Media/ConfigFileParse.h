#pragma once
#include "CommDef.h"
#include <string>
#include <vector>
#include <mutex>
#include <map>

class ConfigFileParse
{
public:
    ConfigFileParse();
    virtual ~ConfigFileParse();
    int getCfg(const std::string &strCfgFile, ConfigSipServer& stConfigSipServer);
};

