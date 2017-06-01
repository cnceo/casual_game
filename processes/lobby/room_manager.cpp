
enum class GameType
{
    xm13;
    xmMaJiang,
};

enum class GameStatus
{
    creatRoom,
    waitPlayers,
    playing,
    settleResult,
};

using GameId = uint32_t;
class Game
{
public:
    GameType type() const;
    GameId id() const;
    bool addClient(Client::Ptr client);

private:
    GameId getGameId() const;
};

class Game13 : public Game
{
pbulic:
private:
    poke
};

struct PlayCard
{
    enum class SUITS {JOKER = 0, HEART = 3, DIAMOND = 4, CLUB = 5, SPADE = 6};
    uint16_t value = 0;
};

class GameManager
{
public:
    bool createNewGame(Client::Ptr client);
    bool jionGame(GameId, Client::Ptr);
private:
};
