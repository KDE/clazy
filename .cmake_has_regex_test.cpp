/*
    SPDX-FileCopyrightText: 2015 Sergio Martins <smartins@kde.org>

    SPDX-License-Identifier: LGPL-2.0-or-later
*/

#include <regex>

int main()
{
    std::regex r(R"(\s*(SIGNAL|SLOT)\s*\(\s*(.+)\s*\(.*)");
    return 0;
}
