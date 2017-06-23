#include "game13.h"
#include "client.h"
#include "componet/logger.h"
#include "componet/scope_guard.h"
#include "componet/tools.h"
#include "protocol/protobuf/public/client.codedef.h"


#include "permanent/proto/game13.pb.h"


#include <random>
#include <algorithm>

namespace lobby{

const time_t MAX_VOTE_DURATION = 300;

Deck Game13::s_deck;

Game13::Ptr Game13::deserialize(const std::string& bin)
{
    permanent::GameRoom protoData;
    if (!protoData.ParseFromString(bin))
        return nullptr;

    if (protoData.type() != underlying(GameType::xm13))
        return nullptr;

    auto obj = Game13::create(protoData.roomid(), protoData.owner_cuid(), GameType::xm13);

    const auto& attr = protoData.g13data().attr();
    obj->m_attr.playType   = attr.play_type();
    obj->m_attr.rounds     = attr.rounds();
    obj->m_attr.payor      = attr.payor();
    obj->m_attr.daQiang    = attr.daqiang();
    obj->m_attr.quanLeiDa  = attr.quanleida();
    obj->m_attr.yiTiaoLong = attr.yitiaolong();
    obj->m_attr.playerSize = attr.size();

    const auto& players = protoData.g13data().players();
    if (obj->m_attr.playerSize != players.size()) //protoData.g13data().players_size())
        return nullptr;
    obj->m_players.resize(attr.size());
    for (auto i = 0; i < obj->m_attr.playerSize; ++i)
    {
        obj->m_players[i].cuid   = players[i].cuid();
        obj->m_players[i].name   = players[i].name();
        obj->m_players[i].status = players[i].status();
        obj->m_players[i].vote   = players[i].vote();

        if (players[i].cards().size() != 13)
            return nullptr;
        std::copy(players[i].cards().begin(), players[i].cards().end(), obj->m_players[i].cards.begin());
    }

//    if (protoData.g13data().status() < 0 || protoData.g13data().status() > underlying(GameStatus::closed))
//        return nullptr;
    obj->m_status           = static_cast<GameStatus>(protoData.g13data().status());
    obj->m_rounds           = protoData.g13data().rounds();
    obj->m_startVoteTime    = protoData.g13data().start_vote_time();
    obj->m_voteSponsorCuid  = protoData.g13data().vote_sponsor_cuid();
    obj->m_settleAllTime    = protoData.g13data().settle_all_time();

    return obj;
}

std::string Game13::serialize(Game13::Ptr obj)
{
    permanent::GameRoom proto;
    proto.set_roomid(obj->getId());
    proto.set_roomtp(0);
    proto.set_type(underlying(GameType::xm13));
    proto.set_owner_cuid(obj->ownerCuid());
    
    auto gameData = proto.mutable_g13data();
    gameData->set_status(underlying(obj->m_status));
    gameData->set_rounds(obj->m_rounds);
    gameData->set_start_vote_time(obj->m_startVoteTime);
    gameData->set_vote_sponsor_cuid(obj->m_voteSponsorCuid);
    gameData->set_settle_all_time(obj->m_settleAllTime);

    auto attrData = gameData->mutable_attr();
    attrData->set_play_type(obj->m_attr.playType);
    attrData->set_rounds(obj->m_attr.rounds);
    attrData->set_payor(obj->m_attr.payor);
    attrData->set_daqiang(obj->m_attr.daQiang);
    attrData->set_quanleida(obj->m_attr.quanLeiDa);
    attrData->set_yitiaolong(obj->m_attr.yiTiaoLong);
    attrData->set_size(obj->m_attr.playerSize);

    for (const auto& playerInfo : obj->m_players)
    {
        auto playerData = gameData->add_players();
        playerData->set_cuid(playerInfo.cuid);
        playerData->set_name(playerInfo.name);
        playerData->set_status(playerInfo.status);
        playerData->set_vote(playerInfo.vote);
        for (auto card : playerInfo.cards)
            playerData->add_cards(card);
    }

    std::string ret;
    return proto.SerializeToString(&ret) ? ret : "";
}

Game13::Ptr Game13::getByRoomId(RoomId roomId)
{
    return std::static_pointer_cast<Game13>(Room::get(roomId));
}

void Game13::regMsgHandler()
{
    using namespace std::placeholders;
    REG_PROTO_PUBLIC(C_G13_CreateGame, std::bind(&Game13::proto_C_G13_CreateGame, _1, _2));
    REG_PROTO_PUBLIC(C_G13_JionGame, std::bind(&Game13::proto_C_G13_JionGame, _1, _2));
    REG_PROTO_PUBLIC(C_G13_GiveUp, std::bind(&Game13::proto_C_G13_GiveUp, _1, _2));
    REG_PROTO_PUBLIC(C_G13_VoteFoAbortGame, std::bind(&Game13::proto_C_G13_VoteFoAbortGame, _1, _2));
    REG_PROTO_PUBLIC(C_G13_ReadyFlag, std::bind(&Game13::proto_C_G13_ReadyFlag, _1, _2));
    REG_PROTO_PUBLIC(C_G13_BringOut, std::bind(&Game13::proto_C_G13_BringOut, _1, _2));
}

void Game13::proto_C_G13_CreateGame(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomId() != 0)
    {
        client->noticeMessageBox("已经在房间中, 无法创建新房间!");
        return;
    }

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_CreateGame, proto);
    if (false) // TODO 检查创建消息中的数据是否合法
    {
        client->noticeMessageBox("创建房间失败, 非法的房间属性设定!");
        return;
    }

    //房间
    auto game = Game13::create(client->cuid(), GameType::xm13);
    if (Room::add(game) == false)
    {
        LOG_ERROR("G13, 创建房间失败, Room::add失败");
        return;
    }

    //初始化游戏信息
    auto& attr      = game->m_attr;
