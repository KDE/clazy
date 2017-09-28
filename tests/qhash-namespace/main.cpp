#include <QtCore/QString>
QT_BEGIN_NAMESPACE
struct A { };
struct E { };
uint qHash(A) { return 0; }; // OK
namespace NS {

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
    enum class EnumClass
    {
        One
    };
    uint qHash(EnumClass) { return 0; } // OK
}
uint qHash(NS::B) { return 0; } // Warn
uint qHash(NS::B *) { return 0; } // Warn
uint qHash(NS::B::B3) { return 0; }; // Warn


QT_END_NAMESPACE
uint qHash(E) { return 0; } // Warn
