# mutex-detaching

This code checks that you do not call methods that might detach Qt containers within
areas of code that have a read-lock. Detaching those containers is a unforseen modification an may cause crashes.

Examples:

```cpp
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
