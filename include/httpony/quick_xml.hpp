/**
 * \file
 *
 * \author Mattia Basaglia
 *
 * \copyright Copyright 2016 Mattia Basaglia
 *
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU Affero General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU Affero General Public License for more details.
 *
 *  You should have received a copy of the GNU Affero General Public License
 *  along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef HTTPONY_QUICK_XML_HPP
#define HTTPONY_QUICK_XML_HPP

#include <ostream>
#include <vector>

#include <melanolib/utils/c++-compat.hpp>

namespace httpony {
namespace quick_xml {

class Node
{
public:

    template<class... Args>
    Node(Args&&... args)
    {
        _children.reserve(sizeof...(args));
        append(std::forward<Args>(args)...);
    }

    const std::vector<std::shared_ptr<Node>>& children() const
    {
        return _children;
    }

    virtual void print(std::ostream& out) const = 0;

    virtual bool is_attribute() const
    {
        return false;
    }

protected:
    template<class Head, class... Tail>
        void append(Head&& head, Tail&&... tail)
        {
            _children.push_back(
                std::make_shared<std::remove_cv_t<std::remove_reference_t<Head>>>(
                    std::forward<Head>(head)
            ));
            append(std::forward<Tail>(tail)...);
        }

    template<class ForwardIterator>
        void append_range(ForwardIterator begin, ForwardIterator end)
        {
            for ( ; begin != end; ++ begin )
                append(*begin);
        }

    template<class Sequence>
        void append_range(Sequence sequence)
        {
            _children.reserve(sequence.size());
            append_range(sequence.begin(), sequence.end());
        }

private:
    void append()
    {}

    std::vector<std::shared_ptr<Node>> _children;
};

inline std::ostream& operator<<(std::ostream& stream, const Node& node)
{
    node.print(stream);
    return stream;
}

class BlockElement : public Node
{
public:
    template<class... Args>
        BlockElement(std::string tag_name, Args&&... args)
            : Node(std::forward<Args>(args)...),
              _tag_name(std::move(tag_name))
        {}

    std::string tag_name() const
    {
        return _tag_name;
    }

    void print(std::ostream& out) const override
    {
        out << '<' << tag_name();
        for ( const auto& child : children() )
            if ( child->is_attribute() )
                out << ' ' << *child;
        out << '>';

        for ( const auto& child : children() )
            if ( !child->is_attribute() )
                out << *child;

        out << "</" << tag_name() << '>';
    }

private:
    std::string _tag_name;
};

class Element : public BlockElement
{
public:
    using BlockElement::BlockElement;


    void print(std::ostream& out) const override
    {
        for ( const auto& child : children() )
            if ( !child->is_attribute() )
                return BlockElement::print(out);

        out << '<' << tag_name();
        for ( const auto& child : children() )
            if ( child->is_attribute() )
                out << ' ' << *child;
        out << "/>";
    }
};

class Attribute : public Node
{
public:
    Attribute(std::string name, std::string value)
        : _name(std::move(name)),
          _value(std::move(value))
    {}

    std::string name() const
    {
        return _name;
    }

    std::string value() const
    {
        return _value;
    }

    void print(std::ostream& out) const override
    {
        out << _name << "='" << _value << '\'';
    }

    bool is_attribute() const override
    {
        return true;
    }

private:
    std::string _name;
    std::string _value;
};

class Attributes : public Node
{
public:
    Attributes(const std::initializer_list<Attribute>& attributes)
    {
        append_range(attributes);
    }

    void print(std::ostream& out) const override
    {
        if ( children().empty() )
            return;
        auto iter = children().begin();
        out << **iter;
        for ( ++iter; iter != children().end(); ++iter )
                out << ' ' << **iter;
    }

    bool is_attribute() const override
    {
        return true;
    }
};

class Text : public Node
{
public:
    Text(std::string contents)
        : _contents(std::move(contents))
    {}

    std::string contents() const
    {
        return _contents;
    }

    void print(std::ostream& out) const override
    {
        out << _contents;
    }

private:
    std::string _contents;
};

class XmlDeclaration : public Node
{
public:
    XmlDeclaration(std::string version="1.0", std::string encoding="utf-8")
        : _version(std::move(version)),
          _encoding(std::move(encoding))
    {}

    std::string version() const
    {
        return _version;
    }

    std::string encoding() const
    {
        return _encoding;
    }

    void print(std::ostream& out) const override
    {
        out << "<?xml version='" << _version << '\'';
        if ( !_encoding.empty() )
            out << " encoding='" << _encoding << '\'';
        out << "?>";
    }

private:
    std::string _version;
    std::string _encoding;
};

class DocType : public Node
{
public:
    DocType(std::string string)
        : _string(std::move(string))
    {}

    std::string string() const
    {
        return _string;
    }

    void print(std::ostream& out) const override
    {
        out << "<!DOCTYPE " << _string << ">";
    }

private:
    std::string _string;
};

} // namespace quick_xml
} // namespace httpony
#endif // HTTPONY_QUICK_XML_HPP
