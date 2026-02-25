
#include "examples/sub_structures/structure.h"


PKT_ALLOC_STATIC(SubTest);

PKT_SUB_ALLOC_STATIC(Vec2);
PKT_SUB_ALLOC_STATIC(Peer);


int main(void) {
    Vec2::bind_structure();
    Peer::bind_structure();
    SubTest::bind_structure();

    SubTest obj, obj2;

    Peer p, p2;
    p.set_id(2238154);
    p.set_port(65200);
    p.set_timestamp(12934582212);
    p.set_position(Vec2(25, 10));
    
    p2.set_id(8593511);
    p2.set_port(4200);
    p2.set_timestamp(0);
    p2.set_position(Vec2(-388, -4115.5));
    
    obj.get_peers_vec()->push_back(p);
    obj.get_peers_vec()->push_back(p2);

    PKTByteVector to;
    to = SubTest::serialize(&obj);
    to.print_hex_table();

    SubTest::deserialize(&to, &obj2);

    for (Peer &p : *obj2.get_peers_vec()) {
        Vec2 pos = p.get_position();
        printf("(%d,%d,%llu,(%f,%f))\n", p.get_id(), p.get_port(), p.get_timestamp(), pos.get_x(), pos.get_y());
    }
    
    PKT_FREE_STATIC(SubTest);
    PKT_FREE_STATIC(Vec2);
}