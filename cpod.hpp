//
// MIT License
//
// Copyright (c) 2025 Henry Du
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in all
// copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.
//
#pragma once

// Core headers
#include <string>
#include <string_view>
#include <ranges>
#include <utility>
#include <algorithm>
#include <stdexcept>
#include <utility>
#include <tuple>
#include <type_traits>
#include <charconv>  // from_chars and to_chars
#include <format>    // for format api.

// Container support headers.
#include <array>
#include <vector>
#include <span>
#include <deque>
#include <list>
#include <forward_list>
#include <set>
#include <unordered_set>
#include <map>
#include <unordered_map>

namespace cpod {

    using  flag_t = std::uint32_t;
    class  archive;

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///                                   Variable view implementation
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    template <class Ty>
    struct variable_view {
        std::string_view             name;
        Ty*                          value;
        flag_t                       flag; // For get this is useless.

        variable_view(std::string_view n, const Ty& v, flag_t f = {}) :
        name(n), value(const_cast<Ty*>(&v)), flag(f) {}

        variable_view(std::string_view n, Ty& v, flag_t f = {}) :
        name(n), value(&v), flag(f) {}
    };

    struct output_format_view {
        std::string content;
        constexpr explicit output_format_view(const std::string& str)
        : content(str) {}
    };
    
    struct comment_view : output_format_view {
        constexpr explicit comment_view(std::string_view c)
        : output_format_view(std::format("//{:s}\n", c)) {}
    };

    struct macro_define_view : output_format_view {
        constexpr explicit macro_define_view(std::string_view k, std::string_view v)
        : output_format_view(std::format("#define {:s} {:s}\n", k, v)) {}
    };

    struct auto_indent_text_view : output_format_view {
        template <typename ... Args>
        constexpr explicit auto_indent_text_view(std::string_view t, const Args& ... args)
        : output_format_view(std::vformat(t, std::make_format_args(args...))) {}
    };

    // For convenient construction.
    template <class Ty>
    using var = variable_view<Ty>;

    // Format views.
    using com = comment_view;
    using def = macro_define_view;
    using txt = auto_indent_text_view;

    // This demonstrates what a basic serializer should contain.
    template <class Ty>
    struct serializer {};

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///                                    Archive declaration
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    class archive {
        std::string   content_;
        std::size_t   base_indent_count_;
    public:
        
        // Writer mode
        archive(std::size_t bic = 0, char bi = ' ') : content_(), base_indent_count_(bic) {}
        // Reader mode
        archive(std::string_view c) : content_(c), base_indent_count_(0) {}
        
        constexpr std::string&        content()       { return content_; }
        constexpr std::string_view    content() const { return content_; }

        // Compile writes compiled code stream to content_.
        inline    std::string         compile_content_default(std::initializer_list<std::pair<std::string_view, std::string>> init_macro_map = {}) noexcept;

        template <class Ty>
        constexpr std::string::const_iterator find_variable_begin(std::string_view var_name);

        constexpr std::size_t&        indent()        { return base_indent_count_; }
        constexpr std::size_t         indent()  const { return base_indent_count_; }

        constexpr void append_indent() {
            std::string buf(base_indent_count_, ' ');
            content_.append(buf);
        }

        template <class Ty>
        constexpr archive& operator<<(variable_view<Ty> v) {
            append_indent();
            serializer<Ty>{}(*this, v.name, *v.value, v.flag);
            return *this;
        }

        constexpr archive& operator<<(const output_format_view& v) {
            append_indent();
            content_.append(v.content);
            return *this;
        }

        constexpr archive& operator<<(std::string_view str) {
            content_.append(str);
            return *this;
        }

        constexpr archive& operator<<(char c) {
            content_.push_back(c);
            return *this;
        }

        template <class Ty>
        constexpr archive& operator>>(variable_view<Ty> v) {
            if (auto it = find_variable_begin<Ty>(v.name); it != content_.cend()) {
                serializer<Ty>{}(it, *v.value, v.flag);
                return *this;
            }
            throw std::invalid_argument("Can't find variable name!");
        }
    };
    
    namespace details {

        //////////////////////////////////////////////////////////////////////////////////////////////////////////
        ///                                 Detailed traits and constraints
        //////////////////////////////////////////////////////////////////////////////////////////////////////////

        template <class Ty>
        struct std_basic_type_traits : std::false_type {};

#define DEFINE_STD_BASIC_TYPE_STRING(type, id)                                 \
template <>                                                                    \
struct std_basic_type_traits<std::remove_cvref_t<##type##>> : std::true_type { \
    static constexpr std::string_view name = #type ;                           \
    static constexpr std::uint8_t     identifier = id;                         \
}
    
        DEFINE_STD_BASIC_TYPE_STRING(int8_t,        1);
        DEFINE_STD_BASIC_TYPE_STRING(uint8_t,       2);
        DEFINE_STD_BASIC_TYPE_STRING(int16_t,       3);
        DEFINE_STD_BASIC_TYPE_STRING(uint16_t,      4);
        DEFINE_STD_BASIC_TYPE_STRING(int,           5);
        DEFINE_STD_BASIC_TYPE_STRING(uint32_t,      6);
        DEFINE_STD_BASIC_TYPE_STRING(int64_t,       7);
        DEFINE_STD_BASIC_TYPE_STRING(uint64_t,      8);
        DEFINE_STD_BASIC_TYPE_STRING(float,         9);
        DEFINE_STD_BASIC_TYPE_STRING(double,        10);
        DEFINE_STD_BASIC_TYPE_STRING(bool,          11);

        // To support custom compiler string.
        template <class Allocator>
        struct std_basic_type_traits<std::basic_string<char, std::char_traits<char>, Allocator>> : std::true_type {
            static constexpr std::string_view name = "std::string" ;
            static constexpr std::uint8_t identifier = 12;
        };

        // String and string_view uses same type name.
        template <>
        struct std_basic_type_traits<std::basic_string_view<char>> : std::true_type {
            static constexpr std::string_view name        = "std::string";
            static constexpr std::uint8_t     identifier  = 12;
        };
#undef  DEFINE_STD_BASIC_TYPE_STRING

        // Concepts and constraints.
        template <class Ty>
        struct std_string_type_traits : std::false_type {};

