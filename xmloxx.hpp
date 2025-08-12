#pragma once
#include <format>
#include <string>
#include <vector>
#include <ranges>
#include <algorithm>

namespace xmloxx {
    
    typedef struct attribute {
        std::string key;
        std::string value;
    } attribute;

    typedef struct node_data {
        
        typedef enum node_flags : std::uint8_t {
            flag_none        = static_cast<node_flags>(0),
            flag_comment     = static_cast<node_flags>(1 << 0),
            flag_single_line = static_cast<node_flags>(1 << 1),
            flag_begin_brace = static_cast<node_flags>(1 << 2),
        } node_flags;
        
        node_flags                  flags = flag_none;
        std::string                 name;
        std::string                 content;
        std::vector<attribute>  attributes;

        // For copy usage.
        node_data() = default;
        node_data(std::string_view name, node_flags fs = flag_none) : flags(fs), name(name), content(), attributes() {
            attributes.reserve(8);
        }

        node_data(const node_data&) = default;
        node_data(node_data&&) = default;
        node_data& operator=(const node_data&) = default;
        node_data& operator=(node_data&&) = default;
        
    } node_data;

    class tree_node {
        node_data                      current_;
        tree_node*                     parent_;
    public:

        using iterator_attribute = std::vector<attribute>::iterator;
        // For copy usage.
        tree_node() = default;
        tree_node(std::string_view name, tree_node* p, node_data::node_flags fs = node_data::flag_none) : current_(name, fs), parent_(p) {}
        tree_node(const tree_node& right) = default;
        tree_node(tree_node&& right) = default;
        tree_node& operator=(const tree_node& right) = default;
        tree_node& operator=(tree_node&&) = default;
        ~tree_node() = default;

        constexpr inline const tree_node* parent() const { return parent_; }
        constexpr inline       tree_node* parent()       { return parent_; }

        constexpr inline tree_node* push_attribute(std::string_view key, std::string_view val) {
            current_.attributes.emplace_back(key.data(), val.data());
            return this;
        }

        constexpr inline bool is_comment() const {
            return current_.flags & node_data::flag_comment;
        }

        constexpr inline tree_node* name(std::string_view name) {
            current_.name.assign(name);
            return this;
        }

        constexpr inline std::string_view name() const {
            return current_.name;
        }

        constexpr inline tree_node*   text(std::string_view txt) {
            current_.content.assign(txt);
            return this;
        }

        constexpr inline std::string_view text() const {
            return current_.content;
        }
        
        constexpr inline iterator_attribute begin_attribute() {
            return current_.attributes.begin();
        }
        
        constexpr inline iterator_attribute end_attribute() {
            return current_.attributes.end();
        }

        constexpr inline iterator_attribute find_attribute(std::string_view key) {
            return std::ranges::find_if(current_.attributes, [key](const auto& a) {
                return key == a.key;
            });
        }

        // Make sure it's still valid after sort_to.
        constexpr inline std::size_t    depth() const {
            std::size_t result = 0;
            for (auto j = parent_; j != nullptr; j = j->parent_) { ++result; }
            return result;

        }
        
        constexpr inline std::string    attributes_to_string() const {
            std::string  buffer;
            for (auto& [k, v] : current_.attributes) { buffer.append(std::format(" {:s}=\"{:s}\"", k, v)); }
            return buffer;
        }

        constexpr inline std::string    to_string(std::size_t depth, node_data::node_flags fs) const {
            std::string space(depth << 1, ' ');
            if (is_comment()) {
                if (fs & node_data::flag_begin_brace) {
                    return std::format("{:s}<!--{:s}-->\n", space, current_.name);
                }
            } else {
                if (!(fs & node_data::flag_single_line)) {
                    return (fs & node_data::flag_begin_brace) ? std::format("{:s}<{:s}{:s}>{:s}\n", space, current_.name, attributes_to_string() , current_.content) :
                    std::format("{:s}</{:s}>\n", space, current_.name);
                } 
                return current_.content.empty() ? std::format("{:s}<{:s}{:s}/>\n", space, current_.name, attributes_to_string()) :
                std::format("{0:s}<{1:s}{3:s}>{2:s}</{1:s}>\n", space, current_.name, current_.content, attributes_to_string());
            }
            return "";
        }
    };


    class tree {
        std::vector<tree_node> nodes_;
    public:
        using iterator       = tree_node*;
        using const_iterator = const tree_node*;
        using node_type      = tree_node;
        
