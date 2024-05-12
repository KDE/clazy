#include <QtCore/QObject>
#include <QtCore/QString>




void testUsedVar(QString bla)
{
    Q_UNUSED(bla) // WARN
    Q_UNUSED(0) // OK, just some other variable
    bla.at(0);
}

void testUnusedVar(QString bla)
{
    Q_UNUSED(bla) // OK, it is really unused
    int i = 1;
}

void testVoidStmt(const QString &bla)
{
  (void) bla; // WARN
  bla.at(0);
}
void testCastStmt(int bla)
{
  auto casted = (float) bla; // OK - diferent from void cast
}
