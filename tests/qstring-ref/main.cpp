#include <QtCore/QString>
#include <QtCore/QRegExp>
#include <QtCore/QStringList>
void test()
{
    bool ok = false;
    QString s;
    s.mid(1, 1).toInt(&ok); // Warning
    s.mid(1, 1); // OK
    s.toInt(&ok); // OK
    s.midRef(1, 1).toInt(&ok); // OK
    s.mid(s.lastIndexOf(QLatin1Char('#')) + 1).toUpper(); // OK
    s.mid(s.lastIndexOf(QLatin1Char('#')) + 1).trimmed(); // Warning
    const QRegExp r;
    QRegExp r2;
    s.mid(1, 1).indexOf(r); // OK
    s.mid(1, 1).indexOf(r2); // OK
    QString s2;
    s.append(s2.mid(1,1)); // Warning
    s.count(s2.left(1)); // Warning
    QStringList list;
    list.append(s2.mid(1,1)); // OK
    s += s2.mid(1,1); // Warning
    int start,end;
    int m = s.mid(start + 1, end - start - 1).toUInt(&ok); // Warning
}

QString i18n(const QString &) { return {}; }

void test2()
{
    QString s;
    s.append(i18n(s.mid(1, 1))); // OK
}
