#include <QtCore/QString>
#include <QtCore/QByteArray>

QByteArray returnsByteArray()
{
    return QByteArray();
}

const QByteArray returnsConstByteArray()
{
    return QByteArray();
}

struct A
{
    QByteArray member;
    const char * returnsFromMember()
    {
        return member.data(); // OK
    }

    const char * returnsFromMember2()
    {
        return member.constData(); // OK
    }
};

const char * returnsFromLocal()
{
    QByteArray b;
    return b.data(); // Warn
}

const char * returnsFromLocal2()
{
    QByteArray b;
    return b.constData(); // Warning
}

const char * returnsFromConstLocal()
{
    const QByteArray b;
    return b.data(); // OK
}

const char * returnsFromConstLocalPtr()
{
    const QByteArray *b;
    return b->data(); // OK
}

const char * returnsFromConstLocalPtr2()
{
    QByteArray *const b = nullptr;
    return b->data(); // Warn
}

const char * returnsFromConstLocal2()
{
    const QByteArray b;
    return b.constData(); // OK
}

const char * returnsFromStaticLocal()
{
    static QByteArray b;
    return b.data(); // OK
}

const char * returnsFromStaticLocal2()
{
    static QByteArray b;
    return b.constData(); // OK
}

const char * returnsFromTemporary()
{
    return returnsByteArray().data(); // Warn
}

const char * returnsFromTemporary2()
{
    return returnsConstByteArray().data(); // Ok
}

const char * returnsFromTemporary3()
{
    QString s;
    return s.toUtf8().constData(); // Warn
    return s.toUtf8().data(); // Warn
    return s.toLatin1().data(); // Warn
    return s.toLatin1().constData(); // Warn
}

const char * returnsByteArrayCasted()
{
    return returnsByteArray(); // Warn
}

QString getString() { return QString(); }
const char * returnsFromTemporary4()
{
    return getString().toUtf8().constData(); // Warn
    return getString().toUtf8().data(); // Warn
    return getString().toLatin1().data(); // Warn
    return getString().toLatin1().constData(); // Warn
}

QByteArray castBackToByteArray()
{
    QByteArray b;
    return b.data(); // OK
    return getString().toUtf8().constData(); // OK
    return getString().toUtf8().data(); // OK
    return getString().toLatin1().data(); // OK
    return getString().toLatin1().constData(); // OK
}


void testAssignment()
{
    QByteArray b;
    QString str;
    const char *c1 = b.data(); // OK
    const char *c2 = b.constData(); // OK
    const char *c3 = b; // OK
    const char *c4 = str.toUtf8().data(); // Warn
    const char *c5 = str.toLatin1().data(); // Warn
    const char *c6 = str.toUtf8().constData(); // Warn
    const char *c7 = str.toLatin1().constData(); // Warn
    const char *c8 = str.toUtf8(); // Warn
    const char *c9 = str.toLatin1(); // Warn
    const char *c10 = returnsByteArray(); // Warn
    const char* buffer = QByteArray("Hello").constData(); // Warn
}

const char * testByParam(QByteArray &ba, QString &foo, QByteArray ba2)
{
    return ba.data(); // OK
    return ba.constData(); // OK
    return ba; // OK
    return ba2; // Warn
    return foo.toLatin1().data(); // Warn
}
