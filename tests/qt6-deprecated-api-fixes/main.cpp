#include <QtCore/QTextStream>
#include <QtCore/QCalendar>
#include <QtCore/QDateTime>
#include <QtCore/QDir>
#include <QtCore/QHash>
#include <QtCore/QLinkedList>
#include <QtCore/QMap>
#include <QtCore/QProcess>
#include <QtCore/QResource>
#include <QtCore/QSet>
#include <QtCore/QSignalMapper>
#include <QtCore/QTimeLine>
#include <QtWidgets/QButtonGroup>
#include <QtWidgets/QComboBox>
#include <QtWidgets/QDirModel>
#include <QtWidgets/QDockWidget>
#include <QtWidgets/QGraphicsView>
#include <QtWidgets/QMacCocoaViewContainer>
#include <QtWidgets/QMacNativeWidget>
#include <QtWidgets/QSplashScreen>
#include <QtWidgets/QStyle>
#include <QtWidgets/QTextBrowser>
#include <QtWidgets/QWizard>

class my_Class
{
public:
    QDir m_dir;
    QDir *m_dir_bis;
    QVariant m_variant;
};
#define MYSTRING "myDirPath"

void test()
{
    QDir dir;
    dir = "myStuff";

    QDir d;
    QFileInfo fi;
    d = fi.absolutePath();

    my_Class test_class;
    test_class.m_dir = "name";

    my_Class* test_class_bis = new my_Class;
    test_class_bis->m_dir = ("name");
    *test_class.m_dir_bis = "name";

    QDir dir2;
    dir2 = MYSTRING;
    dir2 = dir;

    QDir dir3;
    dir3= "my" "Stuff";

    QDir dir4;
    char *pathName = "myStuff";
    dir4 = pathName;

    QDir dir5;
    bool cond = true;
    dir5 = cond ? "mystuff" : "yourStuff";

    QDir dir6;
    dir6 = true ? (cond ? "path1" : "path2") : (cond ? "path3" : "path4");

    QDir *dir7 = new QDir("apath");
    *dir7 = "adir";
    ((*dir7)) = "adir";

    QDir::addResourceSearchPath("somePath1");
    dir6.addResourceSearchPath("somePath2");

    QMap<QString, QString> m;
    m.insertMulti("foo", "bar");
    QList<QString> m_keys= m.uniqueKeys();
    QList<QString> m_list= m.values();
    QMap<QString, QString> mm;
    m.unite(mm);

    QProcess pp;
    pp.start("stringContainingACommandWithArguments");
    pp.execute("stringContainingACommandWithArguments");
    pp.startDetached("stringContainingACommandWithArguments");

    QResource rr;
    bool a_bool = rr.isCompressed();

    uint matchtype = 4;
    if (matchtype ==  Qt::MatchRegExp)
        matchtype = 0;

    QTextStream out;
    out << "blabla" << endl;
    out << hex << endl;

    QString a_string = "eeoaaoii";
    QString sep = "o";
    QStringList my_list =  a_string.split(sep, QString::KeepEmptyParts);
    QStringList my_list2 =  a_string.split(sep, QString::SplitBehavior::KeepEmptyParts);
    QString::SplitBehavior behavior = QString::KeepEmptyParts;

    QSet<QString> my_set;
    QSet<QString>::iterator it_set = my_set.begin();
    QSet<QString>::const_iterator cit_set = my_set.cbegin();
    --it_set;
    it_set + 1;
    it_set - 1;
    it_set += 1;
    it_set -= 1;
    ++it_set; //ok
    cit_set -= 2;
    cit_set += 1;
    cit_set + 1;
    cit_set - 1;

    QSetIterator<QString> i(my_set);
    i.hasPrevious();
    i.previous();
    i.peekPrevious();
    i.findPrevious(a_string);

    QSet<int> s;
    s << 1 << 17 << 61 << 127 << 911;
    s.rbegin();
    s.rend();
    s.crbegin();
    s.crend();

    int my_int = 2;
    QWidget* my_qwidget;
    QObject* my_qobject;
    QSignalMapper sigMap;
    sigMap.mapped(1);
    sigMap.mapped(my_int);
    sigMap.mapped("astring");
    sigMap.mapped(a_string);
    sigMap.mapped(my_qwidget);
    sigMap.mapped(my_qobject);

    QHash<QString, QString> my_hash;
    QHash<QString, QString>::iterator it_hash = my_hash.begin();
    QHash<QString, QString>::const_iterator cit_hash = my_hash.cbegin();
    --it_hash;
    it_hash + 1;
    it_hash - 1;
    it_hash += 1;
    it_hash -= 1;
    ++it_hash; //ok
    cit_hash -= 2;
    cit_hash += 1;
    cit_hash + 1;
    cit_hash - 1;

    QHashIterator<QString, QString> ih(my_hash);
    ih.hasPrevious();
    ih.previous();
    ih.peekPrevious();
    ih.findPrevious(a_string);

    QLinkedList<QString> linkList;

    qrand();
    qsrand(1);

    QTimeLine timeline;
    timeline.setCurveShape(QTimeLine::CurveShape::EaseInCurve);
    timeline.curveShape();

    QDate myDate;
    QDate *myOtherDate = new QDate();
    QCalendar myCalendar;
    myDate.toString(Qt::DateFormat::TextDate, myCalendar);
    myDate.toString("format", myCalendar);
    QDateTime myDateTime = QDateTime(myDate);
    myDateTime = QDateTime(*myOtherDate);

    QVariant var1;
    QVariant *var3;
    QVariant var2;
    bool bool1 = var1 > var2;
    bool bool2 = (var1 >= (var2));
    bool bool3 = ((*var3) < var2);
    bool bool4 = (*var3 <= var2);
    bool bool5 = (*var3 <= test_class.m_variant);
    bool bool6 = (test_class_bis->m_variant <= test_class.m_variant);


}

