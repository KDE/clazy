#include <regex>

int main()
{
    std::regex r(R"(\s*(SIGNAL|SLOT)\s*\(\s*(.+)\s*\(.*)");
    return 0;
}
