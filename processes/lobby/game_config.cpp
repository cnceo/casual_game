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

void GameConfig::reload(const std::string& cfgDir /* = "" */)
{
    const std::string dir = (cfgDir != "") ? cfgDir : m_cfgDir;
    try
    {
        load(dir);
    }
    catch (const LoadGameCfgFailedGW& ex)
    {
        const std::string configFile = dir + "/game.xml";
        LOG_ERROR("reload {}, failed! {}", configFile, ex);
    }
}

void GameConfig::load(const std::string& cfgDir)
{
    using componet::XmlParseDoc;
    using componet::XmlParseNode;

    const std::string configFile = cfgDir + "/game.xml";
    LOG_TRACE("load game config, file={}", configFile);

    XmlParseDoc doc(configFile);
    XmlParseNode root = doc.getRoot();
    if (!root)
        EXCEPTION(LoadGameCfgFailedGW, configFile + "prase root node failed");

    uint32_t version = root.getAttr<uint32_t>("version");
    if (version <= m_version)
    {
        LOG_TRACE("load {}, ignored, version:{}->{}", configFile, m_version, version);
        return;
    }

    GameConfigData data;
    {
        XmlParseNode customServiceNode = root.getChild("customService");
        if (!customServiceNode)
            EXCEPTION(LoadGameCfgFailedGW, "customService node dose not exist");
        data.customService.wechat1 = customServiceNode.getAttr<std::string>("wechat1");
        data.customService.wechat2 = customServiceNode.getAttr<std::string>("wechat2");
        data.customService.wechat3 = customServiceNode.getAttr<std::string>("wechat3");
    }

    {
        XmlParseNode pricePerPlayerNode = root.getChild("pricePerPlayer");
        if (!pricePerPlayerNode)
            EXCEPTION(LoadGameCfgFailedGW, "pricePerPlayer node dose not exist");

        data.pricePerPlayer.clear();
        for (XmlParseNode itemNode = pricePerPlayerNode.getChild("item"); itemNode; ++itemNode)
        {
            uint32_t rounds = itemNode.getAttr<uint32_t>("rounds");
            int32_t  money  = itemNode.getAttr<int32_t>("money");
            data.pricePerPlayer[rounds] = money;
        }
    }

    do{
        data.testDeck.index = -1u;
        data.testDeck.decks.clear();
        XmlParseNode testDeckNode = root.getChild("testDeck");
        if (!testDeckNode) //测试配置, 可有可无, 不报错
            break;
        data.testDeck.index = testDeckNode.getAttr<uint32_t>("index");
        if (data.testDeck.index == -1u)
            break;
        for (XmlParseNode deckNode = testDeckNode.getChild("deck"); deckNode; ++deckNode)
        {
            std::vector<uint16_t> deck;
            componet::fromString(&deck, deckNode.getAttr<std::string>("cards"), ",");
            if (deck.size() < 65u)
                EXCEPTION(LoadGameCfgFailedGW, "testDeck, deck-{} size={}", data.testDeck.decks.size(), deck.size());
            data.testDeck.decks.push_back(std::move(deck));

        }
        if (data.testDeck.index >= data.testDeck.decks.size())
            EXCEPTION(LoadGameCfgFailedGW, "testDeck, index={}, decks.size={}", data.testDeck.index,  data.testDeck.decks.size());
    } while(false);

    {
        XmlParseNode shareByWeChatNode = root.getChild("shareByWeChat");
        if (!shareByWeChatNode)
            EXCEPTION(LoadGameCfgFailedGW, "shareByWeChat node dose not exist");

        data.shareByWeChat.begin  = shareByWeChatNode.getAttr<water::componet::TimePoint>("begin");
        data.shareByWeChat.end    = shareByWeChatNode.getAttr<water::componet::TimePoint>("end");
        data.shareByWeChat.awardMoney = shareByWeChatNode.getAttr<int32_t>("awardMoney");
        LOG_TRACE("load game cfg, shareByWeChat, begin={}, end={}, awardMoney={}", 
                  componet::timePointToString(data.shareByWeChat.begin),
                  componet::timePointToString(data.shareByWeChat.end),
                  data.shareByWeChat.awardMoney);
    }

    data = data;
    m_cfgDir = cfgDir;
    LOG_TRACE("load {}, successed, version:{}->{}", configFile, m_version, version);
    m_version = version;
}

}


