#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QRegularExpression>

void test()
{
    if (QString("abc").contains(QRegularExpression("[a-z]"))) { // Warn
    }

    QString h = "hello";
    auto c = h.indexOf(QRegularExpression("[hel]"), 0);  // Warn

    QRegularExpression e("[a-z]");
    auto r = h.indexOf(e); // Warn

    static const QRegularExpression staticRegex("[a-z]");
    auto nr = h.indexOf(staticRegex); // Ok

    bool empty = QString().isEmpty();

    h.lastIndexOf(QRegularExpression("[hel]")); // Warn
    h.lastIndexOf(e); // Warn
    h.lastIndexOf(staticRegex); // Ok

    h.count(QRegularExpression("[hel]"));  // Warn
    h.count(e);  // Warn
    h.count(staticRegex); // Ok

    h.replace(QRegularExpression("[hel]"), QString()); // Warn
    h.replace(e, QString()); // Warn
    h.replace(staticRegex, QString()); // Ok

    h.remove(QRegularExpression("[hel]")); // Warn
    h.remove(e); // Warn
    h.remove(staticRegex); // Ok

    h.section(QRegularExpression("[hel]"), 0); // Warn
    h.section(e, 0); // Warn
    h.section(staticRegex, 0); // Ok

    h.split(QRegularExpression("[hel]"));  // Warn
    h.split(e);  // Warn
    h.split(staticRegex); // Ok

    h.splitRef(QRegularExpression("[hel]"));  // Warn
    h.splitRef(e);  // Warn
    h.splitRef(staticRegex); // Ok

    QStringList strList;

    strList.indexOf(e); // Warn
    strList.indexOf(staticRegex);

    strList.lastIndexOf(e); // Warn
    strList.lastIndexOf(staticRegex); // Ok
}
