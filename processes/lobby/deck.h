#include <vector>
#include <string>
#include <algorithm>

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
        straightFlush   = 0x0100,  //同花顺
        fourOfKind      = 0x0080,  //四条, 铁支
        fullHouse       = 0x0040,  //葫芦, 满堂红, 三代二
        flush           = 0x0020,  //同花
        straight        = 0x0010,  //顺子
        threeOfKind     = 0x0008,  //三条, 三一一
        twoPairs        = 0x0004,  //双对, 两对
        onwPair         = 0x0002,  //单对, 对子, 一对
        hightCard       = 0x0001,  //散牌, 高牌
    };


    Suit suit(Card card)
    {
        //定义牌组数值时, 端要求[1, 52], 不是从0开始, 所以数值算法要 (c-1) 然后再整除或取余 ╮(╯▽╰)╭
        assert(card >= 1 && card <= 52);
        return static_cast<Suit>((card - 1) / 13);
    }

    Rank rank(Card card)
    {
        assert(card >= 1 && card <= 52);
        return static_cast<Rank>((card - 1) % 13);
    }

    bool cmpBySuit(Card c1, Card c2)
    {
        return suit(c1) < suit(c2);
    }

    bool cmpByRank(Card c1, Card c2)
    {
        return rank(c1) < rank(c2);
    }


    Brand brand(Card* begin, uint32_t size)
    {
        Card* end = begin + size;
        if (size == 5)
        {
            {//staright
                bool sameSuit  = true;
                bool isStright = true;
                std::sort(begin, end, &Deck::cmpByRank);

                Suit s = suit(*begin);
                auto last = begin;
                auto cur = last + 1;
                do{
                    if (s != suit(cur)) //同花
                    if (rank(*last) + 1 != rank(*cur))
                        isStright = false;
                        break;
                } while (cur != end);

                //特殊的顺子, a1234
                if (rank(begin[0]) == Rank::r2 &&
                    rank(begin[1]) == Rank::r3 &&
                    rank(begin[2]) == Rank::r4 &&
                    rank(begin[3]) == Rank::r5 &&
                    rank(begin[4]) == Rank::rA)
                    isStright = true;
                if (isStright

            } 
        }
    }

    std::vector<Card> cards;
};

}
