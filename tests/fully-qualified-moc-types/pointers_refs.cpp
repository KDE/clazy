#include <QtCore/qobject.h>
namespace EnumNs {
  class FwdClass; // https://invent.kde.org/sdk/clazy/-/issues/35
}
using namespace EnumNs;
class Test: public QObject{
  Q_OBJECT

Q_SIGNALS:
  void emitChangePtr(QList<EnumNs::FwdClass *> list);
  void emitChangeRef(QList<EnumNs::FwdClass &> list);
  void emitChangePtrSpace( QList< EnumNs::FwdClass  * > list);
  void emitChangeRefSpace( QList< EnumNs::FwdClass  & > list);
  void emitChangePtrUnqual(QList<FwdClass *> list);
  void emitChangeRefUnqual(QList<FwdClass &> list);

};