        template <typename CharT, template <class> class Allocator>
        struct std_string_type_traits<std::basic_string<CharT, std::char_traits<CharT>, Allocator<CharT>>> : std::true_type {
            static constexpr bool is_view = false;
        };

        template <typename CharT>
        struct std_string_type_traits<std::basic_string_view<CharT, std::char_traits<CharT>>> : std::true_type {
            static constexpr bool is_view = true;
        };

        // Container type traits.
        template <class Ty>
        struct std_template_library_type_traits : std::false_type {};
        
#define DEFINE_MONO_STL_TRAITS(type, id, resizable) \
    template <typename Ty, typename ... OtherStuff> \
    struct std_template_library_type_traits<type##<Ty, OtherStuff...>> : std::true_type { \
        static constexpr bool             is_resizeable      = resizable;\
        static constexpr bool             is_mono            = true;     \
        static constexpr bool             is_double          = false;    \
        static constexpr std::string_view name               = #type;    \
        static constexpr std::uint8_t     identifier         = id;       \
    }
#define DEFINE_DOUBLE_STL_TRAITS(type, id) \
template <typename K, typename V, typename ... OtherStuff> \
    struct std_template_library_type_traits<type##<K, V, OtherStuff...>> : std::true_type { \
        static constexpr bool             is_resizeable      = false;    \
        static constexpr bool             is_mono            = false;    \
        static constexpr bool             is_double          = true;     \
        static constexpr std::string_view name               = #type;    \
        static constexpr std::uint8_t     identifier         = id;       \
    }
        
        DEFINE_MONO_STL_TRAITS(std::vector,               13, true);
        DEFINE_MONO_STL_TRAITS(std::deque,                14, true);
        DEFINE_MONO_STL_TRAITS(std::list,                 15, true);
        DEFINE_MONO_STL_TRAITS(std::forward_list,         16, true);
        
//        std::hive as a new basic std container will join the family in C++26
//        So we leave this position for it in case future code won't compatible with old code.
//        DEFINE_MONO_STL_TRAITS(std::hive,                 17, false);
        
        DEFINE_MONO_STL_TRAITS(std::set,                  18, false);
        DEFINE_MONO_STL_TRAITS(std::multiset,             19, false);
        DEFINE_MONO_STL_TRAITS(std::unordered_set,        20, false);
        DEFINE_MONO_STL_TRAITS(std::unordered_multiset,   21, false);

        DEFINE_DOUBLE_STL_TRAITS(std::map,                22);
        DEFINE_DOUBLE_STL_TRAITS(std::multimap,           23);
        DEFINE_DOUBLE_STL_TRAITS(std::unordered_map,      24);
        DEFINE_DOUBLE_STL_TRAITS(std::unordered_multimap, 25);

        template <typename K, typename V>
        struct std_template_library_type_traits<std::pair<K, V>> : std::true_type {
            static constexpr bool             is_resizeable      = false;
            static constexpr bool             is_mono            = false;
            static constexpr bool             is_double          = false; 
            static constexpr std::string_view name               = "std::pair";
            static constexpr std::uint8_t     identifier         = 26;
        };

        template <typename Ty, std::size_t N>
        struct std_template_library_type_traits<std::array<Ty, N>> : std::true_type {
            static constexpr bool             is_resizeable      = false;
            static constexpr bool             is_mono            = false;
            static constexpr bool             is_double          = false; 
            static constexpr std::string_view name               = "std::array";
            static constexpr std::uint8_t     identifier         = 27;
            
        };

        template <typename ... Args>
        struct std_template_library_type_traits<std::tuple<Args...>> : std::true_type {
            static constexpr bool             is_resizeable      = false;
            static constexpr bool             is_mono            = false;
            static constexpr bool             is_double          = false; 
            static constexpr std::string_view name               = "std::tuple";
            static constexpr std::uint8_t     identifier         = 28;
        };
