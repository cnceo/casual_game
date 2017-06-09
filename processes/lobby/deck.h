#include <vector>
#include <string>
#include <algorithm>

#include <assert.h>

namespace lobby{

class Deck
{
    friend class Game13;

    using Card = int8_t;

    enum class Suit : uint8_t
    {
        SPADE   = 3,
        HEART   = 2,
        CLUB    = 1,
        DIAMOND = 0
    };

    enum class Rank : uint8_t
    {
        r2 = 0,
        r3 = 1,
        r4 = 2,
        r5 = 3,
        r6 = 4,
        r7 = 5,
        r8 = 6,
        r9 = 7,
        rX = 8,
        rJ = 9,
        rQ = 10,
        rK = 11,
        rA = 12,
    };

    enum class Brand : uint16_t //枚举还是取德州的叫法了, 规则是一样的
    {
        fiveOfKind      = 0x0200,  //五同(类似4条, 5张同点数的)
        straightFlush   = 0x0100,  //同花顺
        fourOfKind      = 0x0080,  //四条, 铁支
        fullHouse       = 0x0040,  //葫芦, 满堂红, 三带二
        flush           = 0x0020,  //同花
        straight        = 0x0010,  //顺子
        threeOfKind     = 0x0008,  //三条, 三一一
        twoPairs        = 0x0004,  //双对, 两对
        onePair         = 0x0002,  //单对, 对子, 一对
        hightCard       = 0x0001,  //乌龙, 高牌, 散牌
    };


    static Suit suit(Card card);

    static Rank rank(Card card);

    static bool cmpSuit(Card c1, Card c2);

    static bool cmpRank(Card c1, Card c2);

    static bool cmp(Card c1, Card c2);


    Brand brand(Card* begin, uint32_t size);
    Brand brand3(Card* begin);
    Brand brand5(Card* begin);

    std::vector<Card> cards;
};

inline Deck::Suit Deck::suit(Card card)
{
    //定义牌组数值时, 端要求[1, 52], 不是从0开始, 所以数值算法要 (c-1) 然后再整除或取余 ╮(╯▽╰)╭
    assert(card >= 1 && card <= 52);
    return static_cast<Suit>((card - 1) / 13);
}

inline Deck::Rank Deck::rank(Card card)
{
    assert(card >= 1 && card <= 52);
    return static_cast<Rank>((card - 1) % 13);
}

inline bool Deck::cmpSuit(Card c1, Card c2)
{
    return suit(c1) < suit(c2);
}

inline bool Deck::cmpRank(Card c1, Card c2)
{
    return rank(c1) < rank(c2);
}

inline bool Deck::cmp(Card c1, Card c2)
{
    return (rank(c1) != rank(c2)) ? cmpRank(c1, c2) : cmpSuit(c1, c2);
}

}
