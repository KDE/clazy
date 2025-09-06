#include <QtCore/QList>
#include <QtCore/QString>

const int myArr[] = {1,2,3}; // OK
const QString strArr[] = { // Warn, once
  QString("testme"),
  QString("testme2")
};
static QList<QString> s_globalStuff{ // Warn, once
  QString("Hello"),
  QString("fellow"),
  QString("KDE"),
  QString("dev")
};
