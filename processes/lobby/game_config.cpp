#include "game_config.h"

#include "componet/logger.h"
#include "componet/xmlparse.h"
#include "componet/string_kit.h"


namespace lobby{

using namespace water;

GameConfig GameConfig::s_me;
GameConfig& GameConfig::me()
{
    return s_me;
}

void GameConfig::reload(const std::string& cfgDir)
{
    try
    {
        load(cfgDir);
    }
    catch (const LoadGameCfgFailedGW& ex)
    {
        const std::string configFile = cfgDir + "/game.xml";
        LOG_ERROR("reload {} failed! {}", configFile, ex);
    }
}

void GameConfig::load(const std::string& cfgDir)
{
    using componet::XmlParseDoc;
    using componet::XmlParseNode;

    const std::string configFile = cfgDir + "/game.xml";

    XmlParseDoc doc(configFile);
    XmlParseNode root = doc.getRoot();
    if (!root)
        EXCEPTION(LoadGameCfgFailedGW, configFile + "prase root node failed");

    {
        XmlParseNode customServiceNode = root.getChild("customService");
        if (!customServiceNode)
            EXCEPTION(LoadGameCfgFailedGW, "customService node dose not exist");
        m_data.customService.wechat1 = customServiceNode.getAttr<std::string>("wechat1");
        m_data.customService.wechat2 = customServiceNode.getAttr<std::string>("wechat2");
        m_data.customService.wechat3 = customServiceNode.getAttr<std::string>("wechat3");
    }

    {
        XmlParseNode pricePerPlayerNode = root.getChild("pricePerPlayer");
        if (!pricePerPlayerNode)
            EXCEPTION(LoadGameCfgFailedGW, "pricePerPlayer node dose not exist");

        //m_data.allRoomSize.clear();
        //const std::string& roomSizeListStr = pricePerPlayerNode.getAttr<std::string>("roomSizeList");
        //componet::fromString(&m_data.allRoomSize, roomSizeListStr, ",");

        m_data.pricePerPlayer.clear();
        for (XmlParseNode itemNode = pricePerPlayerNode.getChild("item"); itemNode; ++itemNode)
        {
            uint32_t rounds = itemNode.getAttr<uint32_t>("rounds");
            int32_t  money  = itemNode.getAttr<int32_t>("money");
            m_data.pricePerPlayer[rounds] = money;
        }
    }

    {
        m_data.testDeck.index = -1u;
        m_data.testDeck.decks.clear();
        XmlParseNode testDeckNode = root.getChild("testDeck");
        if (!testDeckNode) //测试配置, 可有可无, 不报错
            return;
        m_data.testDeck.index = testDeckNode.getAttr<uint32_t>("index");
        if (m_data.testDeck.index == -1u)
            return;
        for (XmlParseNode deckNode = testDeckNode.getChild("deck"); deckNode; ++deckNode)
        {
            std::vector<uint16_t> deck;
            componet::fromString(&deck, deckNode.getAttr<std::string>("cards"), ",");
            if (deck.size() < 65u)
                EXCEPTION(LoadGameCfgFailedGW, "testDeck, deck-{} size={}", m_data.testDeck.decks.size(), deck.size());
            m_data.testDeck.decks.push_back(std::move(deck));

        }
        if (m_data.testDeck.index >= m_data.testDeck.decks.size())
            EXCEPTION(LoadGameCfgFailedGW, "testDeck, index={}, decks.size={}", m_data.testDeck.index,  m_data.testDeck.decks.size());
    }

    {
        XmlParseNode shareByWeChatNode = root.getChild("shareByWeChat");
        if (!shareByWeChatNode)
            return;
        m_data.shareByWeChat.begin  = shareByWeChatNode.getAttr<water::componet::TimePoint>("begin");
        m_data.shareByWeChat.end    = shareByWeChatNode.getAttr<water::componet::TimePoint>("end");
        m_data.shareByWeChat.awardMoney = shareByWeChatNode.getAttr<int32_t>("awardMoney");
    }

    LOG_TRACE("load {} successed", configFile);
}

}


