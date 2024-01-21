#include <QtCore/QString>
#include <QtCore/QRegularExpression>


void test()
{
    QRegularExpression e("[a-z]");
    QString h = "hello";
    static const QRegularExpression staticRegex("[a-z]");

    h.splitRef(QRegularExpression("[hel]")); // Warn
    h.splitRef(e); // Warn
    h.splitRef(staticRegex); // Ok
}
