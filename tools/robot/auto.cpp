#include "auto.h"
#include "client.h"
#include "componet/logger.h"
#include "coroutine/coroutine.h"
#include "protocol/protobuf/public/client.codedef.h"


using namespace PublicProto;


void msgBox()
{
    while (true)
    {
        auto msgbox = RECV_MSG(S_Notice);
        LOG_TRACE("MSG_BOX: {}", msgbox->text());
        corot::this_corot::yield();
    }
}

void msg_S_G13_PlayersInRoom()
{
    while (true)
    {
        auto msg = RECV_MSG(S_G13_PlayersInRoom);
        LOG_TRACE("S_G13_PlayersInRoom:");
        corot::this_corot::yield();
    }
}

void msg_S_G13_HandOfMine()
{
    auto msg = RECV_MSG(S_G13_HandOfMine);
    std::string cards;
    for (int i = 0; i < msg->cards_size(); ++i)
    {
        cards.append(std::to_string(msg->cards(i)));
        cards.append(", ");
    }
    LOG_TRACE("handOfMine, [{}]", cards);
}

void AutoActions::init()
{
    corot::create(std::bind(&AutoActions::start, this));
    corot::create(msgBox);
    corot::create(msg_S_G13_PlayersInRoom);
    corot::create(msg_S_G13_HandOfMine);
}

struct Self
{
    uint64_t cuid = 0;
};
static Self self;


void AutoActions::start()
{
    C_Login c_login;
    c_login.set_login_type(LOGINT_WETCHAT);
    c_login.set_openid("testrobot4xxxx");
    c_login.set_token("xxxx");
    c_login.set_nick_name("testrobot4");
    SEND_MSG(C_Login, c_login);

    auto r = RECV_MSG(S_LoginRet);
    self.cuid = r->cuid();
    LOG_TRACE("login successful, <{}, {}, {}>", r->ret_code(), r->cuid(), r->temp_token());

    //建房间
    C_G13_CreateGame c_crtgm;
    c_crtgm.set_player_size(4);
    c_crtgm.set_play_type(52);
    c_crtgm.set_rounds(12);
    c_crtgm.set_payor(10);
    c_crtgm.set_da_qiang(2);
    c_crtgm.set_quan_lei_da(true);
    c_crtgm.set_yi_tiao_long(2);
    SEND_MSG(C_G13_CreateGame, c_crtgm);

    auto loginRet = RECV_MSG(S_G13_RoomAttr);
    LOG_TRACE("RECVED S_G13_RoomAttr, roomid={}, bankerCuid={}", loginRet->room_id(), loginRet->banker_cuid());

    //离开房间
    if (false)
    {
        C_G13_GiveUp  c_giveUp;
        SEND_MSG(C_G13_GiveUp, c_giveUp);

        auto playerQuite = RECV_MSG(S_G13_PlayerQuited);
        LOG_TRACE("RECVED, S_G13_PlayerQuited, rev->cuid={}, selfcuid={}", playerQuite->cuid(), self.cuid);
    }

    //确认准备就绪
    C_G13_ReadyFlag c_readyFlag;
    c_readyFlag.set_ready(true);
    SEND_MSG(C_G13_ReadyFlag, c_readyFlag);
}

