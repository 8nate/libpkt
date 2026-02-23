
#include "examples/hello_world/structure.h"


PKT_ALLOC_STATIC(HelloWorld);

int main(int argc, char **argv) {
    
    HelloWorld::bind_structure();
    HelloWorld instance;

    instance.set_a(3250);
    instance.set_b(3.141592654);
    instance.set_c("Hello World!");

    PKTByteVector serialized_data = HelloWorld::serialize(&instance);

    HelloWorld instance_copy;
    HelloWorld::deserialize(&serialized_data, &instance_copy);

    STR copied_str = instance_copy.get_c();
    printf("%s\n", copied_str.c_str());

    PKT_FREE_STATIC(HelloWorld);
}