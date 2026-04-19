#include <QtCore/qobject.h>
namespace TestNsp {
  class FwdClass; // https://invent.kde.org/sdk/clazy/-/issues/35
}
using namespace TestNsp;
class Test: public QObject{
  Q_OBJECT

Q_SIGNALS:
  void emitChangePtr(QList<TestNsp::FwdClass *> list);
  void emitChangeRef(QList<TestNsp::FwdClass &> list);
  void emitChangePtrSpace( QList< TestNsp::FwdClass  * > list);
  void emitChangeRefSpace( QList< TestNsp::FwdClass  & > list);
  void emitChangePtrUnqual(QList<FwdClass *> list);
  void emitChangeRefUnqual(QList<FwdClass &> list);

  void emitChangeConstPtrOk(TestNsp::FwdClass *const list);
  void emitChangeConstPtrOkSpace(TestNsp::FwdClass  *  const  list);
  void emitChangeConstPtrOkNoSpace(TestNsp::FwdClass*const list);
  void emitChangeConstPtrWarn(FwdClass * const list);
  void emitChangeConstPtrWarn2(FwdClass*const list);
};
