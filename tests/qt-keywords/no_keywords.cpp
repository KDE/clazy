#define QT_NO_KEYWORDS
#include <QtCore/QObject>
#define emit test();

void test()
{
    emit // OK
}
