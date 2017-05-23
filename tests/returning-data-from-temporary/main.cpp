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