void function1(QLinkedList<QString> arg) {};
QLinkedList<QString> function2() { return {}; };

class aclass
{
public:
     QLinkedList<QString> m_linkList;
     void m_function1(QLinkedList<QString> arg) {};
};

namespace Qt {
    void test_1() {
        uint matchtype = 4;
        if (matchtype ==  MatchRegExp)
            matchtype = 0;
        QTextStream out;
        out << "blabla" << QTextStreamFunctions::endl;
        out << QTextStreamFunctions::hex << QTextStreamFunctions::endl;

        QString a_string = "eeoaaoii";
        QString sep = "o";
        QStringList my_list =  a_string.split(sep, QString::KeepEmptyParts);
        QString::SplitBehavior behavior = QString::KeepEmptyParts;

    }
}

void test_widgets()
{
    QWizard wizard;
    wizard.visitedPages();

    QButtonGroup buttonGroup;
    buttonGroup.buttonClicked(1);
    buttonGroup.buttonPressed(1);
    buttonGroup.buttonReleased(1);
    buttonGroup.buttonToggled(1, true);

    QComboBox combobox;
    combobox.setSizeAdjustPolicy(QComboBox::SizeAdjustPolicy::AdjustToMinimumContentsLength);
    QString astring = "asdf";
    combobox.currentIndexChanged(astring);
    combobox.activated(astring);
    combobox.highlighted(astring);

    QMacCocoaViewContainer *cocoa = NULL;
    QMacNativeWidget *native = NULL;

    QWidget *a_widget = NULL;
    QSplashScreen *splash1 = new QSplashScreen(a_widget);
    QScreen *a_screen = NULL;
    QSplashScreen *splash2 = new QSplashScreen(a_screen);

    QTextBrowser browser;
    QString a_string = "aaa";
    browser.highlighted(a_string);
    QUrl a_url;
    browser.highlighted(a_url);

    QDockWidget dock;
    dock.setFeatures(QDockWidget::AllDockWidgetFeatures);

    QDirModel dirModel;

    QGraphicsView graphview;
    QMatrix matrix = graphview.matrix();
    graphview.setMatrix(matrix);
    graphview.resetMatrix();

    QStyle *style = NULL;
    style->pixelMetric(QStyle::PixelMetric::PM_DefaultTopLevelMargin);
    style->pixelMetric(QStyle::PixelMetric::PM_DefaultChildMargin);
    style->pixelMetric(QStyle::PixelMetric::PM_DefaultLayoutSpacing);
    style->subElementRect(QStyle::SubElement::SE_DialogButtonBoxLayoutItem, NULL);

}

void func_coco (QMacCocoaViewContainer *cocoa) {}
void func_native (QMacNativeWidget *native){}
