#include "deck.h"
#include "componet/tools.h"

namespace lobby{

Deck::BrandInfo Deck::brandInfo(Card* c, uint32_t size)
{
    if (size == 5)
        return brandInfo5(c);
    else if (size == 3)
        return brandInfo3(c);
    return {Brand::hightCard, 0};
}

Deck::BrandInfo Deck::brandInfo5(Card* c)
{
    const uint32_t size = 5;
    using IntOfRank = typename std::underlying_type<Rank>::type;


    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]), rank(c[3]), rank(c[4]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]), suit(c[3]), suit(c[4]) };
//    std::vector<int32_t> powR = { 7529536, 537824, 38416, 2744, 196, 14, 1 }; //14的
    std::vector<int32_t> pow = { 1, 14, 196, 2744, 38416, 537824, 7529536 };
    auto rtoi = [&r](uint32_t i)->IntOfRank
    {
        return static_cast<IntOfRank>(r[i]) + 1;
    };

    // check 9, 五同
    if (r[0] == r[4])
        return {Brand::fiveOfKind, rtoi(0)};

    // 检查花色和连号情况
    bool isStright = true;
    bool isFlush  = true;
    for (uint32_t i = 1; i < size; ++i)
    {
        if (rtoi(i - 1) + 1 != rtoi(i))
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

    // check 8, 同花顺
    if (isStright && isFlush)
    {
        int32_t point = rtoi(4);
        if (r[4] == Rank::rA)
            point = (r[0] == Rank::r2) ? 14 : 15;
        return {Brand::straightFlush, point};
    }

    {// check 7, 铁支, 四条
        int32_t point = 0;
        if (r[0] == r[3])
            point = rtoi(3);
        else if(r[1] == r[4])
            point = rtoi(4);
        if (point > 0)
            return {Brand::fourOfKind, point};
    }

    {// check 6, 葫芦, 满堂红
        int32_t point = 0;
        if (r[0] == r[2] && r[3] == r[4])
           point = rtoi(2);
        else if (r[2] == r[4] && r[0] == r[1])
            point = rtoi(4);
        if (point > 0)
            return {Brand::fullHouse, point};
    }

    // check 5, 同花
    if (isFlush)
    {
        //return {Brand::flush, rtoi(4)}; //最大的牌
        {//判断其中是否存在两对
            int32_t point = 0; //rank(大对子) * 14^^6 + rank(小对子) * 14^^5 + rank(单张)
            if (r[0] == r[1] && r[2] == r[3])
                point = rtoi(3) * pow[6] + rtoi(1) * pow[5] + rtoi(4);
            else if (r[0] == r[1] && r[3] == r[4])
                point = rtoi(4) * pow[6] + rtoi(1) * pow[5] + rtoi(2);
            else if (r[1] == r[2] && r[3] == r[4])
                point = rtoi(4) * pow[6] + rtoi(2) * pow[5] + rtoi(0);
            if (point > 0)
                return {Brand::flush, point};
        }
        {//判断其中是否存在对子
            uint32_t pairIndex = -1;
            for (uint32_t i = 1; i < size; ++i)
            {
                if (r[i - 1] == r[i])
                {
                    pairIndex = i;
                    break;
                }
            }
            if (pairIndex != uint32_t(-1))
            {
                int32_t point = 0; //rank(对子) * 14^^5 + point(3张乌龙)
                point += rtoi(pairIndex) * pow[5];
                uint32_t hightCounter = 0;
                for (uint32_t i = 0; i < size; ++i)
                {
                    if (i == pairIndex || i == pairIndex - 1u)
                        continue;
                    point += rtoi(i) * pow[hightCounter++];
                }
                return {Brand::flush, point};
            }
        }

        //不包含两对也不包含对子, 那就只能是包含乌龙了
        int32_t point = 0;
        for (uint32_t i = 0; i < size; ++i)
            point += rtoi(i) * pow[i];
        return {Brand::flush, point};
    }

    // check 4, 顺子
    if (isStright)
    {
        int32_t point = rtoi(4);
        if (r[4] == Rank::rA)
            point = (r[0] == Rank::r2) ? 14 : 15;
        return {Brand::straight, point};
    }

    {// check 3, 三条
        int32_t point = 0;
        if (r[0] == r[2])
            point = rtoi(2);
        else if (r[1] == r[3])
            point = rtoi(3);
        else if (r[2] == r[4])
            point = rtoi(4);
        if (point > 0)
            return {Brand::threeOfKind, point};
    }

    {// check 2, 两对
        int32_t point = 0; //rank(大对子) * 14^^6 + rank(小对子) * 14^^5 + rank(单张)
        if (r[0] == r[1] && r[2] == r[3])
            point = rtoi(3) * pow[6] + rtoi(1) * pow[5] + rtoi(4);
        else if (r[0] == r[1] && r[3] == r[4])
            point = rtoi(4) * pow[6] + rtoi(1) * pow[5] + rtoi(2);
        else if (r[1] == r[2] && r[3] == r[4])
            point = rtoi(4) * pow[6] + rtoi(2) * pow[5] + rtoi(0);
        if (point > 0)
            return {Brand::twoPairs, point};
    }
    {// check 1, 对子
        uint32_t pairIndex = -1u;
        for (uint32_t i = 1; i < size; ++i)
        {
            if (r[i - 1] == r[i])
            {
                pairIndex = i;
                break;
            }
        }
        if (pairIndex != -1u)
        {
            int32_t point = 0; //rank(对子) * 14^^5 + point(3张乌龙)
            point += rtoi(pairIndex) * pow[5];
            uint32_t hightCounter = 0;
            for (uint32_t i = 0; i < size; ++i)
            {
                if (i == pairIndex || i == pairIndex - 1)
                    continue;
                point += rtoi(i) * pow[hightCounter++];
            }
            return {Brand::onePair, point};
        }
    }

    // check 0, 乌龙, 散牌, 高牌
    int32_t point = 0;
    for (uint32_t i = 0; i < size; ++i)
        point += rtoi(i) * pow[i];
    return {Brand::hightCard, point};
}

