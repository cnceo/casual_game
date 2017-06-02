#include "room.h"


class GameManager
{
private:
    bool createNewRoom(Client::Ptr client, GameType gameType);

public:
    registMsgHandler();
    
private:
    std::unordered_map<RoomId, Game::Ptr> m_games;
};

bool GameManager::createNewRoom(Client::Ptr client, GameType gameType)
{
    switch (gameType)
    {
    case GameType::xm13:
        {
            Room::Ptr = Room::create(
        }
        break;
    default:
        return false;
    }
}


class Game13
{
    friend Room::Ptr GameManager::createNewRoom(Client::Ptr, GameType);
    enum class Status
    {
        creatRoom,
        waitPlayers,
        playing,
        settleResult,
    };

    struct PlayCard
    {
        enum Suits {SPADE = 3, HEART = 2, CLUB = 1, DIAMOND = 0};
        uint8_t deck[65];
    };

    enum AttrValue
    {
        GP_52,                  //玩法，普通
        GP_65,                  //玩法，多一色

        //    ROUND_BO8,      //局数，1
        //    ROUND_BO12,     //局数，2
        //    ROUND_BO16,     //局数，4
        PAY_BANKER,     //支付，庄家
        PAY_SHARE_EQU,  //支付，均摊
        PAY_WINNER,     //支付，赢家, 这个较为复杂，暂不支持
        DQ_3_DAO,       //打枪，3道
        DQ_SHUANG_BEI,  //打枪，双倍
        YTL1,//一条龙1
        YTL2,//一条龙2
        YTL4,//一条龙4
    }
public:
    Game13(Room::Ptr room, AttrValue* attr)
    {
    }

private:
    bool enterRoom(RoomId, Client::Ptr);
    bool leaveRoom(RoomId, Client::Ptr);
    bool settlement(RoomId);
    void shuffle();
    void deal();
    void 

private:
    Room::Ptr m_room;

    struct Data
    {
        int32_t playType    = PG_52;        //玩法
        int32_t rounds      = 0;            //局数
        int32_t payment     = PAY_BANKER;   //支付方式
        int32_t daQiang     = DQ_3_DAO;     //打枪
        bool    quanLeiDa   = false;        //打枪, 全垒打
        int32_t yiTiaoLong  = 1;            //一条龙
        int32_t playerSize  = 4;            //人数
    };
    Data m_data;
};

