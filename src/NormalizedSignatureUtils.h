/****************************************************************************
**
** Copyright (C) 2015 The Qt Company Ltd.
** Copyright (C) 2015 Olivier Goffart <ogoffart@woboq.com>
** Contact: http://www.qt.io/licensing/
**
** This file contains code copied from Qt 5.6 (https://github.com/qt/qtbase/blob/5.6/src/corelib/kernel/qmetaobject.cpp).
**
** $QT_BEGIN_LICENSE:LGPL21$
** Commercial License Usage
** Licensees holding valid commercial Qt licenses may use this file in
** accordance with the commercial license agreement provided with the
** Software or, alternatively, in accordance with the terms contained in
** a written agreement between you and The Qt Company. For licensing terms
** and conditions see http://www.qt.io/terms-conditions. For further
** information use the contact form at http://www.qt.io/contact-us.
**
** GNU Lesser General Public License Usage
** Alternatively, this file may be used under the terms of the GNU Lesser
** General Public License version 2.1 or version 3 as published by the Free
** Software Foundation and appearing in the file LICENSE.LGPLv21 and
** LICENSE.LGPLv3 included in the packaging of this file. Please review the
** following information to ensure the GNU Lesser General Public License
** requirements will be met: https://www.gnu.org/licenses/lgpl.html and
** http://www.gnu.org/licenses/old-licenses/lgpl-2.1.html.
**
** As a special exception, The Qt Company gives you certain additional
** rights. These rights are described in The Qt Company LGPL Exception
** version 1.1, included in the file LGPL_EXCEPTION.txt in this package.
**
** $QT_END_LICENSE$
**
****************************************************************************/

#ifndef CLAZY_NORMALIZED_SIGNATURE_UTILS_H
#define CLAZY_NORMALIZED_SIGNATURE_UTILS_H

#include <vector>
#include <string>

