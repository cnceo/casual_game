#include "room.h"
#include "client.h"

#include "componet/logger.h"
#include "componet/random.h"

namespace lobby{

const char* ROOM_TABLE_NAME = "tb_room";

RoomId Room::s_lastRoomId = 100000;
std::list<RoomId> Room::s_expiredIds;
std::unordered_map<RoomId, Room::Ptr> Room::s_rooms;
componet::TimePoint Room::s_timerTime;


RoomId Room::getRoomId()
{
    componet::Random<RoomId> rander(100001, 999999);
    RoomId id = rander.get();
    while (id == 0 || get(id) != nullptr)
    {
        if (s_expiredIds.empty())
        {
            if(++id > 999999)
                id = 100001;
        }
        else
        {
            id = s_expiredIds.front();
            s_expiredIds.pop_front();
        }
    }
    return id;
}

void Room::destroyLater()
{
    m_id = 0;
}

bool Room::add(Room::Ptr room)
{
    if (room == nullptr)
        return false;
    return s_rooms.insert(std::make_pair(room->getId(), room)).second;
}

void Room::delLater(Room::Ptr room)
{
    if (room == nullptr)
        return;
    room->destroyLater();
}

Room::Ptr Room::get(RoomId roomid)
{
    auto iter = s_rooms.find(roomid);
    if (iter == s_rooms.end())
        return nullptr;
    return iter->second;
}

void Room::timerExecAll(componet::TimePoint now)
{
    s_timerTime = now;
    auto iter = s_rooms.begin();
    while (iter != s_rooms.end())
    {
        if (iter->second->getId() == 0)
        {
            s_expiredIds.push_back(iter->first); //回收roomid
            iter = s_rooms.erase(iter);
            break;
        }
        iter->second->timerExec();
        ++iter;
    }
}

void Room::clientOnline(ClientPtr client)
{
    LOG_TRACE("Room, client on line, cuid={}, ccid={}, openid={}, roomid={}", client->cuid(), client->ccid(), client->openid(), client->roomid());
    if (client == nullptr || client->roomid() == 0)
        return;
    auto room = Room::get(client->roomid());
    if (room == nullptr)
    {
        LOG_TRACE("Room, client on line, room expried, cuid={}, ccid={}, openid={}, roomid={}", client->cuid(), client->ccid(), client->openid(), client->roomid());
        client->setRoomId(0);
        return;
    }
    room->clientOnlineExec(client);
}

/**********************************non static**************************************/

Room::Room(RoomId roomid, ClientUniqueId ownerCuid, GameType gameType)
: m_id(roomid)
, m_ownerCuid(ownerCuid)
, m_gameType(gameType)
{
}

Room::Room(ClientUniqueId ownerCuid, GameType gameType)
: m_id(getRoomId())
, m_ownerCuid(ownerCuid)
, m_gameType(gameType)
{
}

Room::~Room()
{
//    Room::s_rooms.erase(m_id);
//    Room::s_expiredIds.push_back(m_id); //暂不回收, 要解决ABA问题才能用回收机制
}


}

