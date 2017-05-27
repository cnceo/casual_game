#include "cpp_redis/cpp_redis"
#include <iostream>
#include <string.h>

using std::cout;
using std::cerr;
using std::cin;
using std::endl;


const char digit_pairs[201] = {
  "00010203040506070809"
  "10111213141516171819"
  "20212223242526272829"
  "30313233343536373839"
  "40414243444546474849"
  "50515253545556575859"
  "60616263646566676869"
  "70717273747576777879"
  "80818283848586878889"
  "90919293949596979899"
};


std::string& itostr(int n, std::string& s)
{
    if(n==0)
    {
        s="0";
        return s;
    }

    int sign = -(n<0);
    unsigned int val = (n^sign)-sign;

    int size;
    if(val>=10000)
    {
        if(val>=10000000)
        {
            if(val>=1000000000)
                size=10;
            else if(val>=100000000)
                size=9;
            else 
                size=8;
        }
        else
        {
            if(val>=1000000)
                size=7;
            else if(val>=100000)
                size=6;
            else
                size=5;
        }
    }
    else 
    {
        if(val>=100)
        {
            if(val>=1000)
                size=4;
            else
                size=3;
        }
        else
        {
            if(val>=10)
                size=2;
            else
                size=1;
        }
    }
    size -= sign;
    s.resize(size);
    char* c = &s[0];
    if(sign)
        *c='-';

    c += size-1;
    while(val>=100)
    {
       int pos = val % 100;
       val /= 100;
       *(short*)(c-1)=*(short*)(digit_pairs+2*pos); 
       c-=2;
    }
    while(val>0)
    {
        *c--='0' + (val % 10);
        val /= 10;
    }
    return s;
}

std::string& itostr(unsigned val, std::string& s)
{
    if(val==0)
    {
        s="0";
        return s;
    }

    int size;
    if(val>=10000)
    {
        if(val>=10000000)
        {
            if(val>=1000000000)
                size=10;
            else if(val>=100000000)
                size=9;
            else 
                size=8;
        }
        else
        {
            if(val>=1000000)
                size=7;
            else if(val>=100000)
                size=6;
            else
                size=5;
        }
    }
    else 
    {
        if(val>=100)
        {
            if(val>=1000)
                size=4;
            else
                size=3;
        }
        else
        {
            if(val>=10)
                size=2;
            else
                size=1;
        }
    }

    s.resize(size);
    char* c = &s[size-1];
    while(val>=100)
    {
       int pos = val % 100;
       val /= 100;
       *(short*)(c-1)=*(short*)(digit_pairs+2*pos); 
       c-=2;
    }
    while(val>0)
    {
        *c--='0' + (val % 10);
        val /= 10;
    }
    return s;
}

thread_local cpp_redis::future_client client;

int main()
{
    client.connect("127.1.1.1", 6379, 
                   [](cpp_redis::redis_client&){ cout << "redis client distconnected" << endl; } );

    const int N = 1;//000000;
    int data[N];

    std::vector<cpp_redis::future_client::future> sets;
    std::vector<cpp_redis::future_client::future> gets;
    sets.reserve(N);
    gets.reserve(N);

    std::string name;
    std::string value;
	for(int i = 0; i < N; ++i)
	{
        itostr(i, name);
        itostr(i + 1, value);
        cout << "set and get, {" << name        << ',' << value        << "}" << endl;
        cout << "set and get, {" << name.size() << ',' << value.size() << "}" << endl;
		sets.push_back(std::move(client.set(name, "1")));
		gets.push_back(std::move(client.get(name)));
    }
    client.commit();


    cout << "waiting ... " << endl;
    bool readyAll = true;
    do{
        readyAll = true;
        for(int i = 0; i < N; ++i)
        {
            if ( (sets[i].wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) &&
                 (gets[i].wait_for(std::chrono::milliseconds(0)) == std::future_status::ready) )
            {
                auto ret = gets[i].get();
                cout << "get ret, type=" << ret.get_type_str() << ", value=" << ret << endl;
//                data[i] = gets[i].get().as_integer();
                continue;
            }
            readyAll = false;
        }
    } while(!readyAll);

    cout << "checking" << endl;
    for(int i = 0; i < N; ++i)
    {
        if(data[i] != i + 1)
            cerr << "err, checking failed, data[" << i << "]=" << data[i] << endl;
    }
    cout << "check done" << endl;

    
    return 0;
}



/*
    std::future_status status;
    do{
        status = setFuture.wait_for(std::chrono::milliseconds(1));
        if (status == std::future_status::deferred)
            cout << "set no deferedd" << endl;
        else if(status == std::future_status::timeout)
            cout << "waiting set ready ..." << endl;
    } while( status != std::future_status::ready );
    */
