#include <QtCore/QObject>
#include <QtCore/QString>

void test()
{
    QStringLiteral("foo");
    QStringLiteral("");
    QStringLiteral();

    if ("foo" == QStringLiteral()) {}
}
