#include <QtCore/QString>
#include <QtCore/QRegExp>

void test()
{
    bool ok = false;
    QString s;
    s.mid(1, 1).toInt(&ok); // Warning
    s.mid(1, 1); // OK
    s.toInt(&ok); // OK
    s.midRef(1, 1).toInt(&ok); // OK
    s.mid(s.lastIndexOf(QLatin1Char('#')) + 1).toUpper(); // OK
    s.mid(s.lastIndexOf(QLatin1Char('#')) + 1).trimmed(); // OK
    const QRegExp r;
    QRegExp r2;
    s.mid(1, 1).indexOf(r); // OK
    s.mid(1, 1).indexOf(r2); // OK
}
