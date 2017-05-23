/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-12-03 10:46 +0800
 *
 * Description:  场景逻辑处理服
 */

#include "interior.h"
#include "base/shell_arg_parser.h"
#include "protocol/protobuf/proto_manager.h"

#include <csignal>

int main(int argc, char* argv[])
{
    using namespace interior;

    water::process::ShellArgParser arg(argc, argv);

//    protocol::protobuf::ProtoManager::me().loadConfig(arg.configDir());

    interior::Interior::init(arg.num(), arg.configDir(), arg.logDir());
    interior::Interior::me().start();
}
