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
            int32_t  price  = itemNode.getAttr<int32_t>("price");
            m_data.pricePerPlayer[rounds] = price;
        }
    }
    LOG_TRACE("load {} successed", configFile);
}

}

