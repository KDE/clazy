Warn against deprecated API in Qt6

The code is fixed when possible.

QDate::toString(Qt::DateFormat format, QCalendar cal) becomes  QDate::toString(Qt::DateFormat format)

QDateTime(const QDate &) becomes QDate::startOfDay().

Qdir dir; dir = "..." becomes	QDir dir; dir.setPath("...");
Qdir::addResourceSearchPath() becomes QDir::addSearchPath() with prefix.
Only warning are emitted for addResourceSearchPath.

QProcess::start(), execute(), startDetached() becomes QProcess::startCommand(), executeCommand(), startDetachedCommand().

QResource::isCompressed() is replaced with QResource::compressionAlgorithm()

QSignalMapper::mapped() is replaced with QSignalMapper::mappedInt, mappedString, mappedWidget, mappedObject depending on the argument of the functino.

QString::SplitBehavior is replaced with Qt::SplitBehavior.

Qt::MatchRegExp is replaced with Qt::MatchRegularExpression.

QTextStream functions are replaced by the one under the Qt namespace.

QVariant operators '<' '<=' '>' '>=' are replaced with QVariant::compare() function.
QVariant v1; QVariant v2; 'v1 < v2' becomes 'v1.compare(v2) < 0'.


Warning for:
Usage of QMap::insertMulti, uniqueKeys, values, unite, to be replaced manumally with QMultiMap versions.
Usage of QHash::uniqueKeys, to be replaced manumally with QMultiHash versions.
Usage of QLinkedList, to be replaced manually with std::list.
Usage of global qrand() and qsrand(), to be replaces manually using QRandomGenerator.
Usage of QTimeLine::curveShape and setCurveShape, to be replaced manually using QTimeLine::easingCurve and QTimeLine::setEasingCurve.
Usage of QSet and QHash biderectional iterator. Code has to be ported manually using forward iterator.


This fix-it is intended to aid the user porting from Qt5 to Qt6.
Run this check with Qt5. The produced fixed code will compile on Qt6.
