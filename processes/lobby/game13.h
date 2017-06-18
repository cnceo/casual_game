#ifndef LOBBY_GAME_HPP
#define LOBBY_GAME_HPP



#include "room.h"

#include "protocol/protobuf/proto_manager.h"

#include "deck.h"

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
        vote,
        settle,  //一局结束
        settleAll, //总结算
        closed,  //所有局结束
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

private:
    CREATE_FUN_NEW(Game13);
    TYPEDEF_PTR(Game13);
    Game13(ClientUniqueId ownerCuid, uint32_t maxSize, GameType gameType);
    bool enterRoom(ClientPtr client);
    void tryStartRound();
    void trySettleGame();
    void checkAllVotes();
    void abortGame();

    void removePlayer(ClientPtr client);
    void afterEnterRoom(ClientPtr client);

    virtual void sendToAll(TcpMsgCode msgCode, const ProtoMsg& proto) override;
    virtual void sendToOthers(ClientUniqueId cuid, TcpMsgCode msgCode, const ProtoMsg& proto) override;
    virtual void timerExec(componet::TimePoint now) override;
    virtual void clientOnlineExec(ClientPtr) override;

    void syncAllPlayersInfoToAllClients(); //这个有空可以拆成 sendAllToMe和sendMeToAll, 现在懒得搞了

    struct RoundSettleData;
    std::shared_ptr<RoundSettleData> calcRound();

    const uint32_t NO_POS = -1;
    uint32_t getEmptySeatIndex();
    struct PlayerInfo;
    PlayerInfo* getPlayerInfoByCuid(ClientUniqueId cuid);

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

    struct PlayerInfo
    {
        PlayerInfo(ClientUniqueId cuid_ = 0, std::string name_ = "", int32_t status_ = 0, int32_t money_ = 0)
        :cuid(cuid_), name(name_), status(status_), money(money_)
        {
        }
        void clear()
        {
            cuid = 0;
            name.clear();
            status = 0;
            money = 0;
        }
        ClientUniqueId cuid;
        std::string name;
        int32_t status;
        int32_t money;
        std::array<Deck::Card, 13> cards;
        int32_t vote = 0;
    };
    std::vector<PlayerInfo> m_players;

    GameStatus m_status = GameStatus::prepare;
    int32_t m_rounds = 0;
    time_t m_startVoteTime = 0;
    ClientUniqueId m_voteSponsorCuid = 0;
    time_t m_settleAllTime = 0;

    struct RoundSettleData
    {
        TYPEDEF_PTR(RoundSettleData)
        CREATE_FUN_MAKE(RoundSettleData)

        struct PlayerData
        {
            ClientUniqueId cuid;
            std::string name;
            std::array<Deck::Card, 13> cards;   //所有牌
            std::array<Deck::BrandInfo, 3> dun; //3墩牌型
            Deck::G13SpecialBrand spec;         //特殊牌型
            int32_t prize = 0;
            std::map<ClientUniqueId, std::array<int32_t, 2>> losers; //<loser, <price, 打枪>>
        };
        std::vector<PlayerData> players;
    };
    std::vector<RoundSettleData::Ptr> m_settleData;

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
