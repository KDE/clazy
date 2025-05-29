# readlock-detaching

This check verifies that you do not call methods that might detach Qt containers within
areas of code that have a read-lock. Detaching those containers is a unforseen modification and may cause crashes.

Examples:

```cpp
QReadWriteLock m_projectLock;
QMap<QString, QString> m_fileToProjectParts;
{
    QReadLocker locker(&m_projectLock);
    auto it = m_fileToProjectParts.find(fileName); // WARN, possible detach
}

{
    QReadLocker locker(&m_projectLock);
    auto it = m_fileToProjectParts.find(fileName); // WARN, possible detach
    locker.unlock();
    it = m_fileToProjectParts.find(fileName); // OK, we unlocked above
}

{
    auto it = m_fileToProjectParts.find(fileName); // OK, we did not lock yet
    m_readWriteLock.lockForRead();
    it = m_fileToProjectParts.find(fileName); // WARN, inside of read-only lock
    m_readWriteLock.unlock();
}

```

Note: This check only works properly for QReadWriteLock if the unlocking is done in the same level as the locking was done.
Conditions of if-statements are for example not considered.
