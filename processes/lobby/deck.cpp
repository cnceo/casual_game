#include "deck.h"

namespace lobby{

Deck::Brand Deck::brand(Card* begin, uint32_t size)
{
    if (size == 5)
        return brand5(begin);
    else if (size == 3)
        return brand3(begin);
    return Brand::none;
}

Deck::Brand Deck::brand5(Card* c)
{
    const uint32_t size = 5;

    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::cmp);
    std::vector<Rank> r(size);
    std::vector<Suit> s(size);
    for (uint32_t i = 0; i < size; ++i)
    {
        r[i] = rank(c[i]);
        s[i] = suit(c[i]);
    }

    // check 1, 五同
    if (r[0] == r[4])
        return Brand::fiveOfKind;

    // 检查花色和连号情况
    bool isFlush  = true;
    bool isStright = true;
    for (uint32_t i = 1; i < size; ++i)
    {
        using T = typename std::underlying_type<Rank>::type;
        if (static_cast<T>(r[i - 1]) + 1 != static_cast<T>(r[i]))
            isStright = false;
        if (s[i - 1] != s[i])
            isFlush = false;
    }

    //特殊的顺子, a2345
    if (r[0] == Rank::r2 && 
        r[1] == Rank::r3 &&
        r[2] == Rank::r4 &&
        r[3] == Rank::r5 &&
        r[4] == Rank::rA)
        isStright = true;

    // check 2, 同花顺
    if (isStright && isFlush)
        return Brand::straightFlush;

    // check 3, 铁支, 四条
    if ( r[0] == r[3] || r[1] == r[4] )
        return Brand::fourOfKind;

    // check 4, 葫芦, 满堂红
    if ( (r[0] == r[2] && r[3] == r[4]) ||
         (r[2] == r[4] && r[0] == r[1]) )
        return Brand::fullHouse;

    // check 5, 同花
    if (isFlush)
        return Brand::flush;

    // check 6, 顺子
    if (isStright)
        return Brand::straight;

    // check 7, 三条
    if ( r[0] == r[2] || r[2] == r[4] || r[1] == r[3] )
        return Brand::threeOfKind;

    // check 8, 两对
    if ( (r[0] == r[1] && r[2] == r[3]) || 
         (r[0] == r[1] && r[3] == r[4]) || 
         (r[1] == r[2] && r[3] == r[4]) )
        return Brand::twoPairs;

    // check 9, 对子
    for (uint32_t i = 1; i < size; ++i)
    {
        if (r[i - 1] == r[i])
            return Brand::onePair;
    }

    // check 10, 乌龙, 散牌, 高牌
    return Brand::hightCard;
}

Deck::Brand Deck::brand3(Card* c)
{
    const uint32_t size = 3;

    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::cmp);
    std::vector<Rank> r(size);
    std::vector<Suit> s(size);
    for (uint32_t i = 0; i < size; ++i)
    {
        r[i] = rank(c[i]);
        s[i] = suit(c[i]);
    }

    // check 1, 三条
    if (r[0] == r[2])
        return Brand::threeOfKind;
    
    // check 2, 对子
    if (r[0] == r[1] || r[1] == r[2])
        return Brand::onePair;

    // check 3, 乌龙, 散牌, 高牌
    return Brand::hightCard;
}

}

