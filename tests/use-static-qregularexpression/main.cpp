#include <QtCore/QRegularExpression>
#include <QtCore/QString>

void test()
{
    if (QString("abc").contains(QRegularExpression("[a-z]"))) { // Warn
    }

    QString h = "hello";
    auto c = h.indexOf(QRegularExpression("[hel]"), 0); // Warn

    QRegularExpression e("[a-z]");
    auto r = h.indexOf(e); // Warn

    static const QRegularExpression staticRegex("[a-z]");
    auto nr = h.indexOf(staticRegex); // Ok

    bool empty = QString().isEmpty();

    h.lastIndexOf(QRegularExpression("[hel]")); // Warn
    h.lastIndexOf(e); // Warn
    h.lastIndexOf(staticRegex); // Ok

    h.count(QRegularExpression("[hel]")); // Warn
    h.count(e); // Warn
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

    h.split(QRegularExpression("[hel]")); // Warn
    h.split(e); // Warn
    h.split(staticRegex); // Ok

    QStringList strList;

    strList.indexOf(e); // Warn
    strList.indexOf(staticRegex); // Ok

    strList.lastIndexOf(e); // Warn
    strList.lastIndexOf(staticRegex); // Ok

    QString regexStr = "[abc]";
    h.contains(QRegularExpression(regexStr)); // Warn

    {
        QRegularExpression reg(regexStr);
        h.contains(reg); // Warn
    }
    {
        static const QRegularExpression reg(regexStr);
        h.contains(reg); // Ok
    }
}

void test1(const QString &regex, QString toCheck)
{
    QRegularExpression re(regex);
    toCheck.contains(re); // Ok
}

void test2(const QStringList &regexes, QString toCheck)
{
    for (const auto &regix : regexes) {
        toCheck.contains(QRegularExpression(regix)); // Ok, no warn
    }
}

void test_nocrash()
{
    QRegularExpression re;
    QString s;
    s.contains(re);
}

#include <QtCore/QVariant>
void test_qregexmatch(QString selectedText)
{
    QRegularExpression weekRE("(?<week>");
    auto match1 = weekRE.match(selectedText);
    auto match2 = weekRE.globalMatch(selectedText);

    auto m1 = QRegularExpression("[123]").match(selectedText);
    auto m2 = QRegularExpression("[123]").globalMatch(selectedText);

    QVariant v;
    v.toRegularExpression().match(selectedText); // No Warn
}

extern QString someText();
void test_qregexmatch_rvalueString(QString s)
{
    // XValue + RValue arg
    QString text;
    QRegularExpression(someText()).match(text); // Ok

    // LValue + RValue arg
    QRegularExpression re(someText());
    re.match(text);

    QRegularExpression re1("^" + someText());
    re1.match(text);

    QRegularExpression re2("^" + s);
    re2.match(text);

    QRegularExpression re3((QString(s)));
    re3.match(text);
}

void testArgs()
{
    {
        QString("bla").contains(QRegularExpression(QString("test%1").arg("123"))); // OK
    }
    {
        QRegularExpression reg(QString("test%1").arg("123"));
        QString().contains(reg); // OK
    }
    {
        QString pattern = QString("test%1").arg("123");
        QRegularExpression reg(pattern);
        QString().contains(reg); // OK
    }
    {
        QString pattern = QString("someteststring").mid(1, 2);
        QRegularExpression reg(pattern);
        QString().contains(reg); // OK
    }
}
