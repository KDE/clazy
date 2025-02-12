Warn against deprecated API in Qt6

The code is fixed when possible.

* `QDate::toString(Qt::DateFormat format, QCalendar cal)` becomes  `QDate::toString(Qt::DateFormat format)`.

* `QDateTime(const QDate &)` becomes `QDate::startOfDay()`.

* `QDir dir; dir = "..."` becomes	`QDir dir; dir.setPath("...")`.

* `QDir::addResourceSearchPath()` becomes `QDir::addSearchPath()` with prefix.

* Only warning are emitted for `addResourceSearchPath`.

* `QResource::isCompressed()` is replaced with `QResource::compressionAlgorithm()`.

* `QSignalMapper::mapped()` is replaced with `QSignalMapper::mappedInt`, `QSignalMapper::mappedString`, `QSignalMapper::mappedObject` depending on the argument of the function.

* `QString::SplitBehavior` is replaced with `Qt::SplitBehavior`.

* `Qt::MatchRegExp` is replaced with `Qt::MatchRegularExpression`.

* `QTextStream` functions are replaced by the one under the Qt namespace.

* `QVariant` operators '`<`' '`<=`' '`>`' '`>=`' are replaced with `QVariant::compare()` function.

* `QVariant v1; QVariant v2; 'v1 < v2'` becomes '`QVariant::compare(v1, v2) < 0`'.

* `QWizard::visitedPages()` is replaced with `QWizard::visitedIds()`.

* `QButtonGroup buttonClicked/Pressed/Released/Toggled(int)` is replaced with `QButtonGroup::idClicked/Pressed/Released/Toggled(int)`.

* `QCombobox::activated(const QString &)` and `highlighted(const QString &)` are replaced with `QComboBox::textActivated` or `textHighlighted` respectively.

Warning for:  

* Usage of `QMap::insertMulti`, `uniqueKeys`, `values`, `unite`, to be replaced with QMultiMap versions.  

* Usage of `QHash::uniqueKeys`, to be replaced with `QMultiHash` versions.  

* Usage of `LinkedList`, to be replaced with `std::list`.  

* Usage of global `qrand()` and `qsrand()`, to be replaced using `QRandomGenerator`.  

* Usage of `QTimeLine::curveShape` and `QTimeLine::setCurveShape`, to be replaced using `QTimeLine::easingCurve` and `QTimeLine::setEasingCurve`.  

* Usage of `QSet` and `QHash` biderectional iterator. Code has to be ported using forward iterator.  

* Usage of `QMacCocoaViewContainer`, to be replaced using `QWindow::fromWinId` and `QWidget::createWindowContainer` instead.  

* Usage of `QMacNativeWidget`, to be replaced using `QWidget::winId` instead.  

* Usage of `QComboBox::SizeAdjustPolicy::AdjustToMinimumContentsLength`, to be replaced using `AdjustToContents` or `AdjustToContentsOnFirstShow`.  

* Usage of `QComboBox::currentIndexChanged(const QString &)`, to be replaced using `currentIndexChanged(int)` instead, and getting the text using `itemText(index)`.  

* Usage of `QSplashScreen(QWidget *parent,...)`, to be replaced with the constructor taking a `QScreen *`.  

* Usage of `QTextBrowser::highlighted(const QString &)`, to be replaced with `highlighted(const QUrl &)`.  

* Usage of `QDockWidget::AllDockWidgetFeatures`, to  be replaced with  `DockWidgetClosable|DockWidgetMovable|DockWidgetFloatable`.  

* Usage of `QDirModel`, to be replaced with `QFileSystemModel`.  

* Usage of `QGraphicsView::matrix`, `setMatrix(const QMatrix &)` and `resetMatrix`, to be replaced with `QGraphicsView::transform`, `setTransform(const QTransform &)` and `resetTransfrom`.  

* Usage of the following QStyle enum: `QStyle::PixelMetrix::PM_DefaultTopLevelMargin`, `PM_DefaultChildMargin`, `PM_DefaultLayoutSpacing` and `QStyle::SubElement::SE_DialogButtonBoxLayoutItem`.  

This fix-it is intended to aid the user porting from Qt5 to Qt6.  
Run this check with Qt5. The produced fixed code will compile on Qt6.
