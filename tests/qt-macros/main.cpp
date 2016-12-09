#ifdef Q_OS_WIN // Warning: testing before including qglobal.h
#endif

#if defined(Q_OS_WIN) // Warning: testing before including qglobal.h
#endif

#include <QtCore/QString>

#if defined(Q_OS_WINDOWS) // Warning, should be Q_OS_WIN
#endif

#ifdef Q_OS_WINDOWS // Warning, should be Q_OS_WIN
#endif

#ifdef Q_OS_WIN // OK
#endif

#if defined(Q_OS_WIN) // OK
#endif
