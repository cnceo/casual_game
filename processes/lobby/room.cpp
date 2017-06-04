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
    Room::s_rooms.erase(m_id);
//    Room::s_expiredIds.push_back(m_id); //暂不回收, 要解决ABA问题才能用回收机制
}

}

