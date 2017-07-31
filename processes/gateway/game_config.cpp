#include "game_config.h"

#include "componet/logger.h"
#include "componet/xmlparse.h"
#include "componet/string_kit.h"


namespace gateway{

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
        XmlParseNode versionNode = root.getChild("version");
        if (!versionNode)
            EXCEPTION(LoadGameCfgFailedGW, "version node dose not exist");

        m_data.versionInfo.version = versionNode.getAttr<std::string>("version");
        m_data.versionInfo.appleReview = versionNode.getAttr<bool>("appleReview");
    }

    {
        XmlParseNode customServiceNode = root.getChild("customService");
        if (!customServiceNode)
            EXCEPTION(LoadGameCfgFailedGW, "customService node dose not exist");
        m_data.customService.wechat1 = customServiceNode.getAttr<std::string>("wechat1");
        m_data.customService.wechat2 = customServiceNode.getAttr<std::string>("wechat2");
        m_data.customService.wechat3 = customServiceNode.getAttr<std::string>("wechat3");
    }

    {
        XmlParseNode systemNoticeNode = root.getChild("systemNotice");
        if (!systemNoticeNode)
            EXCEPTION(LoadGameCfgFailedGW, "systemNotice node dose not exist");

        m_data.systemNotice.internalSec = systemNoticeNode.getAttr<time_t>("internalSec");

        for (XmlParseNode itemNode = systemNoticeNode.getChild("item"); itemNode; ++itemNode)
            m_data.systemNotice.texts.emplace_back(itemNode.getAttr<std::string>("text"));
        m_data.systemNotice.texts.shrink_to_fit();
    }

    {
        XmlParseNode pricePerPlayerNode = root.getChild("pricePerPlayer");
        if (!pricePerPlayerNode)
            EXCEPTION(LoadGameCfgFailedGW, "pricePerPlayer node dose not exist");

        m_data.pricePerPlayer.clear();
        for (XmlParseNode itemNode = pricePerPlayerNode.getChild("item"); itemNode; ++itemNode)
        {
            uint32_t rounds = itemNode.getAttr<uint32_t>("rounds");
            int32_t  money  = itemNode.getAttr<int32_t>("money");
            m_data.pricePerPlayer[rounds] = money;
        }

    }
    LOG_TRACE("load {} successed", configFile);
}

}