        tree(std::string_view root_name, std::size_t cap = 1 << 10) {
            nodes_.reserve(cap);
            nodes_.emplace_back(root_name, nullptr);
        }
        tree(const tree& right) = default;
        tree(tree&& right) = default;
        tree& operator=(const tree& right) = default;
        tree& operator=(tree&& right) = default;
        ~tree() = default;

        // Begin is root.
        constexpr inline iterator begin() { return &nodes_.front(); }
        constexpr inline iterator end()   { return (&nodes_.back() + 1); }

        constexpr inline const_iterator begin() const { return &nodes_.front(); }
        constexpr inline const_iterator end()   const { return (&nodes_.back() + 1); }
        
        constexpr inline iterator push_node(std::string_view name, iterator p, node_data::node_flags fs = node_data::flag_none) {
            return &nodes_.emplace_back(name, p, fs);
        }

        constexpr inline iterator push_node(std::string_view name, node_data::node_flags fs = node_data::flag_none) {
            return push_node(name, begin(), fs);
        }

        constexpr inline iterator find_first_child(iterator b) {
            return std::ranges::find_if(*this, [b](const auto& it) {
                return it.parent() == b;
            });
        }

        constexpr inline iterator find_first_child_with_name(iterator b, std::string_view name) {
            return std::ranges::find_if(*this, [b, name](const auto& it) {
                return it.parent() == b && it.name() == name;
            });
        }

        constexpr inline iterator find_first_child_with_attribute(iterator b, std::string_view key, std::string_view value) {
            return std::ranges::find_if(*this, [b, key, value](auto& it) {
                auto a = it.find_attribute(key);
                return it.parent() == b && a != it.end_attribute() && a->key == key && a->value == value;
            });
        }
        
        constexpr inline iterator find_first_sibling(iterator b) {
            return std::find_if(b + 1, end(), [b](const auto& it) {
                return it.parent() == b->parent();
            });
        }

        constexpr inline iterator find_first_sibling_with_name(iterator b) {
            return std::find_if(b + 1, end(), [b](const auto& it) {
                return it.parent() == b->parent() && it.name() == b->name();
            });
        }

        constexpr inline iterator find_first_sibling_with_attribute(iterator b, std::string_view key, std::string_view value) {
            return std::find_if(b + 1, end(), [b, key, value](auto& it) {
                auto a = it.find_attribute(key);
                return b->parent() == it.parent() && a != it.end_attribute() && a->key == key && a->value == value;
            });
        }

        constexpr inline iterator find_nth_sibling(iterator b, std::size_t n) {
            for (std::size_t i = 0; b != end() && i != n; b = find_first_sibling(b)) { ++i; }
            return b;
        }

        constexpr inline iterator find_nth_sibling_with_name(iterator b, std::size_t n) {
            for (std::size_t i = 0; b != end() && i != n; b = find_first_sibling_with_name(b)) { ++i; }
            return b;
        }

        constexpr inline std::size_t depth() const {
            return std::ranges::max_element(*this, [](const auto& a, const auto& b) {
                return a.depth() < b.depth();
            })->depth();
        }

        inline std::string to_string() const {
            static constexpr std::string_view format_string = R"(<?xml version="1.0" encoding="utf-8"?>)""\n{:s}";
            // A single root node doesn't require iterations.
            if (depth() == 0) {
                return std::format(format_string, begin()->to_string(0, node_data::flag_single_line));
            }
            std::vector<std::string> node_string_dense(nodes_.size());
            for (std::size_t i = depth(); i != 0; i--) {
                auto                     same_depth_view = *this | std::views::filter([i](auto& e) { return e.depth() == i; });
                auto                     parents_view    = same_depth_view | std::views::transform([](auto& e) { return e.parent(); });
                for (auto parent : parents_view) {
                    if (node_string_dense[parent - begin()].empty()) {
                        std::string element_cache;
                        element_cache.append(parent->to_string(i - 1, node_data::flag_begin_brace));
                        for (auto& j : same_depth_view | std::views::filter([parent](auto& e) { return e.parent() == parent; })) {
                            if (node_string_dense[&j - begin()].empty()) {
                                element_cache.append(j.to_string(i, static_cast<node_data::node_flags>(node_data::flag_begin_brace | node_data::flag_single_line)));
                            } else {
                                element_cache += node_string_dense[&j - begin()];
                            }
                        }
                        element_cache.append(parent->to_string(i - 1, node_data::flag_none));
                        node_string_dense[parent - begin()] = element_cache;
                    }
                }
            }
            return std::format(format_string, node_string_dense.front());
        }
    };
}