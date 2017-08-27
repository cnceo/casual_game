/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-28 18:46 +0800
 *
 * Description:  游戏配置
 */


#ifndef LOBBY_GAME_CONFIG_H
#define LOBBY_GAME_CONFIG_H


#include "componet/exception.h" 
#include "componet/datetime.h"

#include <map>
#include <vector>
#include <set>


namespace lobby{


DEFINE_EXCEPTION(LoadGameCfgFailedGW, water::componet::ExceptionBase);

struct GameConfigData
{
    struct
    {
        std::string wechat1;
        std::string wechat2;
        std::string wechat3;
    } customService;

    std::map<uint32_t, int32_t> pricePerPlayer; //<房间局数, 人头费>

//    std::set<uint32_t> allRoomSize; //所有的房间可能大小
    struct
    {
        uint32_t index = -1;
        std::vector<std::vector<uint16_t>> decks;
    } testDeck;

    struct
    {
        water::componet::TimePoint begin;
        water::componet::TimePoint end;
        int32_t   awardMoney = 0;
    } shareByWeChat;
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

