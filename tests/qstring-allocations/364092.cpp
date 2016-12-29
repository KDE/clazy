#include <QtCore/QString>

inline constexpr QLatin1String operator"" _L1 (const char *str, std::size_t len)
{
     return QLatin1String(str, len);
}

void test364092()
{
    QString s = "F"_L1;
}
