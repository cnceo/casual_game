#include "base/process_id.h"
#include "componet/class_helper.h"

#include <list>
#include <memory>
#include <unordered_map>

namespace lobby{

using namespace water;
using namespace process;


enum class GameType
{
    xm13,
    xmMaJiang,
};

using RoomId = uint32_t;

class Room : public std::enable_shared_from_this<Room>
{
public:
    TYPEDEF_PTR(Room)
    virtual ~Room();

    RoomId getId() const;
    bool addCuid(ClientUniqueId cuid) const;
    std::list<ClientUniqueId>& cuids();
    ClientUniqueId ownerCuid() const;

    /*
    bool full() const;
    uint32_t size() const;
*/
protected:
    Room(ClientUniqueId ownerCuid, uint32_t maxSize, GameType gameType);

private:
    RoomId m_id;
    const ClientUniqueId m_ownerCuid;
    const uint32_t m_maxSize;
    const GameType m_gameType;
//    std::list<ClientUniqueId> m_cuids;

private:
    static RoomId getRoomId();
private:
    static RoomId s_lastRoomId;
    static std::list<RoomId> s_expiredIds;
protected:

protected:
    static std::unordered_map<RoomId, Room::Ptr> s_rooms;
};

inline RoomId Room::getId() const
{
    return m_id;
}

inline ClientUniqueId Room::ownerCuid() const
{
    return m_ownerCuid;
}
/*
inline bool Room::full() const
{
    return size() == m_maxSize;
}

inline uint32_t Room::size() const
{
    return m_cuids.size();
}
*/

}

