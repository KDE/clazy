#include <QtGui/QColor>

extern void takingColor(QColor);

void test()
{
    QColor c1; // OK
    QColor c2("#001020"); // Warning
    QColor c3(QString("#001020")); // Warning. TODO
    QColor c4(QLatin1String("#001020")); // Warning. TODO
    QColor c5("#023"); // Warning
    QColor c6("#00112233"); // Warning
    QColor c7("#000011112222"); // Warning
    c4.setNamedColor("red"); // OK
    c4.setNamedColor("#001020"); // Warning
    c4.setNamedColor(QLatin1String("#001020")); // Warning
    takingColor("#001122");  // Warning
}
