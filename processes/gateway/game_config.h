/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-28 18:46 +0800
 *
 * Description:  游戏配置
 */


#ifndef GATEWAY_GAME_CONFIG_H
#define GATEWAY_GAME_CONFIG_H


#include "componet/exception.h" 

#include <map>
#include <vector>
#include <set>


namespace gateway{


DEFINE_EXCEPTION(LoadGameCfgFailedGW, water::componet::ExceptionBase);

struct GameConfigData
{
    struct
    {
        std::string version;
        bool appleReview = false;
        bool strictVersion = false;
    } versionInfo;

    struct
    {
        std::string wechat1;
        std::string wechat2;
        std::string wechat3;
    } customService;

    struct
    {
        std::vector<std::string> texts;
        time_t internalSec = 120;
    } systemNotice;

    std::map<uint32_t, int32_t> pricePerPlayer; //<房间局数, 人头费>

};

class GameConfig
{
public:
    void load(const std::string& cfgDir);
    void reload(const std::string& cfgDir);

    const GameConfigData& data() const;

private:
    GameConfig() = default;

    GameConfigData m_data;

    static GameConfig s_me;
public:
    static GameConfig& me();
};

inline const GameConfigData& GameConfig::data() const
{
    return m_data;
}


}

#endif

