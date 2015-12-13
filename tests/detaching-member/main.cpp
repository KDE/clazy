#include <QtCore/QList>
#include <QtCore/QString>












struct A
{
    A()
    {
        m_list.begin();  // Warning
        m_list.cbegin(); // OK
    }

    void f()
    {
        QList<int> list;
        list.begin(); // OK
        list[0]; // OK

        m_list.begin();  // Warning
        m_list.cbegin(); // OK
        m_constList.begin();  // OK
    }

    void constF() const
    {
        m_list.begin(); // OK
        m_string.data(); // OK
        m_string[0]; // OK
        m_list[0]; // OK
        m_mutableString.begin(); // Warning
        m_mutableString[0]; // Warning
    }

    void testAssignment()
    {
        m_string[0] = QChar('a'); // OK
        m_mutableString[0] = QChar('a'); // OK
        m_list[0]++;    // OK
        m_list[0] += 2; // OK
        m_list2[0] = m_list[0]; // Warning
    }


    QList<int> m_list;
    QList<int> m_list2;
    QString m_string;
    mutable QString m_mutableString;
    const QList<int> m_constList;
};
