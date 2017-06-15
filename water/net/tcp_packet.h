/*
 * Author: LiZhaojia 
 *
 * Last modified: 2014-12-05 10:35 +0800
 *
 * Description:  重写addCursor，来实现tcp数据的格式化
 *               tcp包格式：
 *
 *               起始地址              |  内容                  |    长度
 *               ----------------------+------------------------+---------------------
 *               0                     |-- SizeType  dataSize --|   sizeof(SizeType)
 *               0+sizeof(SizeType)    |--   uint8_t[] data   --|   *(SizeType*)(data)
 */

#include "packet.h"

namespace water{
namespace net{

class TcpPacket : public Packet
{
public:
    TYPEDEF_PTR(TcpPacket)
    CREATE_FUN_MAKE(TcpPacket)

    explicit TcpPacket();
    explicit TcpPacket(SizeType contentSize);
    void setContent(const void* content, SizeType contentSize);
    void* content();
    SizeType contentSize() const;

public:
    static TcpPacket::Ptr tryParse(uint8_t* data, SizeType size);
};

}}