Deck::BrandInfo Deck::brandInfo3(Card* c)
{
    const uint32_t size = 3;
    using IntOfRank = typename std::underlying_type<Rank>::type;

    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]) };
    std::vector<int32_t> pow = { 1, 14, 196, 2744, 38416, 537824, 7529536 };
    auto rtoi = [&r](uint32_t i)->IntOfRank
    {
        return static_cast<IntOfRank>(r[i]) + 2;
    };

    // check 1, 三条
    if (r[0] == r[2])
        return {Brand::threeOfKind, rtoi(2)};
    
    {// check 2, 对子
        int32_t point = 0;
        if (r[0] == r[1])
            point = rtoi(1) * pow[3] + rtoi(2);
        else if (r[1] == r[2])
            point = rtoi(2) * pow[3] + rtoi(0);
        if (point > 0)
            return {Brand::onePair, point};
    }

    // check 3, 乌龙, 散牌, 高牌
    int32_t point = 0;
    for (uint32_t i = 0; i < size; ++i)
        point += rtoi(i) * pow[i];
    return {Brand::hightCard, point};
}

Deck::Brand Deck::brand(Card* c, uint32_t size)
{
    if (size == 5)
        return brand5(c);
    else if (size == 3)
        return brand3(c);
    return Brand::hightCard;
}

Deck::Brand Deck::brand5(Card* c)
{
    const uint32_t size = 5;

    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]), rank(c[3]), rank(c[4]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]), suit(c[3]), suit(c[4]) };

    // check 9, 五同
    if (r[0] == r[4])
        return Brand::fiveOfKind;

    // 检查花色和连号情况
    bool isStright = true;
    bool isFlush  = true;
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

    // check 8, 同花顺
    if (isStright && isFlush)
        return Brand::straightFlush;

    // check 7, 铁支, 四条
    if ( r[0] == r[3] || r[1] == r[4] )
        return Brand::fourOfKind;

    // check 6, 葫芦, 满堂红
    if ( (r[0] == r[2] && r[3] == r[4]) ||
         (r[2] == r[4] && r[0] == r[1]) )
        return Brand::fullHouse;

    // check 5, 同花
    if (isFlush)
        return Brand::flush;

    // check 4, 顺子
    if (isStright)
        return Brand::straight;

    // check 3, 三条
    if ( r[0] == r[2] || r[2] == r[4] || r[1] == r[3] )
        return Brand::threeOfKind;

    // check 2, 两对
    if ( (r[0] == r[1] && r[2] == r[3]) || 
         (r[0] == r[1] && r[3] == r[4]) || 
         (r[1] == r[2] && r[3] == r[4]) )
        return Brand::twoPairs;

    // check 1, 对子
    for (uint32_t i = 1; i < size; ++i)
    {
        if (r[i - 1] == r[i])
            return Brand::onePair;
    }

    // check 0, 乌龙, 散牌, 高牌
    return Brand::hightCard;
}

