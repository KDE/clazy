#ifndef LEV_DISTANCE_H
#define LEV_DISTANCE_H

/**
 * Levenshtein Distance
 *
 * Code snippet taken from https://en.wikibooks.org/wiki/Algorithm_Implementation/Strings/Levenshtein_distance
 *
 * Licensed under CC BY-SA 3.0 (https://creativecommons.org/licenses/by-sa/3.0/)
 *
 * UNLESS OTHERWISE MUTUALLY AGREED TO BY THE PARTIES IN WRITING, LICENSOR OFFERS THE WORK AS-IS
 * AND MAKES NO REPRESENTATIONS OR WARRANTIES OF ANY KIND CONCERNING THE WORK, EXPRESS, IMPLIED,
 * STATUTORY OR OTHERWISE, INCLUDING, WITHOUT LIMITATION, WARRANTIES OF TITLE, MERCHANTIBILITY,
 * FITNESS FOR A PARTICULAR PURPOSE, NONINFRINGEMENT, OR THE ABSENCE OF LATENT OR OTHER DEFECTS
 * ACCURACY, OR THE PRESENCE OF ABSENCE OF ERRORS, WHETHER OR NOT DISCOVERABLE. SOME JURISDICTIONS
 * DO NOT ALLOW THE EXCLUSION OF IMPLIED WARRANTIES, SO SUCH EXCLUSION MAY NOT APPLY TO YOU.
 * EXCEPT TO THE EXTENT REQUIRED BY APPLICABLE LAW, IN NO EVENT WILL LICENSOR BE LIABLE TO YOU ON
 * ANY LEGAL THEORY FOR ANY SPECIAL, INCIDENTAL, CONSEQUENTIAL, PUNITIVE OR EXEMPLARY DAMAGES ARISING
 * OUT OF THIS LICENSE OR THE USE OF THE WORK, EVEN IF LICENSOR HAS BEEN ADVISED OF THE POSSIBILITY
 * OF SUCH DAMAGES.
 **/

#include <string>
#include <numeric>
#include <algorithm>

int levenshtein_distance(const std::string &s1, const std::string &s2)
{
    // To change the type this function manipulates and returns, change
    // the return type and the types of the two variables below.
    int s1len = s1.size();
    int s2len = s2.size();

    auto column_start = (decltype(s1len)) 1;

    auto column = new decltype(s1len)[s1len + 1];
    std::iota(column + column_start - 1, column + s1len + 1, column_start - 1);

    for (auto x = column_start; x <= s2len; x++) {
        column[0] = x;
        auto last_diagonal = x - column_start;
        for (auto y = column_start; y <= s1len; y++) {
            auto old_diagonal = column[y];
            auto possibilities = {
                column[y] + 1,
                column[y - 1] + 1,
                last_diagonal + (s1[y - 1] == s2[x - 1] ? 0 : 1)
            };
            column[y] = std::min(possibilities);
            last_diagonal = old_diagonal;
        }
    }
    auto result = column[s1len];
    delete[] column;
    return result;
}

#endif
