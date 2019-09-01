#ifdef Q_OS_WIN // Warning: testing before including qglobal.h
#endif

#if defined(Q_OS_WIN) // Warning: testing before including qglobal.h
#endif

#include <QtCore/QString>

#ifdef Q_OS_WIN // OK
#endif

#if defined(Q_OS_WIN) // OK
#endif
