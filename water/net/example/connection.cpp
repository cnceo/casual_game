#include "test.h"
#include "../connection.h"

#include <thread>

using namespace water;
using namespace net;

int main()
{
    cout << EINPROGRESS << endl;
    Endpoint ep("10.173.32.52:8000");
    auto conn = TcpConnection::create(ep);
    int i = 0;
    while (!conn->tryConnect())
    {
        ++i;
    }
    cout << "try " << i << " times before connected" << endl;

    std::string request =
    "GET / HTTP/1.1\r\n"
    "Host: 10.173.32.52:8088\r\n"
 //   "User-Agent: curl/7.47.0\r\n"
  //  "Accept: */*\r\n"
    "\r\n";
    char buf[1024] = {0};
    conn->setBlocking();
    conn->send(request.data(), request.size());
    this_thread::sleep_for(std::chrono::seconds(1)); //
    conn->recv(buf, 1024);
    cout << buf << endl;
    return 0;
}
