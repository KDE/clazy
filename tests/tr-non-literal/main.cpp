#include <QtCore/QString>
#include <QtCore/QObject>

extern void takesString(QString);
extern const char* returnsChar();

void test()
{
    QString s;
    QObject::tr("test");     // OK
    QObject::tr("");         // OK
    QObject::tr(s.toUtf8()); // Warn
}

class MyObject : public QObject
{
public:
    void test()
    {
        takesString(tr("hello"));     // OK
        takesString(tr(returnsChar())); // Warning
    }
};
