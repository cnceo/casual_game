#include "game13.h"
#include "client.h"
#include "componet/logger.h"
#include "protocol/protobuf/public/client.codedef.h"

#include <random>
#include <algorithm>

namespace lobby{


Game13::Deck Game13::s_deck;

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
    auto game = Game13::create(client->cuid(), rcv->player_size(), GameType::xm13);
    if (Room::add(game) == false)
    {
        LOG_ERROR("G13, 创建房间失败, Room::add失败");
        return;
    }

    //初始化游戏信息
    auto& attr      = game->m_attr;
    attr.roomId     = game->getId();
    attr.playType   = rcv->play_type();
    attr.rounds     = rcv->rounds();
    attr.payor      = rcv->payor();
    attr.daQiang    = rcv->da_qiang();
    attr.quanLeiDa  = rcv->quan_lei_da();
    attr.yiTiaoLong = rcv->yi_tiao_long();
    attr.playerSize = rcv->player_size();

    //依据属性检查创建资格,并初始化游戏的动态数据
    {
        //初始化元牌
        if (attr.playType == GP_52)
            Game13::s_deck.cards.resize(52);
        else// if(attr.playType == GP_65)
            Game13::s_deck.cards.resize(65);
        for (uint32_t i = 0; i < Game13::s_deck.cards.size(); ++i)
            Game13::s_deck.cards[i] = (i % 52) + 1;

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

    switch (game->m_status)
    {
    case GameStatus::prepare:
        {
            if (client->cuid() == game->ownerCuid()) //房主
            {
                LOG_TRACE("准备期间房主离开房间, roomId={}, ccid={}, cuid={}, openid={}",
                          client->roomId(), client->ccid(), client->cuid(), client->openid()); 
                game->abortGame();
                Room::del(game); //销毁房间
                LOG_TRACE("准备期间终止游戏, 房间已销毁, roomId={}", game->getId());
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
            client->noticeMessageBox("游戏进行中, 离开解散房间");
            game->abortGame();
            Room::del(game);
            LOG_TRACE("游戏进行期间离开， 终止游戏");
        }
        break;
    case GameStatus::settle:
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
    auto rcv = PROTO_PTR_CAST_PUBLIC(C_G13_VoteFoAbortGame, proto);
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

    PlayInfo* info = nullptr;
    for(PlayInfo& i : game->m_players)
    {
        if (i.cuid == client->cuid())
        {
            info = &i;
            break;
        }
    }
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
        LOG_ERROR("BringOut, client上记录的房间号不存在, roomId={}, ccid={}, cuid={}, openid={}", 
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

    PlayInfo* info = nullptr;
    for(PlayInfo& i : game->m_players)
    {
        if (i.cuid == client->cuid())
        {
            info = &i;
            break;
        }
    }
    if (info == nullptr)
    {
        LOG_ERROR("BringOut, 房间中没有这个玩家的信息, roomId={}, ccid={}, cuid={}, openid={}",
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
    
    //改变状态
    info->status = PublicProto::S_G13_PlayersInRoom::COMPARE;
    std::string cardsStr;
    cardsStr.reserve(42);
    for (uint32_t index = 0; index < info->cards.size(); ++index)
    {
        info->cards[index] = rcv->cards(index);
        cardsStr.append(std::to_string(info->cards[index]));
        cardsStr.append(",");
    }
    LOG_TRACE("BringOut, roomId={}, ccid={}, cuid={}, openid={}, cards=[{}]",
             client->roomId(), client->ccid(), client->cuid(), client->openid(), cardsStr);

    PROTO_VAR_PUBLIC(S_G13_PlayersInRoom, snd)
    auto player = snd.add_players();
    player->set_status(info->status);
    player->set_cuid(info->cuid);
    player->set_name(info->name);
    game->sendToAll(sndCode, snd);

    game->trySettleGame();

    //是否已经结束
    if (game->m_status == GameStatus::closed)
    {
        game->abortGame();
        return;
    }

    //没结束, 就开下一局
    game->tryStartRound();
}


/*************************************************************************/
Game13::Game13(ClientUniqueId ownerCuid, uint32_t maxSize, GameType gameType)
    : Room(ownerCuid, maxSize, gameType)
{
}

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
    m_players[index].status = PublicProto::S_G13_PlayersInRoom::PREP;
    m_players[index].money = client->money();
    
    {//预扣钱, 这个放到最后, 避免扣钱后进入失败需要回退
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

    playerOnLine(client);

    LOG_TRACE("G13, {}房间成功, roomId={}, ccid={}, cuid={}, openid={}", (ownerCuid() != client->cuid()) ? "进入" : "创建",
              client->roomId(), client->ccid(), client->cuid(), client->openid());
    return true;
}

void Game13::playerOnLine(Client::Ptr client)
{
    if (client == nullptr)
        return;
    {//给进入者发送房间基本属性
        PROTO_VAR_PUBLIC(S_G13_RoomAttr, snd)
        snd.set_room_id(m_attr.roomId);
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
    if (m_status == GameStatus::play)
    {
        for (PlayInfo& info : m_players)
        {
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
            client->sendToMe(snd2Code, snd2);
        }
    }

}

void Game13::sendToAll(TcpMsgCode msgCode, const ProtoMsg& proto)
{
    for(const PlayInfo& info : m_players)
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
    for(const PlayInfo& info : m_players)
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
    for(const PlayInfo& info : m_players)
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
    //退钱
    if (m_status == GameStatus::play || m_status == GameStatus::settle)
    {
        switch (m_attr.payor)
        {
        case PAY_BANKER:
            {
                auto roomOwner = ClientManager::me().getByCuid(ownerCuid());
                if (roomOwner == nullptr)
                {
                    LOG_ERROR("开始前解散房间, 房主模式, 退款时不在线, cuid={}, money={}, roomId={}", ownerCuid(), 10, getId());
                    break;
                }
                roomOwner->addMoney(10);
                LOG_TRACE("解开始前散房间, 房主模式, 退款成功, cuid={}, money={}, roomId={}", ownerCuid(), 10, getId());
            }
            break;
        case PAY_SHARE_EQU:
            {
                for(const PlayInfo& info : m_players)
                {
                    Client::Ptr client = ClientManager::me().getByCuid(info.cuid);
                    if (client == nullptr)
                    {
                        LOG_ERROR("解开始前散房间, AA模式, 退款时不在线, cuid={}, money={}, roomId={}", ownerCuid(), 5, getId());
                        continue;
                    }
                    client->addMoney(5);
                    LOG_TRACE("开始前解散房间, AA模式, 退款成功, cuid={}, money={}, roomId={}", ownerCuid(), 5, getId());
                    //顺带发送踢掉通知
                    client->afterLeaveRoom();
                    LOG_TRACE("Game13, abortGame, 踢人, roomid={}, ccid={}, cuid={}, openid={}",
                              getId(), client->ccid(), client->cuid(), client->openid());
                }
                m_players.clear();
                m_players.resize(m_attr.playerSize);
                m_status = GameStatus::closed;
                return;
            }
            break;
        case PAY_WINNER:
            //没结束, 所以没人付钱, 不用退
            break;
        default:
            break;
        }
    }
    //踢所有人
    for (const PlayInfo& info : m_players)
    {
        Client::Ptr client = ClientManager::me().getByCuid(info.cuid);
        if (client == nullptr)
            continue;
        client->afterLeaveRoom();
        LOG_TRACE("Game13, abortGame, 踢人, roomid={}, ccid={}, cuid={}, openid={}",
                  getId(), client->ccid(), client->cuid(), client->openid());
    }
    m_players.clear();
    m_players.resize(m_attr.playerSize);
    m_status = GameStatus::closed;
}

void Game13::tryStartRound()
{
    if (m_status != GameStatus::prepare && m_status != GameStatus::settle)
        return;

    //everyone ready
    for (const PlayInfo& info : m_players)
    {
        if (info.cuid == 0 || info.status != PublicProto::S_G13_PlayersInRoom::READY)
            return;
    }

    //牌局开始
    ++m_rounds;

    //shuffle
    {
        std::random_device rd;
        std::shuffle(Game13::s_deck.cards.begin(), Game13::s_deck.cards.end(), std::mt19937(rd()));
    }

    //deal cards, then update player status and send to client
    PROTO_VAR_PUBLIC(S_G13_PlayersInRoom, snd1)
    snd1.set_rounds(m_rounds);
    uint32_t index = 0;
    for (PlayInfo& info : m_players)
    {
        //发牌
        for (uint32_t i = 0; i < info.cards.size(); ++i)
            info.cards[i] = Game13::s_deck.cards[index++];

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
        auto client = ClientManager::me().getByCuid(info.cuid);
        if (client == nullptr)
        {
            LOG_TRACE("发牌, 玩家不在线, roomid={}, ccid={}, cards=[{}]", getId(), info.cuid, cardsStr);
        }
        else
        {
            if (m_rounds == 1) //第一局
            {
                if(m_attr.payor == PAY_BANKER && client->cuid() == ownerCuid())
                {
                    client->addMoney(-10);
                    info.money -= 10;
                    LOG_TRACE("游戏开始扣钱, 房主付费, moneyChange={}, roomid={}, ccid={}, cuid={}, openid={}",
                              -10, getId(), client->ccid(), info.cuid, client->openid());
                }
                else if(m_attr.payor == PAY_SHARE_EQU)
                {
                    client->addMoney(-5);
                    info.money -= 5;
                    LOG_TRACE("游戏开始扣钱, 均摊, moneyChange={}, roomid={}, ccid={}, cuid={}, openid={}",
                              -5, getId(), client->ccid(), info.cuid, client->openid());
                }
            }

            LOG_TRACE("发牌, roomid={}, ccid={}, cuid={}, openid={}, cards=[{}]",
                      getId(), client->ccid(), info.cuid, client->openid(), cardsStr);
            client->sendToMe(snd2Code, snd2);
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
    if (m_status != GameStatus::prepare)
        return;

    //everyone compare
    for (const PlayInfo& info : m_players)
    {
        if (info.cuid == 0 || info.status != PublicProto::S_G13_PlayersInRoom::COMPARE)
            return;
    }


    //TODO 比牌型, 算的分
    for (PlayInfo& info : m_players)
    {
        info.rank += 0;
    }

    //发结果
    PROTO_VAR_PUBLIC(S_G13_AllHands, snd)
    for (const PlayInfo& info : m_players)
    {
        auto player = snd.add_players();
        player->set_cuid(info.cuid);
        for (Deck::Card crd : info.cards)
            player->add_cards(crd);
        player->set_rank(info.rank);
    }
    sendToAll(sndCode, snd);

    //局数计数器, 并更新游戏装态
    m_status = (m_rounds < m_attr.rounds) ? GameStatus::settle : GameStatus::closed;
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

}

