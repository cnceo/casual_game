#include "room.h"

namespace lobby{

RoomId Room::s_lastRoomId = 100000;
std::list<RoomId> Room::s_expiredIds;
std::unordered_map<RoomId, Room::Ptr> Room::s_rooms;

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

bool Room::add(Room::Ptr room)
{
    if (room == nullptr)
        return false;
    return s_rooms.insert(std::make_pair(room->getId(), room)).second;
}

void Room::del(Room::Ptr room)
{
    if (room == nullptr)
        return;
    s_rooms.erase(room->getId());
    //s_expiredIds.push_back(m_id); //暂不回收, 要解决ABA问题才能用回收机制
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
    for (auto iter = s_rooms.begin(); iter != s_rooms.end(); ++iter)
    {
        iter->second->timerExec(now);
    }
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

