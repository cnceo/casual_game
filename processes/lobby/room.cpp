#include "room.h"
#include "client.h"

namespace lobby{

RoomId Room::s_lastRoomId = 100000;
std::list<RoomId> Room::s_expiredIds;
std::unordered_map<RoomId, Room::Ptr> Room::s_rooms;
componet::TimePoint Room::s_timerTime;


RoomId Room::getRoomId()
{
    RoomId id = 0;
    if (s_expiredIds.empty())
        id = ++s_lastRoomId;
    else
    {
        id = s_expiredIds.front();
        s_expiredIds.pop_front();
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

Room::Ptr Room::get(RoomId roomId)
{
    auto iter = s_rooms.find(roomId);
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
            iter = s_rooms.erase(iter);
            //s_expiredIds.push_back(iter->first); //暂不回收, 要解决ABA问题才能用回收机制
            break;
        }
        iter->second->timerExec(now);
        ++iter;
    }
}

void Room::clientOnline(ClientPtr client)
{
    if (client == nullptr || client->roomId() == 0)
        return;
    auto room = Room::get(client->roomId());
    if (room == nullptr)
    {
        client->setRoomId(0);
        return;
    }
    room->clientOnlineExec(client);
}

/**********************************non static**************************************/

Room::Room(ClientUniqueId ownerCuid, uint32_t maxSize, GameType gameType)
: m_id(Room::getRoomId())
, m_ownerCuid(ownerCuid)
, m_maxSize(maxSize)
, m_gameType(gameType)
{
}

Room::~Room()
{
//    Room::s_rooms.erase(m_id);
//    Room::s_expiredIds.push_back(m_id); //暂不回收, 要解决ABA问题才能用回收机制
}


}

