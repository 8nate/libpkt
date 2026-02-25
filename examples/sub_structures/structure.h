#pragma once
#include "libpkt.h"


class Vec2 {
    PKT_SUB(Vec2);
    PKT_CREATE_PROPERTY_D(F32, x, 0);   // 4b
    PKT_CREATE_PROPERTY_D(F32, y, 0);   // 4b

    inline Vec2() { }
    inline Vec2(const F32 &p_x, const F32 &p_y) : _prop_x(p_x), _prop_y(p_y) { }

    static void _bind_structure() {
        PKT_SUB_BYTES(8);
        PKT_SLOT(0x00);
        PKT_BIND_F32(Vec2, x);
        PKT_SLOT(0x01);
        PKT_BIND_F32(Vec2, y);
    }
};

class Peer {
    PKT_SUB(Peer);
    PKT_CREATE_PROPERTY_D(U32, id, 0);  // 4b
    PKT_CREATE_PROPERTY_D(U16, port, 0); // 2b
    PKT_CREATE_PROPERTY_D(U64, timestamp, 0);   // 8b
    PKT_CREATE_SUBPACK_PROPERTY(Peer, Vec2, position);  // 8b


    static void _bind_structure() {
        PKT_SUB_BYTES(22);
        PKT_SLOT(0x00);
        PKT_BIND_U32(Peer, id);
        PKT_SLOT(0x01);
        PKT_BIND_U16(Peer, port);
        PKT_SLOT(0x02);
        PKT_BIND_U64(Peer, timestamp);
        PKT_SLOT(0x03);
        PKT_BIND_SUBPACK_PROPERTY(Peer, position, Vec2::struct_size_static());
    }
};

class SubTest {
    PKT_STRUCT(SubTest);
    PKT_CREATE_SUBPACK_PROPERTY_VEC(SubTest, Peer, peers);

    static void _bind_structure() {
        PKT_SLOT(0x00);
        PKT_BIND_SUBPACK_PROPERTY_VEC(SubTest, peers, Peer::struct_size_static());
    }
};