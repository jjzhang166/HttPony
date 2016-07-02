/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright (C) 2016 Mattia Basaglia
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

#include "uri.hpp"

#include <cctype>
#include <regex>
#include <melanolib/string/stringutils.hpp>
#include <melanolib/string/quickstream.hpp>

namespace muhttpd {


Uri::Uri(const std::string& uri)
{
    static const std::regex uri_regex(
        R"(^(?:([a-zA-Z][-a-zA-Z.+]*):)?(?://([^/]*))?(/?[^?]*)(?:\?([^#]*))(?:#(.*))?$)",
        std::regex::ECMAScript|std::regex::optimize
    );
    std::smatch match;
    if ( std::regex_match(uri, match, uri_regex) )
    {
        scheme = urldecode(match[1]);
        authority = match[2]; /// \todo could parse user:password@host:port

        auto segments = melanolib::string::char_split(match[3], '/');
        for ( auto& segment : segments )
        {
            if ( segment == ".." )
                path.pop_back();
            else if ( segment != "." )
                path.push_back(urldecode(segment));
        }

        query = parse_query_string(match[4]);

        fragment = urldecode(match[5]);
    }
}

std::string Uri::full() const
{
    std::string result;

    if ( !scheme.empty() )
        result += urlencode(scheme) + ':';

    if ( !authority.empty() )
        result += "//" + authority;

    result += path_string();

    result += query_string(true);

    if ( !fragment.empty() )
        result += '#' + urlencode(fragment);

    return result;
}

bool Uri::operator==(const Uri& oth) const
{
    return scheme == oth.scheme &&
           authority == oth.authority &&
           std::equal(path.begin(), path.end(), oth.path.begin(), oth.path.end()) &&
           std::equal(query.begin(), query.end(), oth.query.begin(), oth.query.end()) &&
           fragment == oth.fragment
    ;
}

std::string Uri::path_string() const
{
    std::string result;
    for ( const auto& segment : path )
        result += '/' + urlencode(segment);
    return result;
}

std::string Uri::query_string(bool question_mark) const
{
    return build_query_string(query);
}


static char hex_digit(uint8_t digit)
{
    return digit > 9 ? digit - 10 + 'A' : digit + '0';
}

static int get_hex(char ch)
{
    return ch > '9' ?
        ( ch >= 'a' ? ch - 'a' + 10 : ch - 'A' + 10 )
        : ch - '0'
    ;
}

std::string urlencode(const std::string& input, bool plus_spaces)
{
    std::string output;
    output.reserve(input.size());

    for ( uint8_t c : input )
    {
        if ( std::isalnum(c) || c == '-' || c == '_' || c == '.' || c == '~' )
        {
            output.push_back(c);
        }
        else if ( plus_spaces && c == ' ' )
        {
            output.push_back('+');
        }
        else
        {
            output.push_back('%');
            output.push_back(hex_digit((c & 0xF0) >> 4));
            output.push_back(hex_digit(c & 0xF));
        }
    }
    return output;
}

std::string urldecode(const std::string& input, bool plus_spaces)
{
    std::string output;
    output.reserve(input.size());

    for ( auto it = input.begin(); it != input.end(); ++it )
    {
        if ( *it == '%' )
        {
            if ( it + 2 >= input.end() )
                break;

            if ( !std::isxdigit(*(it + 1)) || !std::isxdigit(*(it + 2)) )
            {
                output.push_back(*it);
                continue;
            }
            output.push_back((get_hex(*(it + 1)) << 4) | get_hex(*(it + 2)));
            it += 2;
        }
        else if ( plus_spaces && *it == '+' )
        {
            output.push_back(' ');
        }
        else
        {
            output.push_back(*it);
        }
    }
    return output;
}

DataMap parse_query_string(const std::string& str)
{
    DataMap result;

    melanolib::string::QuickStream input(str);
    if ( input.peek() == '?' )
        input.ignore();


    while ( !input.eof() )
    {
        std::string name;
        std::string value;
        name = urldecode(
            input.get_until([](char c){ return c == '&' || c == '='; })
        );
        if ( input.peek_back() == '=' )
            value = urldecode(input.get_line('&'));
        result.append(name, value);
    }

    return result;
}

std::string build_query_string(const DataMap& headers, bool question_mark)
{
    std::string result;

    for ( const auto& header : headers )
    {
        if ( result.empty() )
        {
            if ( question_mark )
                result += '?';
        }
        else
        {
            result += '&';
        }

        result += urlencode(header.name);
        if ( !header.value.empty() )
            result += '=' + urlencode(header.value);
    }

    return result;
}

} // namespace muhttpd