#include <QtCore/QString>




void test()
{
    QString s;
    QString s1;
    s = s.arg(1,1); // OK
    s = s.arg(s1); // OK
    s = s.arg(s1,s1); // OK
    s = s.arg(s1,s1,s1); // OK
    s = s.arg(s1,s1,s1,s1); // OK
    s = s.arg(1); // OK
    s = s.arg('1'); // OK
    s = s.arg('1', 10); // OK
    int i;
    s = s.arg(s1, i); // Warning
    s = s.arg(s1, i, 3); // Warning
    s = s.arg(1, 1, 10); // OK
    int m_labelFieldWidth, latitude;
    s = s.arg(1, m_labelFieldWidth); // OK
    s = s.arg(1, latitude); // Warning
    QString("%1").arg(s, -38); // OK
    QString s2, s3, s4, s5;
    s5 = QString("%1 %2 %3 %4").arg(s).arg(s1).arg(s3, s4); // Warning
    QString().arg(s1, s2, s3, s4, s5).arg(s1, s2, s3, s4, s5); // OK
    QString().arg(s1, s2, s3, s4, s5).arg(s1, s2, s3, s4); // Warning
}
