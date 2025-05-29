#include <QtCore/QObject>
#include <QtCore/QString>
#include <QtCore/QMap>
#include <QtCore/QReadWriteLock>


class Test
{
    QReadWriteLock m_projectLock;
    QMap<QString, QString> m_fileToProjectParts;
    QMap<QString, QString> m_someOtherMap;

    void test(const QString &fileName)
    {
        m_someOtherMap.find(fileName); // OK, we did not pretend to lock anything here...
        QReadLocker locker(&m_projectLock);
        auto it = m_fileToProjectParts.find(fileName); // WARN, possible detach
        auto lookup = m_fileToProjectParts[fileName]; // WARN, possible detach
        Q_UNUSED(lookup);
        auto it3 = QMap<QString, QString>().find(fileName); // OK, not a member
    }

    void test2(const QString &fileName)
    {
        QWriteLocker locker(&m_projectLock);
        auto it = m_fileToProjectParts.find(fileName); // OK, we have a write lock
    }

    void test3(const QString &fileName)
    {
        {
            QReadLocker locker(&m_projectLock);
            auto it = m_fileToProjectParts.constFind(fileName); // OK - const version of this function
        }

        auto it = m_fileToProjectParts.find(fileName); // OK, outside of locker
    }

    void testUnlock(const QString &fileName)
    {
        QReadLocker locker(&m_projectLock);
        auto it = m_fileToProjectParts.find(fileName); // WARN
        locker.unlock();
        it = m_fileToProjectParts.find(fileName); // OK, we unlocked
    }

    void testLockUnlock(const QString &fileName)
    {
        auto it = m_fileToProjectParts.find(fileName); // OK, we did not lock yet
        m_projectLock.lockForRead();
        it = m_fileToProjectParts.find(fileName); // WARN, inside of read-only lock
        m_projectLock.unlock();
        it = m_fileToProjectParts.find(fileName); // OK, we unlocked
    }
};