//    attr.roomId     = game->getId();
    attr.playType   = rcv->play_type() > GP_52 ? GP_65 : GP_52;
    attr.rounds     = rcv->rounds();
    attr.payor      = rcv->payor();
    attr.daQiang    = rcv->da_qiang();
    attr.quanLeiDa  = rcv->quan_lei_da();
    attr.yiTiaoLong = rcv->yi_tiao_long();
    attr.playerSize = rcv->player_size();

    //依据属性检查创建资格,并初始化游戏的动态数据
    {
        LOG_DEBUG("init deck,  rcv_play_type={}", rcv->play_type());

        //玩家座位数量
        game->m_players.resize(attr.playerSize);
    }

    //最后进房间, 因为进房间要预扣款, 进入后再有什么原因失败需要回退
    game->enterRoom(client);
}

void Game13::proto_C_G13_JionGame(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_JionGame, proto);
    auto game = Game13::getByRoomId(rcv->room_id());
    if (game == nullptr)
    {
        client->noticeMessageBox("要加入的房间已解散");
        LOG_DEBUG("申请加入房间失败, 房间号不存在, ccid={}, openid={}, roomid={}", client->ccid(), client->openid(), rcv->room_id());
        return;
    }
    game->enterRoom(client);
}

void Game13::proto_C_G13_GiveUp(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomId() == 0)
    {
        client->noticeMessageBox("已经不在房间内了");
        return;
    }

    auto game = getByRoomId(client->roomId());
    if (game == nullptr)
    {
        LOG_ERROR("C_G13_GiveUp, client上记录的房间号不存在, roomId={}, ccid={}, cuid={}, openid={}", 
                  client->roomId(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
        return;

    switch (game->m_status)
    {
    case GameStatus::prepare:
        {
            if (client->cuid() == game->ownerCuid()) //房主
            {
                LOG_TRACE("准备期间房主离开房间, roomId={}, ccid={}, cuid={}, openid={}",
                          client->roomId(), client->ccid(), client->cuid(), client->openid()); 
                LOG_TRACE("准备期间终止游戏, 销毁房间, roomId={}", game->getId());
                game->abortGame();
                return;
            }
            else
            {
                LOG_TRACE("准备期间普通成员离开房间, roomId={}, ccid={}, cuid={}, openid={}",
                          game->getId(), client->ccid(), client->cuid(), client->openid()); 
               game->removePlayer(client);
               return;
            }
        }
        break;
    case GameStatus::play:
        { //临时处理， 要改成投票
            game->m_startVoteTime = componet::toUnixTime(s_timerTime);
            game->m_voteSponsorCuid = info->cuid;
            game->m_status = GameStatus::vote;
        }
        //no break
    case GameStatus::vote:
        {
            //视为赞成票
            info->vote = PublicProto::VT_AYE;
            game->checkAllVotes();
        }
        break;
    case GameStatus::settle: //结算期间, 外挂, 忽略
        break;
    case GameStatus::settleAll:
    case GameStatus::closed:
        {
            LOG_TRACE("结算完毕后主动离开房间, roomId={}, ccid={}, cuid={}, openid={}",
                      game->getId(), client->ccid(), client->cuid(), client->openid()); 
            game->removePlayer(client);
            return;
        }
    }
    return;
}

void Game13::proto_C_G13_VoteFoAbortGame(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;
    auto game = getByRoomId(client->roomId());
    if (game == nullptr)
    {
        LOG_ERROR("ReadyFlag, client上记录的房间号不存在, roomId={}, ccid={}, cuid={}, openid={}", 
                  client->roomId(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    if (game->m_status != GameStatus::vote)
        return;


    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        LOG_ERROR("ReadyFlag, 房间中没有这个玩家的信息, roomId={}, ccid={}, cuid={}, openid={}",
                  client->roomId(), client->ccid(), client->cuid(), client->openid());

        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_VoteFoAbortGame, proto);
    info->vote = rcv->vote();

    LOG_TRACE("voteForAbortGame, 收到投票, vote={}, roomId={}, ccid={}, cuid={}, openid={}",
              info->vote, client->roomId(), client->ccid(), client->cuid(), client->openid());

    if (rcv->vote() == PublicProto::VT_NONE) //弃权的不处理
        return;
    

    game->checkAllVotes();
    return;
}

void Game13::proto_C_G13_ReadyFlag(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomId() == 0)
    {
        client->noticeMessageBox("已经不在房间内了");
        return;
    }

    auto game = getByRoomId(client->roomId());
    if (game == nullptr)
    {
        LOG_ERROR("ReadyFlag, client上记录的房间号不存在, roomId={}, ccid={}, cuid={}, openid={}", 
                  client->roomId(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    if (game->m_status != GameStatus::prepare)
    {
        client->noticeMessageBox("游戏已开始");
        return;
    }

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        LOG_ERROR("ReadyFlag, 房间中没有这个玩家的信息, roomId={}, ccid={}, cuid={}, openid={}",
                  client->roomId(), client->ccid(), client->cuid(), client->openid());

        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_ReadyFlag, proto);
    auto oldStatus = info->status;
    auto newStatus = rcv->ready() ? PublicProto::S_G13_PlayersInRoom::READY : PublicProto::S_G13_PlayersInRoom::PREP;

    //状态没变, 不用处理了
    if (oldStatus == newStatus)
        return;

    //改变状态
    info->status = newStatus;
    LOG_TRACE("ReadyFlag, 玩家设置准备状态, readyFlag={}, roomId={}, ccid={}, cuid={}, openid={}",
              rcv->ready(), client->roomId(), client->ccid(), client->cuid(), client->openid());

    game->syncAllPlayersInfoToAllClients();
    /*
    PROTO_VAR_PUBLIC(S_G13_PlayersInRoom, snd)
    auto player = snd.add_players();
    player->set_status(newStatus);
    player->set_cuid(info->cuid);
    player->set_name(info->name);
    game->sendToAll(sndCode, snd);
    */

    //新的ready确认, 检查是否所有玩家都已确认, 可以启动游戏
    if (newStatus == PublicProto::S_G13_PlayersInRoom::READY)
        game->tryStartRound();
}

void Game13::proto_C_G13_BringOut(ProtoMsgPtr proto, ClientConnectionId ccid)
{
    auto client = ClientManager::me().getByCcid(ccid);
    if (client == nullptr)
        return;

    if (client->roomId() == 0)
    {
        client->noticeMessageBox("已经不在房间内了");
        return;
    }

    auto game = getByRoomId(client->roomId());
    if (game == nullptr)
    {
        LOG_ERROR("bring out, client上记录的房间号不存在, roomId={}, ccid={}, cuid={}, openid={}", 
                  client->roomId(), client->ccid(), client->cuid(), client->openid());
        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    if (game->m_status != GameStatus::play)
    {
        client->noticeMessageBox("当前不能出牌(0x01)");
        return;
    }

    //TODO 检查牌型是否合法
    if (false)
    {
        client->noticeMessageBox("牌型不合法, 请重新摆牌");
        return;
    }

    PlayerInfo* info = game->getPlayerInfoByCuid(client->cuid());
    if (info == nullptr)
    {
        LOG_ERROR("bring out, 房间中没有这个玩家的信息, roomId={}, ccid={}, cuid={}, openid={}",
                  client->roomId(), client->ccid(), client->cuid(), client->openid());

        client->afterLeaveRoom(); //ERR_HANDLER
        return;
    }

    if (info->status != PublicProto::S_G13_PlayersInRoom::SORT)
    {
         client->noticeMessageBox("当前不能出牌0x02");
         return;
    }

    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_BringOut, proto);
    if (static_cast<uint32_t>(rcv->cards_size()) != info->cards.size())
    {
        client->noticeMessageBox("出牌数量不对");
        return;
    }
    
    //改变玩家状态, 接收玩家牌型数据
    info->status = PublicProto::S_G13_PlayersInRoom::COMPARE;
    std::string cardsStr;
    cardsStr.reserve(42);
    for (uint32_t index = 0; index < info->cards.size(); ++index)
    {
        info->cards[index] = rcv->cards(index);
        cardsStr.append(std::to_string(info->cards[index]));
        cardsStr.append(",");
    }
    LOG_TRACE("bring out, roomId={}, round={}/{}, cuid={}, openid={}, cards=[{}]",
             client->roomId(), game->m_rounds, game->m_attr.rounds, client->cuid(), client->openid(), cardsStr);

    game->syncAllPlayersInfoToAllClients();

    game->trySettleGame();

    //顺利结算完毕, 开下一局
    if (game->m_status == GameStatus::settle)
        game->tryStartRound();
}


/*************************************************************************/
//
//Game13::Game13(ClientUniqueId ownerCuid, GameType gameType)
//    : Room(ownerCuid, gameType)
//{
//}

bool Game13::enterRoom(Client::Ptr client)
{
    if (client == nullptr)
        return false;

    if (client->roomId() != 0)
    {
        if (client->roomId() == getId()) //就在这个房中, 直接忽略
            return true;
        client->noticeMessageBox("已经在{}号房间中了!", client->roomId());
        return false;
    }

    if (m_status != GameStatus::prepare)
    {
        client->noticeMessageBox("进入失败, 此房间的游戏已经开始");
        return false;
    }

    const uint32_t index = getEmptySeatIndex();
    if (index == NO_POS)
    {
        client->noticeMessageBox("进入失败, 房间人数已满");
        return false;
    }

    //加入成员列表
    m_players[index].cuid = client->cuid();
    m_players[index].name = client->name();
    m_players[index].money = client->money();
    m_players[index].status = PublicProto::S_G13_PlayersInRoom::PREP;
    
    {//钱数检查
        bool enoughMoney = false;
        switch (m_attr.payor)
        {
        case PAY_BANKER:
            enoughMoney = (ownerCuid() != client->cuid()) || client->enoughMoney(10);
            break;
        case PAY_SHARE_EQU:
            enoughMoney = client->enoughMoney(5);
            break;
        case PAY_WINNER:
            enoughMoney = client->enoughMoney(10);
            break;
        default:
            return false;
        }
        if (!enoughMoney)
        {
            (ownerCuid() != client->cuid()) ? client->noticeMessageBox("创建失败, 钻石不足") : client->noticeMessageBox("进入失败, 钻石不足");
            return false;
        }
    }

    client->setRoomId(getId());

    afterEnterRoom(client);

    LOG_TRACE("G13, {}房间成功, roomId={}, ccid={}, cuid={}, openid={}", (ownerCuid() != client->cuid()) ? "进入" : "创建",
              client->roomId(), client->ccid(), client->cuid(), client->openid());
    return true;
}

void Game13::clientOnlineExec(Client::Ptr client)
{
    if (client == nullptr)
        return;
    auto info = getPlayerInfoByCuid(client->cuid());
    if (client->roomId() == getId() && info == nullptr)
    {
        LOG_TRACE("玩家上线, 房间号已被复用, roomId={}, ccid={}, cuid={}, openid={}", getId(), client->ccid(), client->cuid(), client->openid());
        client->setRoomId(0);
        return;
    }
    info->money = client->money();

    LOG_TRACE("client, sync gameinfo, roomId={}, ccid={}, cuid={}, openid={}", getId(), client->ccid(), client->cuid(), client->openid());
    afterEnterRoom(client);
}

void Game13::afterEnterRoom(ClientPtr client)
{
    if (client == nullptr)
        return;

    {//给进入者发送房间基本属性
        PROTO_VAR_PUBLIC(S_G13_RoomAttr, snd)
        snd.set_room_id(getId());
        snd.set_banker_cuid(ownerCuid());
        snd.mutable_attr()->set_player_size(m_attr.playerSize);
        snd.mutable_attr()->set_play_type(m_attr.playType);
        snd.mutable_attr()->set_rounds(m_attr.rounds);
        snd.mutable_attr()->set_payor(m_attr.payor);
        snd.mutable_attr()->set_da_qiang(m_attr.daQiang);
        snd.mutable_attr()->set_quan_lei_da(m_attr.quanLeiDa);
        snd.mutable_attr()->set_yi_tiao_long(m_attr.yiTiaoLong);
        client->sendToMe(sndCode, snd);
    }

    //给所有人发送成员列表
    syncAllPlayersInfoToAllClients();

    //给进入者发送他自己的牌信息
    PROTO_VAR_PUBLIC(S_G13_AbortGameOrNot, snd3);
    snd3.set_sponsor(m_voteSponsorCuid);
    time_t elapse = componet::toUnixTime(s_timerTime) - m_startVoteTime;
    snd3.set_remain_seconds(MAX_VOTE_DURATION > elapse ? MAX_VOTE_DURATION - elapse : 0);
    if (m_status == GameStatus::play || m_status == GameStatus::vote)
    {
        for (const PlayerInfo& info : m_players)
        {
            //同步手牌到端
            PROTO_VAR_PUBLIC(S_G13_HandOfMine, snd2)
            for (auto card : info.cards)
                snd2.add_cards(card);
            client->sendToMe(snd2Code, snd2);

            auto voteInfo = snd3.add_votes();
            voteInfo->set_cuid(info.cuid);
            voteInfo->set_vote(info.vote);
        }
    }
    if (m_status == GameStatus::vote)
    {
        client->sendToMe(snd3Code, snd3);
    }
}

void Game13::sendToAll(TcpMsgCode msgCode, const ProtoMsg& proto)
{
    for(const PlayerInfo& info : m_players)
    {
        if (info.cuid == 0)
            continue;

        Client::Ptr client = ClientManager::me().getByCuid(info.cuid);
        if (client != nullptr)
            client->sendToMe(msgCode, proto);
    }
}

void Game13::sendToOthers(ClientUniqueId cuid, TcpMsgCode msgCode, const ProtoMsg& proto)
{
    for(const PlayerInfo& info : m_players)
    {
        if (info.cuid == 0 || info.cuid == cuid)
            continue;

        Client::Ptr client = ClientManager::me().getByCuid(info.cuid);
        if (client != nullptr)
            client->sendToMe(msgCode, proto);
    }
}

void Game13::syncAllPlayersInfoToAllClients()
{
    PROTO_VAR_PUBLIC(S_G13_PlayersInRoom, snd)
    for(const PlayerInfo& info : m_players)
    {
        using namespace PublicProto;
        auto player = snd.add_players();
        player->set_cuid(info.cuid);
        player->set_name(info.name);
        player->set_status(info.status);
        player->set_money(info.money);
    }
    snd.set_rounds(m_rounds);
    sendToAll(sndCode, snd);
}

void Game13::removePlayer(ClientPtr client)
{
    if (client == nullptr)
        return;

    for (auto iter = m_players.begin(); iter != m_players.end(); ++iter)
    {
        if (iter->cuid == client->cuid())
        {
            client->afterLeaveRoom();
            iter->clear();
            LOG_TRACE("Game13, reomvePlayer, roomid={}, ccid={}, cuid={}, openid={}",
                        getId(), client->ccid(), client->cuid(), client->openid());
            break;
        }
    }
    syncAllPlayersInfoToAllClients();
    return;
}

void Game13::abortGame()
{
    //踢所有人
    for (const PlayerInfo& info : m_players)
    {
        LOG_TRACE("Game13, abortGame, 踢人, roomid={}, cuid={}", getId(), info.cuid);
        Client::Ptr client = ClientManager::me().getByCuid(info.cuid);
        if (client != nullptr)
            client->afterLeaveRoom();
    }
    m_players.clear();
    m_status = GameStatus::closed;
    destroyLater();
}

void Game13::tryStartRound()
{
    //everyone ready
    if (m_status == GameStatus::prepare)
    {
        for (const PlayerInfo& info : m_players)
        {
            if (info.cuid == 0 || info.status != PublicProto::S_G13_PlayersInRoom::READY)
                return;
            if (ClientManager::me().getByCuid(info.cuid) == nullptr) //必须都在线, 因为第一局要扣钱
                return;
        }
    }
    else if(m_status != GameStatus::settle)
    {
        for (const PlayerInfo& info : m_players)
        {
            if (info.cuid == 0 || info.status != PublicProto::S_G13_PlayersInRoom::COMPARE)
                return;
        }
    }

    //牌局开始
    ++m_rounds;
    LOG_TRACE("新一轮开始, rounds={}/{}, roomId={}", m_rounds, m_attr.rounds, getId());

    //shuffle
    {
        //初始化牌组
        if (m_attr.playType == GP_52)
        {
            Game13::s_deck.cards.resize(52);
            for (int32_t i = 0; i < 52; ++i)
                Game13::s_deck.cards[i] = i + 1;
        }
        else// if(m_attr.playType == GP_65)
        {
            Game13::s_deck.cards.reserve(65);
            for (int32_t i = 0; i < 52; ++i)
                Game13::s_deck.cards[i] = i + 1;
            for (int32_t i = 0; i < 13; ++i)
                Game13::s_deck.cards[i] = i + 39 + 1;
        }

        static std::random_device rd;
        static std::mt19937 rg(rd());
        std::shuffle(Game13::s_deck.cards.begin(), Game13::s_deck.cards.end(), rg);
    }

    //deal cards, then update player status and send to client
    PROTO_VAR_PUBLIC(S_G13_PlayersInRoom, snd1)
    snd1.set_rounds(m_rounds);
    uint32_t index = 0;
    for (PlayerInfo& info : m_players)
    {
        //发牌
        for (uint32_t i = 0; i < info.cards.size(); ++i)
            info.cards[i] = Game13::s_deck.cards[index++];

        //排个序, 以后检查外挂换牌时好比较
        std::sort(info.cards.begin(), info.cards.end());

        //同步手牌到端
        PROTO_VAR_PUBLIC(S_G13_HandOfMine, snd2)
        std::string cardsStr;
        cardsStr.reserve(42);
        for (auto card : info.cards)
        {
            cardsStr.append(std::to_string(card));
            cardsStr.append(",");
            snd2.add_cards(card);
        }
        LOG_TRACE("deal deck, roomid={}, round={}/{}, cuid={}, name={}, cards=[{}]", getId(), m_rounds, m_attr.rounds, info.cuid, info.name, cardsStr);
        auto client = ClientManager::me().getByCuid(info.cuid);
        if (client != nullptr)
        {
            if (m_rounds == 1) //第一局, 要扣钱
            {
                if(m_attr.payor == PAY_BANKER && client->cuid() == ownerCuid())
                {
                    client->addMoney(-10);
                    LOG_TRACE("游戏开始扣钱, 房主付费, moneyChange={}, roomid={}, ccid={}, cuid={}, openid={}",
                              -10, getId(), client->ccid(), info.cuid, client->openid());
                }
                else if(m_attr.payor == PAY_SHARE_EQU)
                {
                    client->addMoney(-5);
                    LOG_TRACE("游戏开始扣钱, 均摊, moneyChange={}, roomid={}, ccid={}, cuid={}, openid={}",
                              -5, getId(), client->ccid(), info.cuid, client->openid());
                }
            }

            client->sendToMe(snd2Code, snd2);
        }
        else
        {
            if (m_rounds == 1) //函数开头检查过所有乘员都可以获取Client::Ptr成功, 正常情况这里不会被执行才对
                LOG_ERROR("游戏开始, 需要扣费但是玩家不在线, payor={}, roomid={}, cuid={}", m_attr.payor, getId(), info.cuid);
        }

        //玩家状态改变
        info.status = PublicProto::S_G13_PlayersInRoom::SORT;
        auto player = snd1.add_players();
        player->set_status(info.status);
        player->set_cuid(info.cuid);
        player->set_name(info.name);
        player->set_money(info.money);
    }
    sendToAll(snd1Code, snd1);

    //游戏状态改变
    m_status = GameStatus::play;
}

void Game13::trySettleGame()
{
    if (m_status != GameStatus::play)
        return;

    //everyone compare
    for (const PlayerInfo& info : m_players)
    {
        if (info.cuid == 0 || info.status != PublicProto::S_G13_PlayersInRoom::COMPARE)
            return;
    }

    //TODO 比牌型, 算的分
    //先算牌型
    auto curRound = calcRound();
    for (uint32_t i = 0; i < m_players.size(); ++i)
    {
        curRound->players[i].cards = m_players[i].cards;
    }
    m_settleData.push_back(curRound);

    //发结果
    PROTO_VAR_PUBLIC(S_G13_AllHands, snd)
    for (const auto& pd : curRound->players)
    {
        auto player = snd.add_players();
        player->set_cuid(pd.cuid);
        for (Deck::Card crd : pd.cards)
            player->add_cards(crd);
        player->set_rank(pd.prize);
        player->mutable_dun0()->set_brand(static_cast<int32_t>(pd.dun[0].b));
        player->mutable_dun0()->set_point(static_cast<int32_t>(pd.dun[0].point));
        player->mutable_dun1()->set_brand(static_cast<int32_t>(pd.dun[1].b));
        player->mutable_dun1()->set_point(static_cast<int32_t>(pd.dun[1].point));
        player->mutable_dun2()->set_brand(static_cast<int32_t>(pd.dun[2].b));
        player->mutable_dun2()->set_point(static_cast<int32_t>(pd.dun[2].point));
        player->mutable_spec()->set_brand(static_cast<int32_t>(pd.spec));
        player->mutable_spec()->set_point(static_cast<int32_t>(0));
    }
    sendToAll(sndCode, snd);
    LOG_TRACE("单轮结算结束, round={}/{}, roomid={}", m_rounds, m_attr.rounds, getId());

    //总结算
    if (m_rounds >= m_attr.rounds)
    {
        //统计
        struct FinalCount
        {
            ClientUniqueId cuid = 0;
            int32_t win = 0;
            int32_t daqiang = 0;
            int32_t quanleida = 0;
            int32_t rank = 0;
        };
        std::vector<FinalCount> allFinalCount(m_players.size());;
        for (const auto& round : m_settleData)
        {
            for (auto i = 0u; i < round->players.size(); ++i)
            {
                const auto& pd = round->players[i];
                auto& count = allFinalCount[i];
                count.cuid = pd.cuid;
                count.win += pd.losers.size();
                bool daqiang = 0;
                for(const auto& item : pd.losers)
                    daqiang += item.second[1];
                count.daqiang += daqiang;
                if (pd.quanLeiDa)
                    count.quanleida += 1;
                count.rank += pd.prize;
            }
        }
        //发送
        PROTO_VAR_PUBLIC(S_G13_AllRounds, sndFinal)
        for (const auto& count : allFinalCount)
        {
            auto player = sndFinal.add_players();
            player->set_cuid(count.cuid);
            player->set_win(count.win);
            player->set_daqiang(count.daqiang);
            player->set_quanleida(count.quanleida);
            player->set_rank(count.rank);
        }
        sendToAll(sndFinalCode, sndFinal);
        LOG_TRACE("所有轮结束, 总结算完成 round={}/{}, roomid={}", m_rounds, m_attr.rounds, getId());

        //房间状态
        m_status = GameStatus::settleAll;
        m_settleAllTime = componet::toUnixTime(s_timerTime);
        return;
    }
        
    m_status = GameStatus::settle;
}

uint32_t Game13::getEmptySeatIndex()
{
    for (uint32_t i = 0; i < m_players.size(); ++i)
    {
        if (m_players[i].cuid == 0)
            return i;
    }
    return NO_POS;
}

Game13::PlayerInfo* Game13::getPlayerInfoByCuid(ClientUniqueId cuid)
{
    for (PlayerInfo& info : m_players)
    {
        if (info.cuid == cuid)
            return &info;
    }
    return nullptr;
}

void Game13::checkAllVotes()
{
    if (m_status != GameStatus::vote)
        return;

    PROTO_VAR_PUBLIC(S_G13_AbortGameOrNot, snd);
    snd.set_sponsor(m_voteSponsorCuid);
    time_t elapse = componet::toUnixTime(s_timerTime) - m_startVoteTime;
    snd.set_remain_seconds(MAX_VOTE_DURATION > elapse ? MAX_VOTE_DURATION - elapse : 0);
    uint32_t ayeSize = 0;
    ClientUniqueId oppositionCuid = 0;
    for (PlayerInfo& info : m_players)
    {
        if (info.vote == PublicProto::VT_AYE)
        {
            ++ayeSize;
        }
        else if (info.vote == PublicProto::VT_NAY)
        {
            oppositionCuid = info.cuid;
            break;
        }
        auto voteInfo = snd.add_votes();
        voteInfo->set_vote(info.vote);
        voteInfo->set_cuid(info.cuid);
    }
    if (oppositionCuid != 0) //有反对
    {
        //结束投票, 并继续游戏
        PROTO_VAR_PUBLIC(S_G13_VoteFailed, snd1);
        snd1.set_opponent(oppositionCuid);
        for (PlayerInfo& info : m_players)
        {
            info.vote = PublicProto::VT_NONE;
            auto client = ClientManager::me().getByCuid(info.cuid);
            if (client != nullptr)
                client->sendToMe(snd1Code, snd1);
        }
        m_startVoteTime = 0;
        m_voteSponsorCuid = 0;
        m_status = GameStatus::play;
        LOG_TRACE("投票失败, 游戏继续, 反对派cuid={}, roomId={}", oppositionCuid, getId());
    }
    else if (ayeSize == m_players.size()) //全同意
    {
        //结束投票, 并终止游戏
        for (PlayerInfo& info : m_players)
        {
            info.vote = PublicProto::VT_NONE;
            auto client = ClientManager::me().getByCuid(info.cuid);
            if (client != nullptr)
                client->noticeMessageBox("全体通过, 游戏解散!");
        }
        m_startVoteTime = 0;
        m_voteSponsorCuid = 0;
        LOG_TRACE("投票成功, 游戏终止, roomid={}", getId());
        abortGame();
    }
    else
    {
        //更新投票情况, 等待继续投票
        for (PlayerInfo& info : m_players)
        {
            auto client = ClientManager::me().getByCuid(info.cuid);
            if (client != nullptr)
                client->sendToMe(sndCode, snd);
        }
    }
}

void Game13::timerExec(componet::TimePoint now)
{
    switch (m_status)
    {
    case GameStatus::prepare:
        {
            tryStartRound(); //全就绪可能因为某人不在线而无法开始游戏, 这里需要自动重试
        }
        break;
    case GameStatus::play:
        {
        }
        break;
    case GameStatus::vote:
        {
            time_t elapse = componet::toUnixTime(s_timerTime) - m_startVoteTime;
            if (elapse >= MAX_VOTE_DURATION)
            {
                LOG_TRACE("投票超时, 游戏终止, roomid={}", getId());
                for (PlayerInfo& info : m_players)
                {
                    auto client = ClientManager::me().getByCuid(info.cuid);
                    if (client != nullptr)
                        client->noticeMessageBox("投票结束, 游戏解散!");
                    info.vote = PublicProto::VT_NONE;
                }
                m_startVoteTime = 0;
                abortGame();
            }
        }
        break;
    case GameStatus::settle:
        break;
    case GameStatus::settleAll:
        {
            time_t elapse = componet::toUnixTime(s_timerTime) - m_settleAllTime;
            if (elapse >= 120)
            {
                LOG_TRACE("总结算等待120s, 游戏终止, roomid={}", getId());
                m_settleAllTime = 0;
                abortGame();
            }
        }
        break;
    case GameStatus::closed:
        {
            destroyLater();
        }
    }
    return;
}

Game13::RoundSettleData::Ptr Game13::calcRound()
{
    auto rsd = RoundSettleData::create();
    rsd->players.reserve(m_players.size());
    for (PlayerInfo& info : m_players)
    {
        rsd->players.emplace_back();
        auto& data = rsd->players.back();
        data.cuid = info.cuid;
        data.name = info.name;
        data.cards = info.cards;

        //1墩
        data.dun[0] = Deck::brandInfo(data.cards.data(), 3);
        data.dun[1] = Deck::brandInfo(data.cards.data() + 3, 5);
        data.dun[2] = Deck::brandInfo(data.cards.data() + 8, 5);
        data.spec = Deck::g13SpecialBrand(data.cards.data(), data.dun[1].b, data.dun[2].b);
        data.prize = 0;
    }

    auto& datas = rsd->players;
    for (uint32_t i = 0; i < datas.size(); ++i)
    {
        auto& dataI = datas[i];
        for (uint32_t j = i + 1; j < datas.size(); ++j)
        {
            auto& dataJ = datas[j];
            ///////////////////////////////以下为特殊牌型//////////////////////////

            if (dataI.spec != dataJ.spec)
            {
                auto specCmpValueI = underlying(dataI.spec) % 10;
                auto specCmpValueJ = underlying(dataJ.spec) % 10;
                if (specCmpValueI == specCmpValueJ) //平局
                    continue;

                auto& specWinner = (specCmpValueI > specCmpValueJ) ?  dataI : dataJ;
                int32_t specPrize = 0;
                switch (specWinner.spec)
                {
                case Deck::G13SpecialBrand::flushStriaght: //11.   清龙（同花十三水）：若大于其他玩家，每家赢取104分
                    specPrize = 104;
                    break;
                case Deck::G13SpecialBrand::straight:
                    specPrize = 52;
                    break;
                case Deck::G13SpecialBrand::royal:
                case Deck::G13SpecialBrand::tripleStraightFlush:
                case Deck::G13SpecialBrand::tripleBombs:
                    specPrize = 26;
                    break;
                case Deck::G13SpecialBrand::allBig:
                case Deck::G13SpecialBrand::allLittle:
                case Deck::G13SpecialBrand::redOrBlack:
                case Deck::G13SpecialBrand::quradThreeOfKind:
                case Deck::G13SpecialBrand::pentaPairsAndThreeOfKind:
                case Deck::G13SpecialBrand::sixPairs:
                case Deck::G13SpecialBrand::tripleStraight:
                case Deck::G13SpecialBrand::tripleFlush:
                    specPrize = 6;
                    break;
                case Deck::G13SpecialBrand::none:
                    break;
                default:
                    break;
                }

                if (specCmpValueI > specCmpValueJ)
                {
                    specWinner.losers[i][0] = specPrize;
                    specWinner.losers[i][1] = 0;
                }
                else
                {
                    specWinner.losers[j][0] = specPrize;
                    specWinner.losers[j][1] = 0;
                }
                //本轮已经不用再比了, 因为至少有一家是特殊牌型, 特殊的都大于一般的
                continue;
            }


            /////////////////////////////////////以下为一般牌型//////////////////////
            int32_t dunCmps[] = 
            {
                Deck::cmpBrandInfo(dataI.dun[0], dataJ.dun[0]),
                Deck::cmpBrandInfo(dataI.dun[1], dataJ.dun[1]),
                Deck::cmpBrandInfo(dataI.dun[2], dataJ.dun[2]),
            };

            //下面按照按照逐个规则计算3墩的胜负分
            int32_t dunPrize[3] = {0, 0, 0}; //3墩胜负分的记录, I赢记整数, J赢记负数, 最后看正负知道谁赢了, 绝对值代表赢的数量

            // rule 1, 同一墩赢1个玩家1水 +1分
            // rule 2, 同一墩输1个玩家1水 -1分
            // rule 3, 同一墩和其它玩家打和（牌型大小一样）0分
            for (uint32_t d = 0; d < 3; ++d)
            {
                switch (dunCmps[d])
                {
                case 1: //I赢
                    dunPrize[d] += 1;
                    break;
                case 2: //J赢
                    dunPrize[d] -= 1;
                    break;
                case 0: //平
                default:
                    break;
                }
            }
            {// rule 4.  冲三：头墩为三张点数一样的牌且大于对手，记1分+2分奖励，共3分
                const uint32_t d = 0;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 2;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::threeOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::threeOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }
            }
            {// 5, 中墩葫芦：中墩为葫芦且大于对手，记1分+1分奖励，共2分
                const uint32_t d = 1;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 1;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fullHouse)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fullHouse)
                    {
                        dunPrize[d] -= extra;
                    }
                }
            }
            {//6, 五同：
                //中墩为五同且大于对手，记1分+19分奖励，共20分
                uint32_t d = 1;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 19;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fiveOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fiveOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }

                //尾墩为五同且大于对手，记1分+9分奖励，共10分
                d = 2;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 9;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fiveOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fiveOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }

            }
            
            {//7.   同花顺
                //中墩为同花顺且大于对手，记1分+9分奖励，共10分
                uint32_t d = 1;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 9;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::straightFlush)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::straightFlush)
                    {
                        dunPrize[d] -= extra;
                    }
                }

                //尾墩为同花顺且大于对手，记1分+4分奖励，共5分
                d = 2;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 4;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::straightFlush)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::straightFlush)
                    {
                        dunPrize[d] -= extra;
                    }
                }
            }
            {//8.   铁支, 四条
                //中墩为铁支且大于对手，记1分+7分奖励，共8分
                uint32_t d = 1;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 7;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fourOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fourOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }

                //尾墩为为铁支且大于对手，记1分+3分奖励，共4分
                d = 2;
                if (dunCmps[d] != 0)
                {
                    const uint32_t extra = 3;
                    if (dunCmps[d] == 1 && dataI.dun[d].b == Deck::Brand::fourOfKind)
                    {
                        dunPrize[d] += extra;
                    }
                    else if (dunCmps[d] == 2 && dataJ.dun[d].b == Deck::Brand::fourOfKind)
                    {
                        dunPrize[d] -= extra;
                    }
                }
            }

            //9, 打枪
            //如果三墩都比一个玩家大的话，向该玩家收取分数*2,包含特殊分
            //诸如冲三后打枪一个玩家，为5分+5分，共对该玩家收取10分
            const uint32_t prize = dunPrize[0] + dunPrize[1] + dunPrize[2];
            if (m_attr.daQiang && (dunPrize[0] > 0 && dunPrize[1] > 0 && dunPrize[2] > 0) ) //I打枪
            {
                dataI.losers[j][0] = prize;
                dataI.losers[j][1] = 1;
            }
            else if (m_attr.daQiang && (dunPrize[0] < 0 && dunPrize[1] < 0 && dunPrize[2] < 0) ) //J打枪
            {
                dataJ.losers[i][0] = -prize;
                dataJ.losers[i][1] = 1;
            }
            else //没有打枪
            {
                if (prize == 0) //平局
                    break;

                if (prize > 0) //I胜利
                {
                    dataI.losers[j][0] = prize;
                    dataI.losers[j][1] = 0;
                }
                else //J胜利
                {
                    dataJ.losers[i][0] = -prize;
                    dataJ.losers[i][1] = 0;
                }
            }
        }
    }
    //两两比完了, 最终结果把每一次pk都算上
    //10, 全垒打, 计算完每家打枪的分数后，再*2，也就是总分X分+X分
    {
        for (auto d = 0u; d < datas.size(); ++d)
        {
            auto& winner = datas[d];

            //判断是否是全垒打
            winner.quanLeiDa = false;
            if ((m_attr.quanLeiDa)  //全垒打启用
                && (m_attr.playerSize > 2) //两人房无全垒打
                && (winner.losers.size() + 1 == datas.size()) ) //全胜
            {
                for (auto iter = winner.losers.begin(); iter != winner.losers.end(); ++iter)
                {
                    if (iter->second[1] == 0)
                    {
                        winner.quanLeiDa = false;
                        break;
                    }
                }
            }

            //开始对losers逐人结算分数
            for (auto iter = winner.losers.begin(); iter != winner.losers.end(); ++iter)
            {

                auto& loser = datas[iter->first];
                int32_t prize = iter->second[0]; //分数得失
                if (iter->second[1] > 0) //打枪
                    prize *= 2;
                if (winner.quanLeiDa) //全垒打
                    prize *= 2;
                winner.prize += prize;
                loser.prize -= prize;
            }
        }       
    }
    return rsd;
}

}

