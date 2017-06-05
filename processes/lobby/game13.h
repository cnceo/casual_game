#ifndef LOBBY_GAME_HPP
#define LOBBY_GAME_HPP



#include "room.h"

#include "protocol/protobuf/proto_manager.h"

namespace lobby{

class Client;

class Game13 : public Room
{
//    friend Room::Ptr Game13Manager::createNewRoom(Client::Ptr, GameType);
    using ClientPtr = std::shared_ptr<Client>;

    enum class GameStatus
    {
        prepare, //建房之后
        play,    //发牌之后
        settle,  //一局结束
        closed,  //所有局结束
    };

    struct Deck
    {
        enum Suits {SPADE = 3, HEART = 2, CLUB = 1, DIAMOND = 0};
        using Card = int8_t;
        std::vector<Card> cards;
    };

    enum
    {
        GP_52 = 52,                  //玩法，普通
        GP_65 = 65,                  //玩法，多一色

        PAY_BANKER      = 10, //支付，庄家
        PAY_SHARE_EQU   = 11, //支付，均摊
        PAY_WINNER      = 12, //支付，赢家, 这个较为复杂，暂不支持
        DQ_3_DAO        = 3,  //打枪，3道
        DQ_SHUANG_BEI   = 2,  //打枪，双倍
    };

public:
    void playerOnLine(ClientPtr client);

private:
    CREATE_FUN_NEW(Game13);
    TYPEDEF_PTR(Game13);
	using Room::Room;
    bool enterRoom(ClientPtr client);
//    void leaveRoom(ClientPtr client);
    void tryStartRound();
    void trySettleGame();
    void abortGame();

    void removePlayer(ClientPtr client);
    void sendToAll(TcpMsgCode msgCode, const ProtoMsg& proto);
    void syncAllPlayersInfoToAllClients(); //这个有空可以拆成 sendAllToMe和sendMeToAll, 现在懒得搞了

    const uint32_t NO_POS = -1;
    uint32_t getEmptySeatIndex();

private:
    Room::Ptr m_room;

    struct //游戏的基本属性, 建立游戏时确定, 游戏过程中不变
    {
        RoomId  roomId      = 0;
        uint32_t playType   = GP_52;        //玩法
        int32_t rounds      = 0;            //局数
        uint32_t payor      = PAY_BANKER;   //支付方式
        uint32_t daQiang    = DQ_3_DAO;     //打枪
        bool    quanLeiDa   = false;        //打枪, 全垒打
        uint32_t yiTiaoLong = 1;            //一条龙
        uint32_t playerSize = 0;            //人数
    } m_attr;

    struct PlayInfo
    {
//        TYPEDEF_PTR(PlayInfo)
//        CREATE_FUN_NEW(PlayInfo)
        PlayInfo(ClientUniqueId cuid_ = 0, std::string name_ = "", int32_t status_ = 0, int32_t money_ = 0)
        :cuid(cuid_), name(name_), status(status_), money(money_)
        {
        }
        void clear()
        {
            cuid = 0;
            name.clear();
            status = 0;
            money = 0;
            rank = 0;
        }
        ClientUniqueId cuid;
        std::string name;
        int32_t status;
        int32_t money;
        int16_t rank = 0;
        std::array<Deck::Card, 13> cards;
    };
    std::vector<PlayInfo> m_players;

    GameStatus m_status = GameStatus::prepare;
    int32_t m_rounds = 0;

private://消息处理
    static void proto_C_G13_CreateGame(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_JionGame(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_GiveUp(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_VoteFoAbortGame(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_ReadyFlag(ProtoMsgPtr proto, ClientConnectionId ccid);
    static void proto_C_G13_BringOut(ProtoMsgPtr proto, ClientConnectionId ccid);

public:
    static Game13::Ptr getByRoomId(RoomId roomId);
    static void regMsgHandler();
private:
    static Deck s_deck;
};


}

#endif
