/*
 * Author: LiZhaojia - waterlzj@gmail.com
 *
 * Last modified: 2017-06-24 13:22 +0800
 *
 * Description: 
 */


#include <memory>



class Client;
using ClientPtr = std::shared_ptr<Client>;

class MysqlHandler
{
public:
    bool saveClient(ClientPtr);
    ClientPtr loadClient();
private:
    bool loadConfig();
};
