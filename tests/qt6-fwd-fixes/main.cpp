#include <QtCore/qglobal.h>

class myClass
{
  class QStringList; // this is ignored
};

template <typename Key, typename T> class QCache  ;     class SomeClass;
template <typename Key, typename T> class QHash  ;
template <typename Key, typename T> class QMap /*blabla*/ ;
template <typename Key, typename T> class QMultiHash;
template <typename Key, typename T> class QMultiMap;
class QPair;
template <typename T> class QQueue;
template <typename T> class QSet;
template <typename T> class QStack;
template <typename T, qsizetype Prealloc = 256> class QVarLengthArray;
template <typename T> class QList;
template<typename T> class QVector;
class QStringList;
class QByteArrayList;
class QMetaType;
class QVariant;
class QVariantList;
class QVariantMap;
class QVariantHash;
class QVariantPair;