#undef DEFINE_DOUBLE_STL_TRAITS
#undef DEFINE_MONO_STL_TRAITS

        template <class Ty>
        concept std_basic_type             = std_basic_type_traits<Ty>::value;

        template <class Ty>
        concept std_template_library_range = std_template_library_type_traits<Ty>::is_mono || std_template_library_type_traits<Ty>::is_double;

        template <class Ty>
        struct iterate_std_template_stuff_impl {};

        template <class ... Args>
        struct iterate_std_template_recursive_helper {};

        template <class Last>
        struct iterate_std_template_recursive_helper<Last> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                iterate_std_template_stuff_impl<Last>{}(buf, bin);
            }
        };
        
        template <class First, typename ... Rest>
        struct iterate_std_template_recursive_helper<First, Rest...> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                iterate_std_template_stuff_impl<First>{}(buf, bin);
                iterate_std_template_recursive_helper<Rest...>{}(buf, bin);
            }
        };
        
        template <std_basic_type Value>
        struct iterate_std_template_stuff_impl<Value> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append(std_basic_type_traits<std::remove_const_t<Value>>::name);
                } else {
                    buf.push_back(std_basic_type_traits<std::remove_const_t<Value>>::identifier);
                }
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const Value& value) {
                std::invoke(formatter, buf, value);
                buf.push_back(',');
            }
            template <class Reader> // Depart only separate reader from writer so always set this to any random integer, method won't take over this.
            constexpr auto operator()(std::string::const_iterator& iter, Reader reader, Value& value, int department) {
                std::invoke(reader, iter, value);
            }
        };

        template <std_template_library_range STL>
        struct iterate_std_template_stuff_impl<STL> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append(std_template_library_type_traits<STL>::name).push_back('<');   
                } else {
                    buf.push_back(std_template_library_type_traits<STL>::identifier);
                    buf.push_back('<');
                }
                if constexpr (std_template_library_type_traits<STL>::is_mono) {
                    iterate_std_template_recursive_helper<typename STL::value_type>{}(buf, bin);
                }
                else if constexpr (std_template_library_type_traits<STL>::is_double) {
                    iterate_std_template_recursive_helper<typename STL::key_type, typename STL::value_type::second_type>{}(buf, bin);
                }
                buf.back() = '>';
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const STL& value) {
                buf.push_back('{');
                for (auto i = value.cbegin(); i != value.cend(); ++i) {
                    iterate_std_template_stuff_impl<typename STL::value_type>{}(buf, formatter, *i);
                }
                buf.back() = '}';
                buf.push_back(',');
            }
            template <class Reader>
            constexpr auto operator()(std::string::const_iterator& iter, Reader reader, STL& value, int department) {
                const std::size_t n = *reinterpret_cast<const std::size_t*>(&*iter);
                iter += sizeof(std::size_t);
                auto inserter = std::inserter(value, value.end());
                for (std::size_t i = 0; i != n; ++i) {
                    if constexpr (std_template_library_type_traits<STL>::is_mono) {
                        typename STL::value_type cache;
                        iterate_std_template_stuff_impl<typename STL::value_type>{}(iter, reader, cache, department);
                        *inserter++ = std::move(cache);
                    }
                    else if constexpr (std_template_library_type_traits<STL>::is_double) {
                        typename STL::key_type                key;
                        typename STL::value_type::second_type val;
                        iterate_std_template_stuff_impl<typename STL::key_type>{}               (iter, reader, key, department);
                        iterate_std_template_stuff_impl<typename STL::value_type::second_type>{}(iter, reader, val, department);
                        *inserter++ = std::make_pair(key, val);
                    }
                }
            }
        };

        template <typename F, typename S>
        struct iterate_std_template_stuff_impl<std::pair<F, S>> {
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append("std::pair<");
                } else {
                    buf.push_back('\x1a');
                    buf.push_back('<');
                }
                iterate_std_template_recursive_helper<F, S>{}(buf, bin);
                buf.back() = '>';
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const std::pair<F, S>& value) {
                buf.push_back('{');
                iterate_std_template_stuff_impl<std::remove_cvref_t<F>>{}(buf, formatter, value.first);
                iterate_std_template_stuff_impl<std::remove_cvref_t<S>>{}(buf, formatter, value.second);
                buf.back() = '}';
                buf.push_back(',');
            }
            template <class Reader>
            constexpr auto operator()(std::string::const_iterator& iter, Reader reader, std::pair<F, S>& value, int department) {
                iterate_std_template_stuff_impl<F>{}(iter, reader, value.first, department);
                iterate_std_template_stuff_impl<F>{}(iter, reader, value.second, department);
            }
        };
        
        template <typename Ty, std::size_t N>
        struct iterate_std_template_stuff_impl<std::array<Ty, N>> {
            template <std::size_t Index = 0, class Formatter>
            constexpr void write_array(std::string& buf, Formatter formatter, const std::array<Ty, N>& value) {
                if constexpr (Index < N) {
                    iterate_std_template_stuff_impl<Ty>{}(buf, formatter, std::get<Index>(value));
                    write_array<Index + 1, Formatter>(buf, formatter, value);
                }
            }
            template <std::size_t Index = 0, class Reader>
            constexpr void read_array(std::string::const_iterator& iter, Reader reader, std::array<Ty, N>& value, int department) {
                if constexpr (Index < N) {
                    iterate_std_template_stuff_impl<Ty>{}(iter, reader, std::get<Index>(value), department);
                    read_array<Index + 1, Reader>(iter, reader, value, department);
                }
            }
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append("std::array<");
                } else {
                    buf.push_back('\x1b');
                    buf.push_back('<');
                }
                iterate_std_template_stuff_impl<Ty>{}(buf, bin);
                if (!bin) {
                    buf.append(std::to_string(N)).push_back('>');
                } else {
                    const std::size_t n = N;
                    buf.append(reinterpret_cast<const char*>(&n), sizeof(std::size_t)).push_back('>');
                }
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const std::array<Ty, N>& value) {
                buf.push_back('{');
                write_array(buf, formatter, value);
                buf.back() = '}';
                buf.push_back(',');
            }
            template <class Reader>
            constexpr auto operator()(std::string::const_iterator& iter, Reader reader, std::array<Ty, N>& value, int department) {
                read_array(iter, reader, value, department);
            }
        };

        template <class ... Args>
        struct iterate_std_template_stuff_impl<std::tuple<Args...>> {
            template <std::size_t Index = 0, class Formatter>
            constexpr void write_tuple(std::string& buf, Formatter formatter, const std::tuple<Args...>& value) {
                if constexpr (Index < sizeof ... (Args)) {
                    iterate_std_template_stuff_impl<std::tuple_element_t<Index, std::tuple<Args...>>>{}(buf, formatter, std::get<Index>(value));
                    write_tuple<Index + 1, Formatter>(buf, formatter, value);
                }
            }
            template <std::size_t Index = 0, class Reader>
            constexpr void read_tuple(std::string::const_iterator& iter, Reader reader, std::tuple<Args...>& value, int department) {
                if constexpr (Index < sizeof ... (Args)) {
                    iterate_std_template_stuff_impl<std::tuple_element_t<Index, std::tuple<Args...>>>{}(iter, reader, std::get<Index>(value), department);
                    read_tuple<Index + 1, Reader>(iter, reader, value, department);
                }
            }
            constexpr auto operator()(std::string& buf, bool bin) const {
                if (!bin) {
                    buf.append("std::tuple<");
                } else {
                    buf.push_back('\x1c');
                    buf.push_back('<');
                }
                iterate_std_template_recursive_helper<Args...>{}(buf, bin);
                buf.back() = '>';
                buf.push_back(',');
            }
            template <class Formatter>
            constexpr auto operator()(std::string& buf, Formatter formatter, const std::tuple<Args...>& value) {
                buf.push_back('{');
                write_tuple(buf, formatter, value);
                buf.back() = '}';
                buf.push_back(',');
            }
            template <class Reader>
            constexpr auto operator()(std::string::const_iterator& iter, Reader reader, std::tuple<Args...>& value, int department) {
                read_tuple(iter, reader, value, department);
            }
        };
        
    }

    template <class Ty>
    concept std_type = details::std_template_library_type_traits<Ty>::value || details::std_basic_type<Ty>;

    template <class Ty>
    concept structure_type = std::is_class_v<Ty>;
    
    // Further type string all use this.
    template <typename Ty>
    constexpr auto std_type_name_string(bool bin = false) noexcept {
        std::string buffer;
        details::iterate_std_template_stuff_impl<Ty>{}(buffer, bin);
        if (!bin) {
            buffer.pop_back(); // Remove last ','
        }
        else {
            buffer.back() = '\0';
        }
        return buffer;
    }

    template <class Ty>
    constexpr auto structure_type_name_string() {
        std::string buf(1, '\xFF');
        buf.append(serializer<Ty>::type_name).push_back('\0');
        return buf;
    }

    template <typename Ty, class Formatter>
    constexpr auto std_type_value_string(const Ty& value, Formatter formatter) {
        std::string buffer;
        details::iterate_std_template_stuff_impl<Ty>{}(buffer, formatter, value);
        buffer.back() = ';';
        return buffer;
    }

    template <class Ty>
    constexpr auto std_text_value_of(const Ty& value);

    typedef enum std_basic_io_flag{
        integer_binary            = 1 << 1,
        integer_heximal           = 1 << 2,
        floating_point_fixed      = 1 << 3,
        floating_point_scientific = 1 << 4,
        string_use_raw            = 1 << 5,
    } std_basic_io_flag;

    struct std_basic_type_text_output_formatter {
        flag_t flag{};
        
        template <details::std_basic_type Ty>
        constexpr void operator()(std::string& buf, const Ty& value) {
            if constexpr (details::std_string_type_traits<Ty>::value) {
                if (flag & string_use_raw) {
                    buf.append("R\"(").append(value).append(")\"");
                }
                else {
                    std::string cache;
                    cache.reserve(value.size());
                    for (std::size_t i = 0; i != value.size(); ++i) {
                        switch (value[i]) {
                        default: cache.push_back(value[i]); break;
                        case '\n': cache.append("\\n");     break;
                        case '\t': cache.append("\\t");     break;
                        case '\r': cache.append("\\r");     break;
                        case '\b': cache.append("\\b");     break;
                        case '\v': cache.append("\\v");     break;
                        case '\f': cache.append("\\f");     break;
                        case '\a': cache.append("\\a");     break;
                        case '\"': cache.append("\\\"");    break;
                        case '\\': cache.append("\\");      break;
                        }
                    }
                    buf.push_back('\"');
                    buf.append(cache);
                    buf.push_back('\"');
                }
            }
            else if constexpr (std::floating_point<Ty>) {
                std::chars_format fmt = std::chars_format::general;
                if (flag & floating_point_fixed)      { fmt = std::chars_format::fixed; }
                if (flag & floating_point_scientific) { fmt = std::chars_format::scientific; }
                char buffer[32];
                auto end = std::to_chars(buffer, buffer + 32, value, fmt).ptr;
                buf.append(buffer, end - buffer);
            }
            else if constexpr (std::integral<Ty> && !std::is_same_v<Ty, bool>) {
                int base = 10;
                if constexpr (std::is_unsigned_v<Ty>) {
                    if (flag & integer_binary)  { base = 2;   buf.append("0b"); }
                    if (flag & integer_heximal) { base = 16;  buf.append("0x"); }
                }
                char buffer[32];
                auto end = std::to_chars(buffer, buffer + 32, value, base).ptr;
                buf.append(buffer, end - buffer);
            }
            else if constexpr (std::is_same_v<Ty, bool>) {
                std::string_view v = value ? "true" : "false";
                buf.append(v);
            }
        }
    };

    struct std_basic_type_binary_input_reader {
        flag_t flag{};
        
        template <details::std_basic_type Ty>
        constexpr void operator()(std::string::const_iterator& iter, Ty& value) {
            if (std::is_arithmetic_v<Ty>) {
                value = *reinterpret_cast<const Ty*>(&*iter);
                iter += sizeof(Ty);
            }
            else if constexpr (details::std_string_type_traits<Ty>::value) {
                if constexpr (details::std_string_type_traits<Ty>::is_view) {
                    throw std::invalid_argument("Reader can not accept a string_view");
                }
                const std::size_t len = std::strlen(&*iter);
                value.resize(len);
                std::memcpy(value.data(), &*iter, len);
                iter += static_cast<std::ptrdiff_t>(len + 1);
            }
        }
        
    };

    //////////////////////////////////////////////////////////////////////////////////////////////
    ///                               Compiler Implementation
    //////////////////////////////////////////////////////////////////////////////////////////////
    
    struct cpp_subset_compiler {
        std::string src;
        std::string msg;
        std::string out;
        
        static constexpr std::string_view keywords[255] = {
            "int8_t",       "uint8_t",   "int16_t",        "uint16_t",
            "int",          "uint32_t",  "int64_t",        "uint64_t",
            "float",        "double",    "bool",           "std::string",    
            // Containers.
            "std::vector", "std::deque",  "std::list", "std::forward_list", "std::hive", "std::set", "std::multiset",
            "std::unordered_set", "std::unordered_multiset", "std::map", "std::multimap", "std::unordered_map", "std::unordered_multimap",
            "std::pair",           "std::array",             "std::tuple",
            
            // Structure.
            "struct", "class"
        };

        static constexpr std::string_view operators[] = {
            ",", "{", "}", "<", ">", ";", "="
        };
        
        static constexpr std::string_view isfx[] = {
            "u", "U", "l", "L", "ll", "LL", "z", "Z", "uz", "UZ", "ul", "UL", 
            "ull", "ULL", "llu", "LLU", "zu", "ZU"
        };

        constexpr operator bool() const noexcept {
            return msg.empty();
        }

        constexpr void remove_comments() noexcept {
            out.clear();
            out.reserve(src.size());
            bool         is_within_raw = false;
            std::size_t  quote_count   = 0;
            for (std::size_t i = 0; i < src.length(); ++i) {
                switch (src[i]) {
                case 'R':
                    if (src[i + 1] == '\"' && src[i + 2] == '(') {
                        is_within_raw = true;
                        i += 2; out.append("R\"(");
                    } else {
                        out.push_back(src[i]);
                    } break;
                case ')':
                    if (src[i + 1] == '\"' && is_within_raw == true) {
                        is_within_raw = false;
                        ++i; out.append(")\"");
                    } else {
                        out.push_back(src[i]);
                    } break;
                case '\"':
                    if (!is_within_raw) {
                        ++quote_count; 
                    }
                    out.push_back(src[i]); break;
                case '/':
                    // Quotes matched.
                    if ((quote_count & 1) == 0) {
                        // Single line comment.
                        if (src[i + 1] == '/') {
                            i = src.find('\n', i + 1);
                            if (i == std::string_view::npos) {
                                return;
                            }
                        }
                        // Multi line comment.
                        else if (src[i + 1] == '*') {
                            i = src.find("*/", i + 2);
                            if (i == std::string_view::npos) {
                                return;
                            }
                            i += 2;
                        }
                        else {
                            msg = "Invalid character after /";
                            return;
                        }
                    } break;
                default: out.push_back(src[i]); break;
                }
            }
        } // remove_comments.
        
        // We don't need function macro since cpod doesn't support expssions.
        template <class StrAlloc, typename ... Rest>
        constexpr void get_macro_define_map(std::unordered_map<
            std::string_view,
            std::basic_string<char, std::char_traits<char>, StrAlloc>, Rest...>& macro_map) {
            out.clear();
            out.reserve(src.size());
            for (std::size_t i = 0; i != src.size(); ++i) {
                switch (src[i]) {
                default: out.push_back(src[i]); break;
                case '#': {
                    char *k = &src[i];
                    char *l = nullptr;
                    k = std::find_if_not(k + 1, &src[src.length()], [](auto& c) { return std::isspace(c); });
                    const char* cmd_begin = &*k;

                    // Read key value pair from define.
                    if (std::memcmp(cmd_begin, "define", 6) == 0) {
                        k = std::find_if_not(k + 6, &src[src.length()], [](auto& c) { return std::isspace(c); });
                        l = std::find_if(k, &src[src.length()], [](auto& c) { return std::isspace(c); });
                        const std::string_view macro_key(k, l - k);

                        k = std::find_if_not(l, &src[src.length()], [](auto& c) { return std::isspace(c); });
                        // LF mode. CR LF mode is not supported. make sure your source file is using LF new line.
                        l = std::find_if(k, &src[src.length()], [](auto& c) { return c == '\n' && (&c)[-1] != '\\'; });
                        std::basic_string<char, std::char_traits<char>, StrAlloc> macro_value(k, l - k);
                        std::erase_if(macro_value, [](auto& c) { return c == '\\' && (&c)[1] == '\n'; });
                        std::erase_if(macro_value, [](auto& c) { return c == '\n'; });
                        macro_map.insert(std::make_pair(macro_key, macro_value));
                        i += (l - &src[i]);
                    } else {
                        out.push_back(src[i]);
                    }
                } break;
                }
            }
        }

        template <class StrAlloc, typename ... Rest>
        static constexpr void expand_macro_value(std::unordered_map<
            std::string_view,
            std::basic_string<char, std::char_traits<char>, StrAlloc>, Rest...>& macro_map,
            std::string_view                                                     key) {
            auto& value = macro_map[key];
            for (auto it = value.begin(); it != value.end(); ++it) {
                switch (*it) {
                default: break;
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
                case 'm': case 'n': case 'o': case 'p': case 'q': case 's':
                case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
                case 'M': case 'N': case 'O': case 'P': case 'Q': case 'S':
                case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
                case '_': {
                    auto ed = std::find_if_not(it, value.end(), [](auto& c) { return std::isalnum(c) || std::isdigit(c) || c == '_'; });
                    std::string_view found_key(&*it, ed - it);
                    if (macro_map.contains(found_key)) {
                        const std::size_t delta = it - value.begin();
                        value.replace(it, ed, macro_map[found_key]);
                        it = value.begin() + delta - 1;
                    } else {
                        it += ed - it - 1;
                    }
                } break;
                }
            }
        }

        template <class StrAlloc, typename ... Rest>
        constexpr void expand_conditional_macros(std::unordered_map<
            std::string_view,
            std::basic_string<char, std::char_traits<char>, StrAlloc>, Rest...>& macro_map) {
            out.clear();
            out.reserve(src.size());

            bool is_inside_check_scope = false;
            bool is_ifdef              = false;
            bool is_defined            = false;

            for (auto it = src.begin(); it != src.end(); ++it) {
                switch (*it) {
                default:
                    if (!is_inside_check_scope || (is_inside_check_scope && is_ifdef == is_defined)) {
                        out.push_back(*it);
                    } break;
                case '#': {
                    auto j = std::find_if_not(it + 1, src.end(), [](auto& c) { return std::isspace(c); });
                    auto k = std::find_if(j, src.end(), [](auto& c) { return std::isspace(c); });
                    std::string_view cmd(&*j, k - j);
                    if (cmd == "ifdef" || cmd == "ifndef") {
                        j = std::find_if_not(k, src.end(), [](auto& c) { return std::isspace(c); });
                        k = std::find_if(j, src.end(), [](auto& c) { return std::isspace(c); });
                        std::string_view key(&*j, k - j);
                        is_inside_check_scope = true;
                        is_ifdef              = cmd == "ifdef";
                        is_defined            = macro_map.contains(key);
                    }
                    else if (cmd == "endif") {
                        is_inside_check_scope = false;
                        is_defined            = false;
                        is_ifdef              = false;
                    }
                    k = std::find(k, src.end(), '\n');
                    it = k;
                    if (it == src.end()) {
                        --it;
                    }
                } break;
                }
            }
        }

        template <class StrAlloc, typename ... Rest>
        constexpr void replace_remove_macros(const std::unordered_map<
            std::string_view,
            std::basic_string<char, std::char_traits<char>, StrAlloc>, Rest...>& macro_map) {
            out.clear();
            out.reserve(src.size());
            for (std::size_t i = 0; i != src.size(); ++i) {
                switch (src[i]) {
                default: out.push_back(src[i]); break;
                case 'a': case 'b': case 'c': case 'd': case 'e': case 'f':
                case 'g': case 'h': case 'i': case 'j': case 'k': case 'l':
                case 'm': case 'n': case 'o': case 'p': case 'q': case 's':
                case 't': case 'u': case 'v': case 'w': case 'x': case 'y': case 'z':
                case 'A': case 'B': case 'C': case 'D': case 'E': case 'F':
                case 'G': case 'H': case 'I': case 'J': case 'K': case 'L':
                case 'M': case 'N': case 'O': case 'P': case 'Q': case 'S':
                case 'T': case 'U': case 'V': case 'W': case 'X': case 'Y': case 'Z':
                case '_': {
                    char* k = &src[i];
                    k = std::find_if_not(&src[i], &src[src.length()], [](auto& c) { return std::isalnum(c) || std::isdigit(c) || c == ':' || c == '_'; });
                    std::string_view key(&src[i], k - &src[i]);
                    if (macro_map.contains(key)) {
                        out.append(macro_map.at(key));
                    } else {
                        out.append(key);
                    }
                    i += k - &src[i] - 1;
                } break;
                }
            }
        }

        // Change all escape characters to their original forms.
        // And all string literals will be raw string (with R prefix) after this method call.
        constexpr void normalize_string_literals() noexcept {
            out.clear();
            out.reserve(src.size());
            for (std::size_t i = 0; i != src.length(); ++i) {
                switch (src[i]) {
                case 'R':
                    if (src[i + 1] == '\"' && src[i + 2] == '(') {
                        std::size_t j = src.find(")\"", i + 3);
                        if (j == std::string_view::npos) {
                            msg = "Unmatched raw string literals!";
                            return;
                        }
                        out.append(std::string_view(src.data() + i + 1, j + 1 - i));
                        i = j + 1;
                    } break;
                case '\"': {
                    // Doesn't support multiline string.
                    std::size_t j = i + 1;
                    out.append("\"(");
                    for (; j != src.length() && (src[j] != '\"' || src[j - 1] == '\\'); ++j) {
                        if (src[j] == '\\') {
                            switch (src[j + 1]) {
                            case 'n':  out.push_back('\n'); break;
                            case 'r':  out.push_back('\r'); break;
                            case 't':  out.push_back('\t'); break;
                            case 'b':  out.push_back('\b'); break;
                            case 'f':  out.push_back('\f'); break;
                            case 'v':  out.push_back('\v'); break;
                            case '\"': out.push_back('\"'); break;
                            case '\\': out.push_back('\\'); break;
                            case '\'': out.push_back('\''); break;
                            default:
                                msg = "Invalid escape character!";
                                return;
                            }
                            ++j;
                        } else {
                            out.push_back(src[j]);
                        }
                    }
                    if (j == src.length()) {
                        msg = "Unmatched string quote!";
                        return;
                    }
                    out.append(")\"");
                    i = j;
                } break;
                default:
                    out.push_back(src[i]); break;
                }
            } // for loop
        } // normalize_string

        constexpr void combine_string_literals() noexcept {
            out.clear();
            out.reserve(src.size());
            for (std::size_t i = 0; i != src.size(); ++i) {
                switch (src[i]) {
                default:  out.push_back(src[i]); break;
                case ')':
                    if (src[i + 1] == '\"') {
                        std::size_t j = src.find("\"(", i + 2);
                        std::size_t k = src.find_first_of(";,}", i + 2);
                        if (k < j || j == std::string_view::npos) {
                            out.push_back(src[i]);
                            out.push_back(src[i + 1]);
                            i = k - 1;
                        } else {
                            i = j + 1;
                        }
                    } else {
                        out.push_back(src[i]);
                    } break;
                }
            }
        }

        // This step must after remove comment and normalize string.
        template <typename Iter>
        constexpr void tokenize_source(Iter it) noexcept {
            for (std::size_t i = 0; i < src.length(); ++i) {
                if (std::isspace(src[i])) {
                    auto p = std::find_if_not(&src[i], &src[src.length()], [](auto& c) {
                        return std::isspace(static_cast<int>(c));
                    });
                    i = p - src.data() - 1;
                }
                else if (std::isalpha(src[i]) || src[i] == '_' || src[i] == ':') {
                    auto p = std::find_if_not(&src[i], &src[src.length()], [](auto ch) {
                        return std::isalnum(ch) || ch == '_' || ch == ':';
                    });
                    *it++ = std::string_view(&src[i], p - &src[i]);
                    i = p - src.data() - 1;
                }
                else if (src[i] == '\"') {
                    // This step won't fail because we have successfully normalized all strings in normalize_string.
                    std::size_t j = src.find(")\"", i + 2);
                    *it++ = std::string_view(&src[i], j - i + 2);
                    i = j + 1;
                }
                else if (auto op = std::string_view(&src[i], 1);
                    std::find(std::begin(operators), std::end(operators), op) != std::end(operators)) {
                    *it++ = op;
                }
                else if (std::isxdigit(src[i]) || src[i] == '.' || src[i] == '-' || src[i] == '+') {
                    auto p = std::find_if_not(&src[i], &src[src.length()], [](auto ch) {
                        return std::isxdigit(ch) || ch == '.' || ch == '-' || ch == '+';
                    });
                    std::size_t j = i;
                    i = p - src.data() - 1;
                    if (std::find(std::begin(isfx), std::end(isfx), std::string_view(p, 1)) != std::end(isfx)) {
                        ++p; ++i;
                    }
                    if (std::find(std::begin(isfx), std::end(isfx), std::string_view(p, 2)) != std::end(isfx)) {
                        ++p; ++i;
                    }
                    if (std::find(std::begin(isfx), std::end(isfx), std::string_view(p, 3)) != std::end(isfx)) {
                        ++p; ++i;
                    }
                    --p;
                    *it++ = std::string_view(&src[j], p - &src[j] + 1);
                }
                else {
                    msg = "Invalid character!";
                    return;
                }
            } // for loop.
        } // tokenize_source.

        template <details::std_basic_type Ty>
        static constexpr Ty compile_basic_value(std::string_view value) {
            if constexpr (std::integral<Ty> && !std::is_same_v<Ty, bool>) {
                const char* beg  = value.data();
                int   base = 10;
                if (value[0] == '0') {
                    // Means we have only one zero.
                    if (value.length() == 1) {
                        return 0;
                    }
                    if (value[1] == 'x' || value[1] == 'X') {
                        base = 16; beg += 2;
                    }
                    else if (value[1] == 'b' || value[1] == 'B') {
                        base = 2;  beg += 2;
                    }
                }
                Ty result = 0;
                std::from_chars(beg, value.data() + value.length(), result, base);
                return result;
            }
            if constexpr (std::floating_point<Ty>) {
                Ty result = 0;
                std::from_chars(value.data(), value.data() + value.length(), result, std::chars_format::general);
                return result;
            }
            if constexpr (std::is_same_v<Ty, bool>) {
                if (value == "true")  { return true; }
                if (value == "false") { return false; }
            }
            return Ty{};
        }
        
        static constexpr void compile_basic_type_to_buffer(std::string_view type, std::string_view value, std::string& buf) {
    #define DEFINE_COMPILE_FIXED_VALUE(t)                             \
        do {                                                          \
        if (type == #t) {                                             \
            const auto v = compile_basic_value<t>(value);             \
            buf.append(reinterpret_cast<const char*>(&v), sizeof(v)); \
            return;                                                   \
        }} while(false)                                               
            DEFINE_COMPILE_FIXED_VALUE(int8_t);
            DEFINE_COMPILE_FIXED_VALUE(uint8_t);
            DEFINE_COMPILE_FIXED_VALUE(int16_t);
            DEFINE_COMPILE_FIXED_VALUE(uint16_t);
            DEFINE_COMPILE_FIXED_VALUE(int);
            DEFINE_COMPILE_FIXED_VALUE(uint32_t);
            DEFINE_COMPILE_FIXED_VALUE(int64_t);
            DEFINE_COMPILE_FIXED_VALUE(uint64_t);
            DEFINE_COMPILE_FIXED_VALUE(float);
            DEFINE_COMPILE_FIXED_VALUE(double);
            DEFINE_COMPILE_FIXED_VALUE(bool);
    #undef DEFINE_COMPILE_FIXED_VALUE
            // String requires special handling.
            if (type == "std::string") {
                buf.append(value.data() + 2, value.length() - 4);
                buf.push_back('\0');
            }
        }

        template <char B1, char B2, class Iter>
        constexpr auto find_matching_bracket(Iter b, Iter e) {
            std::size_t brace_count = 1;
            auto i = std::next(b);
            for (; i != e && brace_count != 0; ++i) {
                if ((*i)[0] == B1) { ++brace_count; }
                if ((*i)[0] == B2) { --brace_count; }
            }
            return std::prev(i);
        }

        template <class Iter>
        constexpr auto compile_values_recursively(Iter ttb, Iter tte, Iter vtb, Iter vte, std::string& buf) {
            // Means basic type -- recursive end scenario.
            const std::size_t tid = std::find(std::begin(keywords), std::end(keywords), *ttb) - std::begin(keywords) + 1;
            if (tid < 13) {
                compile_basic_type_to_buffer(*ttb, *vtb, buf);
                return  std::make_pair(std::next(ttb), std::next(vtb)) ;
            }
            // Template types.
            if (tid > 12 && tid < 29) {
                // Recursive variables.
                tte = find_matching_bracket<'<', '>'>(std::next(ttb), tte);
                vte = find_matching_bracket<'{', '}'>(vtb, vte);
                ttb = std::next(ttb, 2);
                std::string cache;
                std::size_t n = 0;
                // Branch recursion.
                switch(tid) {
                default: break;
                // Sequential containers (not map nor pair && tuple && array)
                case 13: case 14: case 15: case 16: case 17: case 18: case 19: case 20: case 21:
                    for (auto k = vtb; k != vte; ++n) {
                        k = compile_values_recursively(ttb, tte, std::next(k), vte, cache).second;
                    }
                    buf.append(reinterpret_cast<const char*>(&n), sizeof(n)); break;
                // Mapping containers 
                case 22: case 23: case 24: case 25:
                    for (auto k = vtb; k != vte; ++n) {
                        auto p1 = compile_values_recursively(ttb, tte, std::next(k, 2), vte, cache);
                        auto p2 = compile_values_recursively(std::next(p1.first), tte, std::next(p1.second), vte, cache);
                        k = std::next(p2.second);
                    }
                    buf.append(reinterpret_cast<const char*>(&n), sizeof(n)); break;
                // std::pair;
                case 26: {
                    auto p1 = compile_values_recursively(ttb, tte, std::next(vtb), vte, cache);
                    auto p2 = compile_values_recursively(std::next(p1.first), tte, std::next(p1.second), vte, cache); } break;
                // std::array
                case 27:
                    // The only difference between sequential containers is this do not write n into the buffer.
                    for (auto k = vtb; k != vte;) {
                        k = compile_values_recursively(ttb, tte, std::next(k), vte, cache).second;
                    } break;
                // std::tuple.
                case 28:
                    for (Iter k = vtb, l = ttb;k != vte && l != tte;) {
                        auto c = compile_values_recursively(l, tte, std::next(k), vte, cache);
                        l = std::next(c.first);
                        k = c.second;
                    } break;
                }
                // Common operation that writes cache to the buffer and move forward iterator.
                buf.append(cache);
                return std::make_pair(std::next(tte), std::next(vte));
            }
            if (tid == 29 || tid == 30) {
                ttb = std::next(ttb, 3);
                for (auto k = ttb; k != vte; ++k) {
                    if (*k != "struct" && *k != "class") {
                        auto assign = std::find(k, vte, "=");
                        auto semico = std::find(assign, vte, ";");
                        compile_values_recursively(k, std::prev(assign), std::next(assign), semico, buf);
                        k = semico;
                    } else {
                        auto h = find_matching_bracket<'{','}'>(std::next(k, 2), vte);
                        compile_values_recursively(k, std::next(k, 2), std::next(k, 2),h, buf);
                        k = std::next(h, 2);
                    }
                }
                return std::make_pair(std::next(tte), std::next(vte));
            }
            return std::make_pair(std::next(tte), std::next(vte));
        }

        template <class Iter>
        static constexpr std::string compile_type_name(Iter ttb, Iter tte) {
            std::string buf;
            for (auto it = ttb; it != tte; ++it) {
                if      (*it == ",") { buf.push_back(','); }
                else if (*it == "<") { buf.push_back('<'); }
                else if (*it == ">") { buf.push_back('>'); }
                else {
                    // For array size.
                    if (std::all_of(it->begin(), it->end(), [](auto& c) {
                        return std::isdigit(static_cast<int>(c));
                    })) {
                        std::size_t n = 0;
                        std::from_chars(&*it->begin(), (&*it->rbegin()) + 1, n);
                        buf.append(reinterpret_cast<const char*>(&n), sizeof(std::size_t));
                    } else {
                        const std::uint8_t t = static_cast<std::uint8_t>(std::find(std::begin(keywords), std::end(keywords), *it) - std::begin(keywords)) + 1;
                        buf.push_back(*reinterpret_cast<const char*>(&t));
                    }
                }
            }
            buf.push_back('\0');
            return buf;
        }
        
        template <class Container>
        constexpr void generate_byte_code(const Container& tokens) {
            out.clear();
            out.reserve(tokens.size());
            for (auto t = tokens.begin(); t != tokens.end(); ++t) {
                if (auto i = std::find(std::begin(keywords), std::end(keywords), *t); i != std::end(keywords)) {
                    std::string              value_cache;
                    std::string              variable_name_cache;
                    std::string              type_cache;
                    decltype(tokens.end())   semicolumn;
                    
                    if (*t == "struct" || *t == "class") {
                        type_cache.push_back('\xFF');
                        type_cache.append(*std::next(t));
                        type_cache.push_back('\0');
                        auto struct_end = find_matching_bracket<'{', '}'>(std::next(t, 2), tokens.end());
                        variable_name_cache = *std::next(struct_end);
                        variable_name_cache.push_back('\0');
                        semicolumn = std::next(struct_end, 2); 
                        compile_values_recursively(t, std::next(t, 2), std::next(t, 2), struct_end, value_cache);
                    }
                    else {
                        auto assign = std::find(t, tokens.end(), "=");
                        if (assign == tokens.end()) {
                            msg = "Missing assign operator (=).";
                            return;
                        }
                        semicolumn = std::find(assign, tokens.end(), ";");
                        if (semicolumn == tokens.end()) {
                            msg = "Missing ; after expression.";
                            return;
                        }
                        type_cache = compile_type_name(t, std::prev(assign));
                        variable_name_cache = *std::prev(assign);
                        variable_name_cache.push_back('\0');
                        compile_values_recursively(t, std::prev(assign), std::next(assign), semicolumn, value_cache);
                        t = semicolumn;
                    }
                    
                    t = semicolumn;
                    const std::size_t offset = type_cache.size() + variable_name_cache.size() + value_cache.size();
                    out.append(reinterpret_cast<const char*>(&offset), sizeof(std::size_t));
                    out.append(type_cache);
                    out.append(variable_name_cache);
                    out.append(value_cache);
                }
            } // for loop
            constexpr std::size_t end_mark = 0;
            out.append(reinterpret_cast<const char*>(&end_mark), sizeof(std::size_t));
        } // Generate byte code.
    };

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///                                    archive media function.
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    template <class Ty>
    constexpr std::string::const_iterator archive::find_variable_begin(std::string_view var_name) {
        // Search tag.
        std::string type_and_name;
        if constexpr (std_type<Ty>) {
            type_and_name = std_type_name_string<Ty>(true);
        } else {
            type_and_name = structure_type_name_string<Ty>();
        }
        type_and_name.append(var_name).push_back('\0');
        // Skip-field variable checking & searching method.
        auto offset_block = content_.cbegin();
        for (std::size_t
            offset = *reinterpret_cast<const std::size_t*>(&*offset_block);
            offset != 0;
            offset = *reinterpret_cast<const std::size_t*>(&*offset_block)) {
            // A very weird technique I developed. 
            offset_block += sizeof(std::size_t);
            if (std::equal(type_and_name.cbegin(), type_and_name.cend(), offset_block)) {
                return offset_block + type_and_name.size();
            }
            offset_block += static_cast<std::size_t>(offset);
        }
        return content_.cend();
    }
    
    inline std::string archive::compile_content_default(std::initializer_list<std::pair<std::string_view, std::string>> init_macro_map) noexcept {
        cpp_subset_compiler compiler(std::move(content_));
        std::vector<std::string_view>                     token_list;
        std::unordered_map<std::string_view, std::string> macro_map(init_macro_map.begin(), init_macro_map.end());

        compiler.remove_comments(); compiler.src = compiler.out;             
        compiler.get_macro_define_map(macro_map);
        std::string out_source = std::move(compiler.src);

        for (auto& i : macro_map) {
            cpp_subset_compiler::expand_macro_value(macro_map, i.first);
        }

        compiler.src = compiler.out; compiler.expand_conditional_macros(macro_map);
        compiler.src = compiler.out; compiler.replace_remove_macros(macro_map);
        compiler.src = compiler.out; compiler.normalize_string_literals();
        compiler.src = compiler.out; compiler.combine_string_literals();
        compiler.src = compiler.out;
        
        compiler.tokenize_source(std::back_inserter(token_list));
        compiler.generate_byte_code(token_list);
        content_ = compiler.out;
        return std::move(compiler.msg);
    }

    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///                                Structure serializer helper
    //////////////////////////////////////////////////////////////////////////////////////////////////////////

    // Make user custom class's serialization easier.
    template <class Ty, bool IsClass = false>
    struct auto_structure_description_writer {
        archive              *arch;
        std::string_view      varname;
        bool                  auto_indent;

        constexpr explicit auto_structure_description_writer(archive& ac, std::string_view var_name, bool idn = true)
        : arch(&ac), varname(var_name), auto_indent(idn) {
            if (auto_indent) { arch->append_indent(); }
            if constexpr (IsClass) {
                arch->content().append("class ");
            } else {
                arch->content().append("struct ");
            }
            arch->content().append(serializer<Ty>::type_name);
            arch->content().push_back('{');
            if (auto_indent) { arch->indent() += 4; *arch << '\n'; }
        }
        
        ~auto_structure_description_writer() {
            if (auto_indent) {
                arch->indent() -= 4;
                arch->append_indent();
            }
            arch->content().push_back('}');
            arch->content().append(varname);
            arch->content().push_back(';');
        }
    };

    template <class Ty>
    constexpr auto std_text_value_of(const Ty& value) {
        std_basic_type_text_output_formatter formatter{0};
        return std_type_value_string(value, formatter);
    }
    
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    ///                                Basic serializer specialization
    //////////////////////////////////////////////////////////////////////////////////////////////////////////
    
    template <std_type Ty>
    struct serializer<Ty> {
        constexpr void operator()(archive& arch, std::string_view name, const Ty& v, flag_t flag) {
            std_basic_type_text_output_formatter formatter{flag};
            arch.append_indent();
            arch << std_type_name_string<Ty>() << ' '
                 << name  << '='
                 << std_type_value_string(v, formatter);
        }
        constexpr void operator()(std::string::const_iterator& mem_begin, Ty& v, flag_t flag) {
            // Reader is much shorter and thus faster.
            std_basic_type_binary_input_reader reader{flag};
            details::iterate_std_template_stuff_impl<Ty>{}(mem_begin, reader, v, 0);
        }
    };
    
}