Deck::Brand Deck::brand3(Card* c)
{
    const uint32_t size = 3;

    //排序并提取suit 和 rank
    Card* end = c + size;
    std::sort(c, end, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]) };

    // check 1, 三条
    if (r[0] == r[2])
        return Brand::threeOfKind;
    
    // check 2, 对子
    if (r[0] == r[1] || r[1] == r[2])
        return Brand::onePair;

    // check 3, 乌龙, 散牌, 高牌
    return Brand::hightCard;
}

Deck::G13SpecialBrand Deck::g13SpecialBrand(Card* c, Brand b2, Brand b3)
{
    G13SpecialBrand dun = g13SpecialBrandByDun(c, b2, b3);
    return g13SpecialBrandByAll(c, dun);
}

Deck::G13SpecialBrand Deck::g13SpecialBrandByDun(Card* c, Brand b2, Brand b3)
{
    const uint32_t size = 3;
    //排序第一墩
    std::sort(c, c + size, &Deck::lessCard);
    std::vector<Rank> r = { rank(c[0]), rank(c[1]), rank(c[2]) };
    std::vector<Suit> s = { suit(c[0]), suit(c[1]), suit(c[2]) };

    // 检查花色和连号情况
    bool isStright = ((underlying(r[0]) + 1 == underlying(r[1]) && underlying(r[1]) + 1 == underlying(r[2])) ||
                      (r[0] == Rank::r2 && r[1] == Rank::r3 && r[2] == Rank::rA));
    const bool isFlush = (s[0] == s[1] && s[1] == s[2]);

    // check 10, 三同花顺 
    if (isStright && isFlush && b2 == Brand::straightFlush && b3 == Brand::straightFlush)
        return G13SpecialBrand::tripleStraightFlush;

    // check 2, 三顺子
    if (isStright && (b2 == Brand::straight || b2 == Brand::straightFlush) && (b3 == Brand::straight || b3 == Brand::straightFlush))
        return G13SpecialBrand::tripleStraight;

    // check 1, 三同花
    if (isFlush && b2 == Brand::flush && b3 == Brand::flush)
        return G13SpecialBrand::tripleFlush;

    return G13SpecialBrand::none;
}

Deck::G13SpecialBrand Deck::g13SpecialBrandByAll(Card* c, G13SpecialBrand dun)
{
    //排序全部
    const uint32_t size = 13;
    std::sort(c, c + size, &Deck::lessCard);
    std::vector<Rank> r(size);
    std::vector<Suit> s(size);
    for (uint32_t i = 0; i < size; ++i)
    {
        r[i] = rank(c[i]);
        s[i] = suit(c[i]);
    }

    // 检查花色和连号情况
    bool isStright = true;
    bool isFlush  = true;
    for (uint32_t i = 1; i < size; ++i)
    {
        using T = typename std::underlying_type<Rank>::type;
        if (static_cast<T>(r[i - 1]) + 1 != static_cast<T>(r[i]))
            isStright = false;
        if (s[i - 1] != s[i])
            isFlush = false;
    }

    // check 13, 青龙
    if (isStright && isFlush)
        return G13SpecialBrand::flushStriaght;

    // check 12, 一条龙
    if (isStright)
        return G13SpecialBrand::straight;

    // check 11, 12皇族
    if (r[0] >= Rank::rJ)
        return G13SpecialBrand::royal;

    // check 10, 三墩同花顺
    if (dun == G13SpecialBrand::tripleStraightFlush)
        return dun;

    // check 9, 三个四条, 以下判断单张分别在 0, 4, 8, 12位置
    if ( (r[1]==r[4] && r[5]==r[8] && r[9]==r[12]) ||
         (r[0]==r[3] && r[5]==r[8] && r[9]==r[12]) ||
         (r[0]==r[3] && r[4]==r[7] && r[9]==r[12]) ||
         (r[0]==r[3] && r[4]==r[7] && r[8]==r[11]) )
        return G13SpecialBrand::tripleBombs;

    // check 8, 全大
    if (r[0] >= Rank::r8)
        return G13SpecialBrand::allBig;

    // check 7, 全小
    if (r[12] <= Rank::r8)
        return G13SpecialBrand::allLittle;

    // check 6, 全红或全黑
    bool allRed = true;
    bool allBlack = true;
    for (uint32_t i = 0; i < size; ++i)
    {
        if (s[i] != Suit::heart && s[i] != Suit::diamond)
            allRed = false;
        if (s[i] != Suit::spade && s[i] != Suit::club)
            allBlack = false;
    }
    if (allRed || allBlack)
        return G13SpecialBrand::redOrBlack;

    // check 5, 四个三条
    if ( (r[1]==r[3] && r[4]==r[6] && r[7]==r[9] && r[10]==r[12]) ||
         (r[0]==r[2] && r[4]==r[6] && r[7]==r[9] && r[10]==r[12]) ||
         (r[0]==r[2] && r[3]==r[5] && r[7]==r[9] && r[10]==r[12]) ||
         (r[0]==r[2] && r[3]==r[5] && r[6]==r[8] && r[10]==r[12]) ||
         (r[0]==r[2] && r[3]==r[5] && r[6]==r[8] &&  r[9]==r[11]) )
        return G13SpecialBrand::quradThreeOfKind;

    // check 4, 五对+三条
    uint32_t threeOfKindBegin = -1;
    for (uint32_t i = 0; i < size - 2; ++i)
    {
        if (r[i] == r[i + 1] && r[i + 1] == r[i + 2])
        {
            threeOfKindBegin = i;
            break;
        }
    }
    if (threeOfKindBegin != -1u)//除了这3张, 剩下的正好要配5对
    {
        bool isPentaPairsAndThreeOfKind = true;
        for (uint32_t i = 0; i < size - 1; )
        {
            if (threeOfKindBegin == i) //对子的第一张牌, 直接跳过这个3条
            {
                i += 3;
                continue;
            }
            uint32_t j = i + 1;
            if (j == threeOfKindBegin) //对子的第二张牌, 也直接跳过这个3条
                j += 3;

            if (r[i] != r[j])
            {
                isPentaPairsAndThreeOfKind = false;
                break;
            }
            i = j + 1;
        }
        if (isPentaPairsAndThreeOfKind)
            return G13SpecialBrand::pentaPairsAndThreeOfKind;
    }

    // check 3, 六对半
    uint32_t pairSize = 0; 
    for (uint32_t i = 0; i < size - 1; )
    {
        if (r[i] == r[i + 1]) //当前张和下一张放一起是对子
        {
            pairSize += 1;
            i += 2;
        }
        else
        {
            i += 1;
        }
    }
    if (pairSize == 6u)
        return G13SpecialBrand::sixPairs;

    // check 1 &  check 2, 直接取dun的值即可
    return dun;
}


}







