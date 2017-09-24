#include <QtCore/QString>

struct A { };
uint qHash(A) { return 0; }; // OK

namespace NS {
    typedef int IntFoo;
    struct B { struct B2 {}; struct B3 {}; };
    uint qHash(B) { return 0; }; // OK
    uint qHash(B::B2) { return 0; }; // OK
    namespace NS2 {
        struct C {};
        uint qHash(C) { return 0; }; // OK
    }
    uint qHash(NS2::C) { return 0; }; // Warn
    uint qHash(::A) { return 0; } // Warn
    uint qHash(B::B3) { return 0; }; // OK
}
uint qHash(NS::B) { return 0; } // Warn
uint qHash(NS::B *) { return 0; } // Warn
uint qHash(NS::B::B3) { return 0; }; // Warn
uint qHash(NS::IntFoo) { return 0; }
