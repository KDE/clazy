#include <QtCore/qglobal.h>

namespace NS {
    typedef int IntFoo;
}
uint qHash(NS::IntFoo) { return 0; }
