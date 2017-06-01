#include "room.h"



class Game13
{
    enum class Status
    {
        creatRoom,
        waitPlayers,
        playing,
        settleResult,
    };
    enum AttrValue
    {
        GP_NORMAL,      //玩法，普通
        GP_QING_YI_SE,  //玩法，清一色

        //    ROUND_BO8,      //局数，1
        //    ROUND_BO12,     //局数，2
        //    ROUND_BO16,     //局数，4
        PAY_OWNER,      //支付，房主
        PAY_SHARE_EQU,  //支付，均摊
        PAY_WINNER,     //支付，赢家, 这个较为复杂，暂不支持
        DQ_3_DAO,       //打枪，3道
        DQ_SHUANG_BEI,  //打枪，双倍
        YTL1,//一条龙1
        YTL2,//一条龙2
        YTL4,//一条龙4
    }
private:
    bool enterRoom(RoomId, Client::Ptr);
    bool leaveRoom(RoomId, Client::Ptr);
    bool settlement(RoomId);

private:
    Room::Ptr m_room;

    struct Data
    {
        int32_t playType        = PG_NORMAL;    //玩法
        int32_t rounds          = 0;            //局数
        int32_t payment         = PAY_OWNER;    //支付方式
        int32_t daQiang         = DQ_3_DAO;     //打枪
        bool    quanLeiDa       = false;        //打枪, 全垒打
        int32_t yiTiaoLong      = 1;            //一条龙
        int32_t playerSumation  = 4;            //人数
    };
    Data m_data;
};

struct PlayCard
{
    enum class SUITS {JOKER = 0, HEART = 3, DIAMOND = 4, CLUB = 5, SPADE = 6};
    uint16_t value = 0;
};

class GameManager
{
private:
    bool createNewRoom(Client::Ptr client, GameType gameType);

public:
    registrMsgHandler();
    
private:
    std::unordered_map<RoomId, Game::Ptr> m_games;
};

