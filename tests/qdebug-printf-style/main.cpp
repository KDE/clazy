#include <QtCore/QDebug>
#include <QtCore/QList>
#include <QtCore/QLoggingCategory>
#include <QtCore/QString>

Q_LOGGING_CATEGORY(lc, "my.category")

#define LOG qDebug()
#define MY_TEXT "macro text"

template<typename A, typename B>
QString makeString() { return QString(); }

struct S {
    int member = 0;
    QString name;
};

void test(const QString &str, int i, unsigned u, long l, qint64 ll, quint64 ull, short sh, double d, float f, bool b, char c, const char *cstr, const S &s)
{
    // Warn and fix
    qDebug() << "hello";
    qInfo() << "value" << i;
    qWarning() << "100%" << u;
    qCritical() << "name" << str << "count" << i;
    qDebug() << l << ll << ull << sh;
    qDebug() << d << f;
    qDebug() << b << (i > 1);
    qDebug() << c << cstr;
    qDebug() << s.member << s.name;
    qDebug() << "a \"quoted\" \t tab";
    qDebug() << "" << i;
    qDebug() << MY_TEXT << i;
    qDebug() << Q_FUNC_INFO;
    qDebug() << "a" "b";
    qDebug() << "\045d" << i;
    qDebug() << makeString<int, int>();
    qDebug() << i + 1 << str.size();
    qCDebug(lc) << "category" << i;
    qCWarning(lc) << str;
    if (b)
        qDebug() << "in if";

    // Don't warn
    qDebug() << QList<int>{1, 2};
    qDebug() << "list" << QList<int>{1, 2};
    qDebug().nospace() << "nospace" << i;
    qDebug().noquote() << str;
    qDebug() << &s;
    qDebug() << QStringView(str);
    qDebug() << QByteArray("x");
    qDebug() << "nul\0byte";
    LOG << "wrapped";
    qDebug("already %d", i);
    qDebug();
    QDebug dbg = qDebug() << "kept";
    dbg << i;
    qDebug() << i << Qt::endl;
}

int next();
struct Meters {
    double v;
    operator double() const { return v; }
};
struct CStr {
    const char *p;
    operator const char *() const { return p; }
};
struct BitFields {
    long x : 3;
    unsigned long long y : 5;
};

QDebug &returnsDebug()
{
    return qDebug() << "returned"; // Don't warn, the result is used
}

void test2(int i, const QList<int> &list, size_t sz, ptrdiff_t pd, qsizetype qs, int64_t i64, quint64 qu64, long l, Meters m, CStr cs, BitFields bf, const QString &str)
{
    // Warn and fix
    qDebug() << "next" << next(); // Only one operand changes state, the others are constants
    qDebug() << "size" << list.size() << i; // const member functions don't change state
    qDebug() << sz << pd << qs << i64 << qu64 << l;
    qDebug() << str.size() << list.count();

    // Don't warn
    qDebug() << next() << next(); // Evaluation order
    qDebug() << i << ++i; // Evaluation order
    qDebug() << i << next(); // next() could change i
    qDebug() << m; // User-defined conversion
    qDebug() << cs; // User-defined conversion
    qDebug() << bf.x << bf.y; // Bit-fields are promoted to int
    auto &ref = qDebug() << "ref";
    qDebug() << "a"
#ifdef SOMETHING
             << i
#endif
             << "b";
    qDebug() << "commented" // a comment
             << i;
}
