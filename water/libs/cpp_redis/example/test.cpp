#include "cpp_redis/cpp_redis"
#include <iostream>
#include <string.h>

using std::cout;
using std::cin;
using std::endl;

int main()
{
	//! Enable logging
	cpp_redis::active_logger = std::unique_ptr<cpp_redis::logger>(new cpp_redis::logger);

	cpp_redis::redis_client client;

	client.connect("127.0.0.1", 6379, [](cpp_redis::redis_client&) {
				   std::cout << "client disconnected (disconnection handler)" << std::endl;
				   });

	// same as client.send({ "GET", "hello" }, ...)
    struct ST
    {
        int i = 31412;
        double d = 2.22;
    };
    ST st;
    std::string binData((const char*)(&st), sizeof(st));
    ST st1;
    cout << "binData.size(): " << binData.size() << endl;
    cout << "sizeofST: " << sizeof(ST) << endl;
    binData.copy((char*)&st1, sizeof(st1));
    cout << "deserialize st1, size=" << binData.size() << " {" << st1.i << ", " << st1.d << "}" << endl;

    client.send({"hset", "sttable", "st", binData}, [](cpp_redis::reply& replay){cout << "serialize st1, set replay: " << replay << endl;});

	client.commit();

    auto parseBin = [](cpp_redis::reply& replay)
    {
        //cout << "bin replay, get_type: " << replay.get_type() << endl;
        cout << "bin replay, is_bulk_string: " << replay.is_bulk_string() << endl;
        cout << "bin replay, is_string: " << replay.is_string() << endl;
        cout << "bin replay, is_simple_string:" << replay.is_simple_string() << endl;

        ST tmp;
        replay.as_string().copy((char*)&tmp, sizeof(tmp));
        cout << "desierliers replay: {" << tmp.i << ", " << tmp.d << "}" << endl;
    };
    client.hget("sttable", "st", parseBin);

    client.commit();

    std::this_thread::sleep_for(std::chrono::seconds(1));


	return 0;
}
