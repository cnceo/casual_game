#include "auto.h"
#include "client.h"
#include "componet/logger.h"
#include "coroutine/coroutine.h"
#include "protocol/protobuf/public/client.codedef.h"


using namespace PublicProto;




void AutoActions::init()
{
    corot::create(std::bind(&AutoActions::start, this));
}

void AutoActions::start()
{
    C_Login s;
    s.set_login_type(LOGINT_WETCHAT);
    s.set_openid("testrobot00335");
    s.set_token("xxxx");
    SEND_MSG(C_Login, s);

    auto r = RECV_MSG(S_LoginRet);
    LOG_TRACE("login successful, <{}, {}, {}>", r->ret_code(), r->cuid(), r->temp_token());
}

