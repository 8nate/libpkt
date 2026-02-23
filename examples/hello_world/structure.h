
#include "libpkt.h"


class HelloWorld {

    // To enable serialization on a class, we must call PKT_STRUCT()
    PKT_STRUCT(HelloWorld);

    // Create serial properties with ease using the PKT_CREATE_PROPERTY macro
    // Properties are private members and cannot be directly interacted with, however, PKT_CREATE_PROPERTY() creates
    // a public setter and getter function for the property 
    PKT_CREATE_PROPERTY(I32, a);    // creates a 32-bit signed integer called 'a'.
    PKT_CREATE_PROPERTY(F64, b);    // creates a 64-bit floating point unit called 'b'.
    PKT_CREATE_PROPERTY(STR, c);    // creates a string called 'c'.

    // _bind_structure() is called during the static binding of the class. It binds the setter and getter methods for the serializer,
    // and stores the necessary byte layout information for reconstructing the data.
    static void _bind_structure() {
        PKT_SLOT(0x00);                     // Sets the current bound slot index to 0 (slots are written in hexadecimal)
        PKT_BIND_I32(HelloWorld, a);        // Creates a ByteProcessor in slot 0 pointed to the property 'a' 
        PKT_SLOT(0x01);
        PKT_BIND_F64(HelloWorld, b);
        PKT_SLOT(0x02);
        PKT_BIND_STR(HelloWorld, c);
    }
};