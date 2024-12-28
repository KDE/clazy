#include <QtQml/qqml.h>
#include <QtQml/qqmlmoduleregistration.h>

#if !defined(QT_STATIC)
#define Q_QMLTYPE_EXPORT Q_DECL_EXPORT
#else
#define Q_QMLTYPE_EXPORT
#endif
Q_QMLTYPE_EXPORT void qml_register_types_org_kde_newstuff_private()
{
}

// No warning - this is just how it is supposed to work
static const QQmlModuleRegistration orgkdenewstuffprivateRegistration("org.kde.newstuff.private", qml_register_types_org_kde_newstuff_private);