/****************** 单元测试, 代码风格就不讲究了 ******************/
#ifdef DECK_UNIT_TEST

#include "deck.h"
#include <iostream>
#include <iomanip>
#include <functional>
#include <algorithm>

namespace deck_unit_test {
using namespace std;
using namespace placeholders;

using namespace lobby;


/*
void table()
{
    std::string suitsName[] = {"    ", "方块", "梅花",  "红桃", "黑桃"};
    std::string ranksName[] = {" ", "2", "3", "4", "5", "6", "7", "8", "9", "X", "J", "Q", "K", "A"};
    for (int s = 0; s < 5; ++s)
    {
        cout << suitsName[s] << ": ";
        if (s != 0)
        {
            for(int r = 1; r <= 13; ++r)
            {
                cout <<  setw(8) << ((s - 1) * 13 + r) << "   ";
            }
        }
        else
        {
            for(int r = 1; r <= 13; ++r)
            {
                cout <<  setw(8) <<  ranksName[r] << "   ";
            }
        }
        cout << endl;
    }
}
*/
/*
 *         :        2          3          4          5          6          7          8          9          X          J          Q          K          A   
 *
 *     方块:        1          2          3          4          5          6          7          8          9         10         11         12         13   
 *
 *     梅花:       14         15         16         17         18         19         20         21         22         23         24         25         26   
 *
 *     红桃:       27         28         29         30         31         32         33         34         35         36         37         38         39   
 *
 *     黑桃:       40         41         42         43         44         45         46         47         48         49         50         51         52 
 */

std::string suitsName[] = {"♦", "♣", "♥", "♠"};
std::string ranksName[] = {"2", "3", "4", "5", "6", "7", "8", "9", "X", "J", "Q", "K", "A"};


using Card = Deck::Card;
using H = std::vector<Card>;

string cardName(Card c)
{
    return suitsName[(c - 1)/13] + ranksName[(c - 1) % 13];
}

string readAbleH(const H& h)
{
    string ret;
    for(Card c : h)
        ret += (cardName(c) + " ");
    return ret;
}

string strH(const H& h)
{
    stringstream ss;
    for(Card c : h)
        ss << setw(2) << ((int)c) << " ";
    return ss.str();
}

string strBrandInfo(const Deck::BrandInfo& bi)
{
    stringstream ss;
    ss << (int)(bi.b) <<  ", " << bi.point;
    return ss.str();
}

string detailH(const H& h)
{
    string ret = strH(h) + "   " + readAbleH(h);
    return ret;
}



void showDeck()
{
    H all(52);
    for (int i = 0; i < 52; ++i)
        all[i] = i + 1;

    cout << readAbleH(all) << endl;
}


H dunArr[] = 
{
//    {6, 19, 32, 45, 45},  //9
//    {34, 35, 36 ,37, 38}, //8
//    {22, 23, 24, 25, 26}, //8
//    {27, 28 ,29, 30, 39}, //8
//    {3, 16, 29, 42, 1},   //7
//    {3, 16, 29, 42, 48},   //7
//    {1, 40, 11, 24, 50},  //6
//    {11, 24, 50, 52, 52},  //6
//    {29, 31, 35, 38, 33}, //5
//    {40, 15, 16, 30, 13}, //4
//    {35, 36 ,37, 25 ,39}, //4
//    {12, 11, 24, 50, 13},  //3
//    {1, 11, 24, 50, 13},  //3
//    {1, 45, 11, 24, 50},  //3
//    {14, 4, 30, 36, 49}, //2
//    {4, 30, 7, 36, 49}, //2
//    {4, 30, 36, 49, 12}, //2
//    {20, 33, 21, 35, 36}, //1
//    {40, 20, 33, 21, 35}, //1
//    {2, 42, 20, 33, 21}, //1
//    {1 , 41, 42, 20, 33}, //1
//    {1, 2, 3, 4, 37}, //0
//    {13,13,26}, //3
//    {1, 15, 27},          //1
//    {10, 37, 52}, //0
    {11, 24, 13}, //1
    {50, 37, 20}, //1
};

H allArr[] =
{
//    {27,28,29,30,31,32,33,34,35,36,37,38,39}, //青龙
    { 1,14, 2,15, 3,16, 4,43, 5,44,39,52,50}, //6对 + 1
    { 1,14, 2,15, 3,16, 4,43, 5,44,50,11,52}, //6对 + 1
    { 1,14,27,40, 3,16, 4,43, 5,13,26,39,52}, //6对 + 1
    { 1,14, 2,15, 3,16, 4,43, 5,44,39,52,13}, //5对 + 3条
    {14, 2,15, 3,16,17, 4,43, 5,40,44,39,52}, //5对 + 3条
    {14, 2,15, 3,16,17, 4,43, 5,40,44,39,52}, //5对 + 3条
    { 1, 2,15, 3,16,17,30, 4,43, 5,44,39,52}, //6对半,  (4条 + 3对 + 1)
    { 1, 2 ,4,15,17,18,21,23,44,50,51,52,43}, //3同花
    { 1, 2 ,3,16,17,18,19,20,47,48,49,50,51}, //3同花顺
    { 1, 2 ,16,16,17,18,19,20,47,48,49,50,51}, //3顺子
};


void testBrand()
{
    for (auto& h : dunArr)
    {
        cout << "--------------------------" << endl;
        cout << "收到: " << detailH(h) << endl;
        std::shuffle(h.begin(), h.end(), std::default_random_engine(::time(0)));
        cout << "洗牌: " << detailH(h) << endl;
        auto info = Deck::brandInfo(h.data(), h.size());
        cout << "整理: " << detailH(h) << endl;
        cout << "牌型:  " << (int)(info.b) <<  ", " << info.point << endl;
        Deck::cmpBrandInfo(info, info);
    }
}

void testAll()
{
    for(auto& h : allArr)
    {
        cout << "--------------------------" << endl;
        cout << "收到: " << detailH(h) << endl;
        auto d1 = Deck::brandInfo(h.data(), 3);
        auto d2 = Deck::brandInfo(h.data() + 3, 5);
        auto d3 = Deck::brandInfo(h.data() + 8, 5);
        auto s = Deck::g13SpecialBrand(h.data(), d2.b, d3.b);
        cout << "头墩: " << strBrandInfo(d1) << endl;
        cout << "中墩: " << strBrandInfo(d2) << endl;
        cout << "尾墩: " << strBrandInfo(d3) << endl;
        cout << "全序: " << detailH(h) << endl;
        cout << "特殊: " << (int)s << endl;
    }
}


}

int main()
{
    using namespace deck_unit_test;
    testAll();
    //testBrand();
    return 0;

}
#endif //#ifdef DECK_UNIT_TEST

