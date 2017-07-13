#include "client.h"
#include "lobby.h"

#include "dbadaptcher/redis_handler.h"

#include "componet/logger.h"
#include "componet/datetime.h"

#include "protocol/protobuf/proto_manager.h"
#include "protocol/protobuf/public/client.codedef.h"
#include "permanent/proto/game13.pb.h"

namespace lobby{

const char* CLIENT_TABLE_NAME = "tb_client";

const uint32_t MAX_G13HIS_SIZE = 50;

std::string Client::serialize() const
{
    permanent::ClientData proto;
    proto.set_openid(m_openid);
    proto.set_openid(m_openid);

    proto.set_openid  (m_openid);
    proto.set_cuid    (m_cuid  );
    proto.set_name    (m_name  );
    proto.set_roomid  (m_roomid);
    proto.set_money   (m_money );
    proto.set_money1  (m_money1);
    proto.set_token   (m_token );

    auto protoG13his = proto.mutable_g13his();
    protoG13his->set_week_rank (m_g13his.weekRank);
    protoG13his->set_week_game (m_g13his.weekGame);
    protoG13his->set_today_rank(m_g13his.todayRank);
    protoG13his->set_today_game(m_g13his.todayGame);
    for (const auto&detail : m_g13his.details)
    {
        auto protoDetail = protoG13his->add_details();
        protoDetail->set_roomid(detail->roomid);
        protoDetail->set_rank(detail->rank);
        protoDetail->set_time(detail->time);

        for (const auto&opp : detail->opps)
        {
            auto protoOpp = protoDetail->add_opps();
            protoOpp->set_name(opp.name);
            protoOpp->set_rank(opp.rank);
        }
    }

    std::string ret;
    return proto.SerializeToString(&ret) ? ret : "";
}

bool Client::deserialize(const std::string& bin)
{
    permanent::ClientData proto;
    if (!proto.ParseFromString(bin))
        return false;

    m_openid  = proto.openid();
    m_cuid    = proto.cuid  ();
    m_name    = proto.name  ();
    m_roomid  = proto.roomid();
    m_money   = proto.money ();
    m_money1  = proto.money1();
    m_token   = proto.token ();

    const auto& protoG13his = proto.g13his();
    m_g13his.weekRank     = protoG13his.week_rank();
    m_g13his.weekGame     = protoG13his.week_game();
    m_g13his.todayRank     = protoG13his.today_rank();
    m_g13his.todayGame    = protoG13his.today_game();

    for (const auto& protoDetail : protoG13his.details())
    {
        auto detail = G13His::Detail::create();
        detail->roomid  = protoDetail.roomid();
        detail->rank    = protoDetail.rank();
        detail->time    = protoDetail.time();

        auto& opps = detail->opps;
        const auto& protoOpps = protoDetail.opps();
        opps.resize(protoOpps.size());
        for (auto j = 0; j < protoOpps.size(); ++j)
        {
            opps[j].name = protoOpps[j].name();
            opps[j].rank = protoOpps[j].rank();
        }

        m_g13his.details.push_back(detail);
    }

    return true;
}

bool Client::saveToDB() const
{
    using water::dbadaptcher::RedisHandler;
    RedisHandler& redis = RedisHandler::me();
    const std::string& bin = serialize();
    if (bin.size() == 0)
    {
        LOG_ERROR("Client sizeToDB, serialize failed, openid={}, cuid={}, roomid={}, money={}, money1={}", 
                  openid(), cuid(), roomid(), money(), money1());
        return false;
    }
    if (!redis.hset(CLIENT_TABLE_NAME, openid(), bin))
    {
        LOG_ERROR("save client, redis hset failed, openid={}, cuid={}, roomid={}, money={}, money1={}", 
                  openid(), cuid(), roomid(), money(), money1());
        return false;
    }
    LOG_TRACE("save client, successed, openid={}, cuid={}, roomid={}, money={}, money1={}", 
                  openid(), cuid(), roomid(), money(), money1());
    return true; 
}

void Client::afterLeaveRoom(G13His::Detail::Ptr detail)
{
    m_roomid = 0;

    if (detail != nullptr)
    {
        if (!m_g13his.details.empty())
        {
            auto lastGameTimePoint = componet::fromUnixTime(m_g13his.details.front()->time);
            auto thisGameTimePoint = componet::fromUnixTime(detail->time);
            if (!componet::inSameDay(lastGameTimePoint, thisGameTimePoint))
            {
                m_g13his.todayRank = 0;
                m_g13his.todayGame = 0;
            }
            if (!componet::inSameWeek(lastGameTimePoint, thisGameTimePoint, 1))
            {
                m_g13his.weekRank = 0;
                m_g13his.weekGame = 0;

            }
        }

        m_g13his.weekRank += detail->rank;
        m_g13his.weekGame += 1;
        m_g13his.todayRank += detail->rank;
        m_g13his.todayGame += 1;

        m_g13his.details.push_front(detail);
        if (m_g13his.details.size() > 50)
            m_g13his.details.pop_back();
        saveToDB();
    }

    PROTO_VAR_PUBLIC(S_G13_PlayerQuited, snd);
    sendToMe(sndCode, snd);
}

bool Client::sendToMe(TcpMsgCode code, const ProtoMsg& proto) const
{
    ProcessId gatewayPid("gateway", 1);
    return Lobby::me().relayToPrivate(ccid(), gatewayPid, code, proto);
}

bool Client::noticeMessageBox(const std::string& text)
{
    PROTO_VAR_PUBLIC(S_Notice, snd);
    snd.set_type(PublicProto::S_Notice::MSG_BOX);
    snd.set_text(text);
    return sendToMe(sndCode, snd);
}

void Client::syncBasicDataToClient() const
{
    PROTO_VAR_PUBLIC(S_PlayerBasicData, snd);
    snd.set_money(money());
    snd.set_money1(money1());
    sendToMe(sndCode, snd);
}

}