namespace clazy
{

inline bool is_space(char s)
{
    return (s == ' ' || s == '\t');
}

inline bool is_ident_start(char s)
{
    return ((s >= 'a' && s <= 'z')
            || (s >= 'A' && s <= 'Z')
            || s == '_' || s == '$'
            );
}

inline bool is_ident_char(char s)
{
    return ((s >= 'a' && s <= 'z')
            || (s >= 'A' && s <= 'Z')
            || (s >= '0' && s <= '9')
            || s == '_' || s == '$'
            );
}

static void qRemoveWhitespace(const char *s, char *d)
{
    char last = 0;
    while (*s && is_space(*s))
        s++;
    while (*s) {
        while (*s && !is_space(*s))
            last = *d++ = *s++;
        while (*s && is_space(*s))
            s++;
        if (*s && ((is_ident_char(*s) && is_ident_char(last))
                   || ((*s == ':') && (last == '<')))) {
            last = *d++ = ' ';
        }
    }
    *d = '\0';
}

// This function is shared with moc.cpp. This file should be included where needed.
static std::string normalizeTypeInternal(const char *t, const char *e, bool fixScope = false, bool adjustConst = true)
{
    int len = e - t;
    /*
      Convert 'char const *' into 'const char *'. Start at index 1,
      not 0, because 'const char *' is already OK.
    */
    std::string constbuf;
    for (int i = 1; i < len; i++) {
        if ( t[i] == 'c'
             && strncmp(t + i + 1, "onst", 4) == 0
             && (i + 5 >= len || !is_ident_char(t[i + 5]))
             && !is_ident_char(t[i-1])
             ) {
            constbuf = std::string(t, len);
            if (is_space(t[i-1]))
                constbuf.erase(i-1, 6);
            else
                constbuf.erase(i, 5);
            constbuf = "const " + constbuf;
            t = constbuf.data();
            e = constbuf.data() + constbuf.length();
            break;
        }
        /*
          We mustn't convert 'char * const *' into 'const char **'
          and we must beware of 'Bar<const Bla>'.
        */
        if (t[i] == '&' || t[i] == '*' ||t[i] == '<')
            break;
    }
    if (adjustConst && e > t + 6 && strncmp("const ", t, 6) == 0) {
        if (*(e-1) == '&') { // treat const reference as value
            t += 6;
            --e;
        } else if (is_ident_char(*(e-1)) || *(e-1) == '>') { // treat const value as value
            t += 6;
        }
    }
    std::string result;

#if 1
    // consume initial 'const '
    if (strncmp("const ", t, 6) == 0) {
        t+= 6;
        result += "const ";
    }
#endif

    // some type substitutions for 'unsigned x'
    if (strncmp("unsigned", t, 8) == 0) {
        // make sure "unsigned" is an isolated word before making substitutions
        if (!t[8] || !is_ident_char(t[8])) {
            if (strncmp(" int", t+8, 4) == 0) {
                t += 8+4;
                result += "uint";
            } else if (strncmp(" long", t+8, 5) == 0) {
                if ((strlen(t + 8 + 5) < 4 || strncmp(t + 8 + 5, " int", 4) != 0) // preserve '[unsigned] long int'
                    && (strlen(t + 8 + 5) < 5 || strncmp(t + 8 + 5, " long", 5) != 0) // preserve '[unsigned] long long'
                    ) {
                    t += 8+5;
                    result += "ulong";
                }
            } else if (strncmp(" short", t+8, 6) != 0  // preserve unsigned short
                       && strncmp(" char", t+8, 5) != 0) { // preserve unsigned char
                //  treat rest (unsigned) as uint
                t += 8;
                result += "uint";
            }
        }
    } else {
        // discard 'struct', 'class', and 'enum'; they are optional
        // and we don't want them in the normalized signature
        struct {
            const char *keyword;
            int len;
        } optional[] = {
            { "struct ", 7 },
            { "class ", 6 },
            { "enum ", 5 },
            { 0, 0 }
        };
        int i = 0;
        do {
            if (strncmp(optional[i].keyword, t, optional[i].len) == 0) {
                t += optional[i].len;
                break;
            }
        } while (optional[++i].keyword != 0);
    }

    bool star = false;
    while (t != e) {
        char c = *t++;
        if (fixScope && c == ':' && *t == ':' ) {
            ++t;
            c = *t++;
            int i = result.size() - 1;
            while (i >= 0 && is_ident_char(result.at(i)))
                --i;
            result.resize(i + 1);
        }
        star = star || c == '*';
        result += c;
        if (c == '<') {
            //template recursion
            const char* tt = t;
            int templdepth = 1;
            int scopeDepth = 0;
            while (t != e) {
                c = *t++;
                if (c == '{' || c == '(' || c == '[')
                    ++scopeDepth;
                if (c == '}' || c == ')' || c == ']')
                    --scopeDepth;
                if (scopeDepth == 0) {
                    if (c == '<')
                        ++templdepth;
                    if (c == '>')
                        --templdepth;
                    if (templdepth == 0 || (templdepth == 1 && c == ',')) {
                        result += normalizeTypeInternal(tt, t-1, fixScope, false);
                        result += c;
                        if (templdepth == 0) {
                            if (*t == '>')
                                result += ' '; // avoid >>
                            break;
                        }
                        tt = t;
                    }
                }
            }
        }

        // cv qualifers can appear after the type as well
        if (!is_ident_char(c) && t != e && (e - t >= 5 && strncmp("const", t, 5) == 0)
            && (e - t == 5 || !is_ident_char(t[5]))) {
            t += 5;
            while (t != e && is_space(*t))
                ++t;
            if (adjustConst && t != e && *t == '&') {
                // treat const ref as value
                ++t;
            } else if (adjustConst && !star) {
                // treat const as value
            } else if (!star) {
                // move const to the front (but not if const comes after a *)
                result = "const " + result;
            } else {
                // keep const after a *
                result += "const";
            }
        }
    }

    return result;
}

inline char *qNormalizeType(char *d, int &templdepth, std::string &result)
{
    const char *t = d;
    while (*d && (templdepth
                  || (*d != ',' && *d != ')'))) {
        if (*d == '<')
            ++templdepth;
        if (*d == '>')
            --templdepth;
        ++d;
    }
    // "void" should only be removed if this is part of a signature that has
    // an explicit void argument; e.g., "void foo(void)" --> "void foo()"
    if (strncmp("void)", t, d - t + 1) != 0)
        result += normalizeTypeInternal(t, d);

    return d;
}

inline std::string normalizedType(const char *type)
{
    std::string result;

    if (!type || !*type)
        return result;

    char *stackbuf = new char[strlen(type) + 1];
    qRemoveWhitespace(type, stackbuf);
    int templdepth = 0;
    qNormalizeType(stackbuf, templdepth, result);
    delete []stackbuf;

    return result;
}

inline std::string normalizedSignature(const char *method)
{
    std::string result;
    if (!method || !*method)
        return result;
    int len = int(strlen(method));
    //char *stackbuf = new char[len + 1];
    char *stackbuf = new char[len + 1];
    char *d = stackbuf;
    qRemoveWhitespace(method, d);

    result.reserve(len);

    int argdepth = 0;
    int templdepth = 0;
    while (*d) {
        if (argdepth == 1) {
            d = qNormalizeType(d, templdepth, result);
            if (!*d) //most likely an invalid signature.
                break;
        }
        if (*d == '(')
            ++argdepth;
        if (*d == ')')
            --argdepth;
        result += *d++;
    }

    delete []stackbuf;
    return result;
}

}

#endif
