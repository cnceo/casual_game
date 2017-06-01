//#include "lobby.h"

#include "client_manager.h"

enum class GameType
{
    xm13;
    xmMaJiang,
};

using RoomId = uint32_t;
class Room
{
public:
    TYPEDEF_PTR(Room)
    ~Room()
    {
        Room::s_rooms.erase(m_id);
        expiredIds.push_back(m_id);
    }

    RoomId getId() const;
    bool addCuid(ClientUniqueId cuid) const;
    std::list<ClientUniqueId>& cuids();
    ClientUniqueId ownerCuid() const;
    bool full() const;
    uint32_t size() const;

private:
    Room(RomId id, ClientUniqueId ownerCuid, uint32_t maxSize, GameType gameType)
    : m_id(id), m_ownerCuid(ownerCuid), m_maxSize(maxSize), m_gameType(gameType)
    {
    }

private:
    RoomId m_id;
    const ClientUniqueId m_ownerCuid;
    const uint32_t m_maxSize;
    const GameType m_gameType;
    std::list<ClientUniqueId> m_cuids;

public:
    static Room::Ptr create(ClientUniqueId ownerCuid, uint32_t maxSize, GameType gameType);
private:
    static RoomId s_lastRoomId = 100000;
    static std::list<RoomId> s_expiredIds;
};

inline RoomId Room::getId() const
{
    return m_id;
}

bool Room::addCuid(ClientUniqueId cuid) const
{
    if (full())
        return false;
    m_cuids.push_back(cuid);
    return true;
}

std::list<ClientUniqueId>& Room::cuids()
{
    return m_cuids;
}

ClientUniqueId Room::ownerCuid() const
{
    return m_ownerCuid;
}

bool Room::full() const
{
    return 
}

uint32_t Room::size() const
{
    m_cuids.size();
}

Room::Ptr Room::create(ClientUniqueId ownerCuid, uint32_t maxSize, GameType gameType)
{
    RoomId id = 0;
    if (s_expiredIds.empty())
        id = ++s_lastRoomId;
    else
    {
        id = s_expiredIds.front();
        s_expiredIds.pop_front();
    }

    Room::Ptr ret(new Room(ownerCuid, maxSize, gameType);
}

