#include <QtCore/QObject>
#include <QtCore/QString>
#include "ui_qstringliteral.h" // OK
void test()
{
    QStringLiteral("foo");
    QStringLiteral("");
    QStringLiteral();

    if ("foo" == QStringLiteral()) {}
}
