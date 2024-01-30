#ifndef WEBXX_H
#define WEBXX_H

//                _                  https://github.com/rthrfrd/webxx
//  __      _____| |____  ___  __
//  \ \ /\ / / _ | '_ \ \/ \ \/ /    MIT License
//   \ V  V |  __| |_) >  < >  <     Copyright (c) 2022
//    \_/\_/ \___|_.__/_/\_/_/\_\    Alexander Carver
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
// SOFTWARE.

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstring>
#include <deque>
#include <functional>
#include <initializer_list>
#include <iterator>
#include <memory>
#include <type_traits>
#include <utility>
#include <string>
#include <string_view>
#include <unordered_set>
#include <vector>


////|        |////
////| Common |////
////|        |////


namespace Webxx { namespace internal {
    static constexpr const char none[] = "";
    static constexpr const char styleTag[] = "style";

    struct Empty {};

    // std::array helpers:

    template<typename, typename = void>
    struct has_size : std::false_type {};
    template<typename T>
    struct has_size<T, std::void_t<typename T::size>> : std::true_type {};
    template<typename T>
    constexpr bool has_size_v = has_size<std::remove_cv_t<std::remove_reference_t<T>>>::value;

    template<typename T>
    constexpr typename std::enable_if_t<!std::is_array_v<T>&&has_size_v<T>, size_t> linkCountOfType () {
        return T::size::value;
    }
    template<typename T>
    constexpr typename std::enable_if_t<!std::is_array_v<T>&&!has_size_v<T>, size_t> linkCountOfType () {
        return 1;
    }
    template<typename T>
    constexpr typename std::enable_if_t<std::is_array_v<T>&&has_size_v<std::remove_extent_t<T>>, size_t> linkCountOfType () {
        return std::extent_v<T>*linkCountOfType<std::remove_extent_t<T>>();
    }
    template<typename T>
    constexpr typename std::enable_if_t<std::is_array_v<T>&&!has_size_v<std::remove_extent_t<T>>, size_t> linkCountOfType () {
        return 1;
    }

    template<typename ...Ts>
    constexpr size_t linkCountOfTypes () {
        return (linkCountOfType< std::remove_cv_t<std::remove_reference_t<Ts>>>() + ... + 0);
    }

    template <typename ...Ts>
    constexpr size_t linkCount = linkCountOfTypes<Ts...>();
}}


////|      |////
////| Tape |////
////|      |////


namespace Webxx { namespace internal {

    // Link:

    struct Link {
        typedef std::integral_constant<std::size_t, 1> size;
        typedef std::function<std::array<Link,1>()> GeneratorFn;

        struct Flag {
            static constexpr const uint8_t passComCss   = 0b10000000;
            static constexpr const uint8_t passComHead  = 0b01000000;
            static constexpr const uint8_t placeholder  = 0b00100000;
            static constexpr const uint8_t insideTag    = 0b00010000;
            static constexpr const uint8_t invisible    = 0b00001000;

            static constexpr const uint8_t maskPass     = 0b11000000;
        };

        enum class Type : uint8_t {
            NONE = 0,
            BRANCH_COM_CSS,
            BRANCH_COM_HEAD,
            BRANCH_DYNAMIC,
            COM_END,
            COM_START,
            CSS_AT_RULE_START,
            CSS_AT_RULE_SELECTOR,
            CSS_AT_RULE_VALUE,
            CSS_BLOCK_START,
            CSS_BLOCK_END,
            CSS_DECLARATION_PROPERTY,
            CSS_DECLARATION_VALUE,
            CSS_RULE_START,
            CSS_SELECTOR,
            HTML_ATTR_END,
            HTML_ATTR_KEY,
            HTML_ATTR_VALUE,
            HTML_TAG_END,
            HTML_TAG_START,
            HTML_TAG_START_SELF_CLOSE,
            BRANCH_LAZY,
            TEXT,
        };

        struct View {
            const char* ptr{none};
            size_t len{0};
            constexpr operator std::string_view() const {
                return {this->ptr, this->len};
            }
        };

        Type type;
        uint8_t flags;
        uint32_t spare;
        mutable std::string* own;
        View view;
        mutable std::vector<Link>* dynamic;
        mutable GeneratorFn* generator;

        constexpr Link () noexcept :
            type{Type::NONE},
            flags{Flag::insideTag | Flag::invisible},
            spare{0},
            own{nullptr},
            view{},
            dynamic{nullptr},
            generator{nullptr}
        {}

        constexpr Link (Link&& other) noexcept : // move construct
            type{other.type},
            flags{other.flags},
            spare{other.spare},
            own{other.own},
            view{own ? View{own->data(), other.view.len} : std::move(other.view)},
            dynamic{other.dynamic},
            generator{other.generator}
        {
            other.own = nullptr;
            other.dynamic = nullptr;
            other.generator = nullptr;
        }
        constexpr Link& operator= (Link&& other) noexcept { // move assign
            this->type = other.type;
            this->flags = other.flags;
            this->own = other.own;
            this->spare = other.spare;
            other.own = nullptr;
            this->view = own ? View{own->data(), other.view.len} : std::move(other.view),
            this->dynamic = other.dynamic;
            other.dynamic = nullptr;
            this->generator = other.generator;
            other.generator = nullptr;
            return *this;
        }

        // Links are never copied, however copy constructors are implemented as
        // quasi-move constructors to allow them to be "moved" out of initializer_lists:
        constexpr Link (const Link& other) noexcept :
            type{other.type},
            flags{other.flags},
            spare{other.spare},
            own{other.own},
            view{own ? View{own->data(), other.view.len} : std::move(other.view)},
            dynamic{other.dynamic},
            generator{other.generator}
        {
            other.own = nullptr;
            other.dynamic = nullptr;
            other.generator = nullptr;
        }
        constexpr Link& operator= (const Link& other) noexcept {
            this->type = other.type;
            this->flags = other.flags;
            this->spare = other.spare;
            this->own = other.own;
            other.own = nullptr;
            this->view = own ? View{own->data(), other.view.len} : std::move(other.view),
            this->dynamic = other.dynamic;
            other.dynamic = nullptr;
            this->generator = std::move(other.generator);
            other.generator = nullptr;
            return *this;
        }

        constexpr Link (const Type tType, uint8_t tFlags = 0) noexcept :
            type{tType},
            flags{tFlags},
            spare{0},
            own{nullptr},
            view{},
            dynamic{nullptr},
            generator{nullptr}
        {}
        constexpr Link (const char* const& text, Type tType = Type::TEXT, uint8_t tFlags = 0) noexcept :
            type{tType},
            flags{tFlags},
            spare{0},
            own{nullptr},
            view{text, std::char_traits<char>::length(text)},
            dynamic{nullptr},
            generator{nullptr}
        {}
        constexpr Link (const std::string_view text, Type tType = Type::TEXT, uint8_t tFlags = 0) noexcept :
            type{tType},
            flags{tFlags},
            spare{0},
            own{nullptr},
            view{text.data(), text.size()},
            dynamic{nullptr},
            generator{nullptr}
        {}
        Link (const std::string text, Type tType = Type::TEXT, uint8_t tFlags = 0) noexcept :
            type{tType},
            flags{tFlags},
            spare{0},
            own{new std::string(std::move(text))},
            view{own->data(), own->size()},
            dynamic{nullptr},
            generator{nullptr}
        {}

        explicit Link (std::vector<Link>&& links) noexcept :
            type{Type::BRANCH_DYNAMIC},
            flags{Flag::insideTag | Flag::invisible},
            spare{0},
            own{nullptr},
            view{},
            dynamic{new std::vector<Link>(std::move(links))},
            generator{nullptr}
        {}
        template<size_t N>
        explicit Link (std::array<Link,N>&& links) noexcept :
            type{Type::BRANCH_DYNAMIC},
            flags{Flag::insideTag | Flag::invisible},
            spare{0},
            own{nullptr},
            view{},
            dynamic{new std::vector<Link>(
                std::make_move_iterator(std::begin(links)),
                std::make_move_iterator(std::end(links))
            )},
            generator{nullptr}
        {}
        template<typename T>
        explicit Link (std::function<T()>&& generator) noexcept :
            type{Type::BRANCH_LAZY},
            flags{Flag::insideTag | Flag::invisible},
            spare{0},
            own{nullptr},
            view{},
            dynamic{nullptr},
            generator{new GeneratorFn(std::move(generator))}
        {}

        constexpr inline Link&& setFlags (uint8_t tFlags) noexcept {
            this->flags |= tFlags;
            return std::move(*this);
        }

        constexpr inline Link&& setSpare (uint32_t tSpare) noexcept {
            this->spare = tSpare;
            return std::move(*this);
        }

        constexpr inline Link&& setType (Type tType) noexcept {
            this->type = tType;
            return std::move(*this);
        }

        inline ~Link () noexcept {
            delete this->own;
            delete this->dynamic;
            delete this->generator;
        }
    };

    template<typename T>
    void setFlagsRecursive (T& tape, uint8_t flags) {
        for (Link& link : tape) {
            link.flags |= flags;
            if (link.dynamic) {
                setFlagsRecursive(*(link.dynamic), flags);
            }
        }
    }

    // Tape:

    template<size_t N>
    struct Tape : public std::array<Link,N> {
        typedef std::integral_constant<std::size_t, N> size;

        constexpr inline Tape&& setFlags (uint8_t tFlags) noexcept {
            setFlagsRecursive(*this, tFlags);
            return std::move(*this);
        }

        constexpr inline Tape&& setType (Link::Type type) noexcept {
            for (Link& link : *this) {
                link.type = type;
            }
            return std::move(*this);
        }
    };

    typedef std::vector<Link> FlexiTape;

    template<std::size_t N, std::size_t... I>
    constexpr Tape<N> fromArrayImpl (Link (&&a)[N], std::index_sequence<I...>) {
        return {{ std::move(a[I])... }};
    }

    template<size_t N>
    constexpr Tape<N> fromArray (Link (&&tLinks)[N]) {
        return fromArrayImpl(std::move(tLinks), std::make_index_sequence<N>{});
    }

    template<size_t NT, size_t NS>
    constexpr void moveOrCopyTo (long& offset, Tape<NT>& target, Tape<NS>&& source) noexcept {
        std::move(
            std::make_move_iterator(source.begin()),
            std::make_move_iterator(source.end()),
            target.begin() + offset
        );
        offset += NS;
    }

    template<size_t NT, size_t NS>
    constexpr void moveOrCopyTo (long& offset, Tape<NT>& target, const Tape<NS>& source) noexcept {
        std::copy(
            source.begin(),
            source.end(),
            target.begin() + offset
        );
        offset += NS;
    }

    template<typename T, size_t NS>
    constexpr void moveOrCopyTo (long& offset, std::vector<T>& target, Tape<NS>&& source) noexcept {
        target.insert(
            target.begin() + offset,
            std::make_move_iterator(source.begin()),
            std::make_move_iterator(source.end())
        );
        offset += NS;
    }

    template<typename T, size_t NS>
    constexpr void moveOrCopyTo (long& offset, std::vector<T>& target, const Tape<NS>& source) noexcept {
        target.insert(
            target.begin() + offset,
            source.begin(),
            source.end()
        );
        offset += NS;
    }

    constexpr inline Tape<0> flatten () noexcept {
        return {};
    }

    inline Tape<1> flatten (const char* text) noexcept {
        return {Link(text)};
    }

    inline Tape<1> flatten (const std::string_view text) noexcept {
        return {Link(text)};
    }

    inline Tape<1> flatten (const std::string&& text) noexcept {
        return {Link(std::move(text))};
    }

    inline Tape<1> flatten (const std::string& text) noexcept {
        return {Link(text)};
    }

    template<typename T>
    inline Tape<1> flatten (std::function<T()>&& lazy) noexcept {
        return {Link(std::move(lazy))};
    }

    inline Tape<1> flatten (Link&& link) noexcept {
        return Tape<1>{std::move(link)};
    }

    inline Tape<1> flatten (const Link& link) noexcept {
        return Tape<1>{link};
    }

    template<size_t N>
    constexpr Tape<N> flatten (Tape<N>&& tape) noexcept {
        return std::move(tape);
    }

    template<size_t N>
    constexpr const Tape<N>& flatten (const Tape<N>& tape) noexcept {
        return tape;
    }

    template<class T>
    Tape<1> flatten (const std::vector<T>&& tapesOrLinks) noexcept {
        FlexiTape flattened;
        flattened.reserve(linkCount<T>*tapesOrLinks.size());
        long offset = 0;
        for (T& tapeOrLink : tapesOrLinks) {
            moveOrCopyTo(offset, flattened, flatten(std::move(tapeOrLink)));
        }
        return Tape<1>{Link(std::move(flattened))};
    }

    template<class T>
    Tape<1> flatten (const std::initializer_list<T> tapesOrLinks) noexcept {
        FlexiTape flattened;
        flattened.reserve(linkCount<T>*tapesOrLinks.size());
        long offset = 0;
        for (T tapeOrLink : tapesOrLinks) {
            moveOrCopyTo(offset, flattened, flatten(std::move(tapeOrLink)));
        }
        return Tape<1>{Link(std::move(flattened))};
    }

    template<typename T, size_t N>
    constexpr Tape<linkCount<T>*N> flatten (T (&& tapesOrLinks)[N]) noexcept {
        Tape<linkCount<T>*N> flattened;
        long offset = 0;
        for (T& tapeOrLink : tapesOrLinks) {
            moveOrCopyTo(offset, flattened, flatten(std::move(tapeOrLink)));
        }
        return flattened;
    }

    template<typename T, size_t N>
    constexpr Tape<linkCount<T>*N> flatten (std::array<T,N>&& tapesOrLinks) noexcept {
        Tape<linkCount<T>*N> flattened;
        long offset = 0;
        for (T& tapeOrLink : tapesOrLinks) {
            moveOrCopyTo(offset, flattened, flatten(std::move(tapeOrLink)));
        }
        return flattened;
    }

    template<class ...Ts>
    constexpr Tape<linkCount<Ts...>> flattenMany (Ts&&... tapesOrLinks) noexcept {
        Tape<linkCount<Ts...>> flattened;
        long offset = 0;
        (moveOrCopyTo(offset, flattened, flatten(std::forward<Ts>(tapesOrLinks))), ...);
        return flattened;
    }

    struct TapeFragment : public Tape<1> {
        TapeFragment () :
            Tape<1>{Link(FlexiTape{})}
        {}
        TapeFragment (Link link) :
            Tape<1>{std::move(link)}
        {}
        TapeFragment (FlexiTape tape) :
            Tape<1>{Link(std::move(tape))}
        {}
        template<size_t N>
        TapeFragment (Tape<N> tape) :
            Tape<1>{Link(std::move(tape))}
        {}
        template<class ...T>
        TapeFragment (T&&... children) :
            Tape<1>{Link(flattenMany(std::forward<T>(children)...))}
        {}

        void push_back (Link&& link) {
            this->at(0).dynamic->push_back(std::move(link));
        }

        void push_back (FlexiTape&& tape) {
            auto& dest = this->at(0).dynamic;
            dest->insert(
                dest->end(),
                std::make_move_iterator(tape.begin()),
                std::make_move_iterator(tape.end())
            );
        }

        template<size_t N>
        void push_back (Tape<N>&& tape) {
            auto& dest = this->at(0).dynamic;
            dest->insert(
                dest->end(),
                std::make_move_iterator(tape.begin()),
                std::make_move_iterator(tape.end())
            );
        }
    };

    namespace exports {
        using fragment = TapeFragment;
        using lazy = std::function<fragment()>;
    }
}}


////|              |////
////| Placeholders |////
////|              |////


namespace Webxx { namespace internal {
    struct Placeholder : Link {
        template<class T>
        constexpr Placeholder(T&& value) noexcept :
            Link(std::forward<T>(value), Type::TEXT, Link::Flag::placeholder)
        {}
    };

    namespace exports {
        using _ = Placeholder;
    }
}}


////|       |////
////|  CSS  |////
////|       |////


namespace Webxx { namespace internal {

    static constexpr const size_t linksCssFragment = 3;
    struct CssFragment : public Tape<linksCssFragment> {
        struct Declaration{};
        struct AtRuleSingle{};
        struct AtRuleNested{};

        // For rule sets:
        template<class ...T>
        CssFragment (Link selector, T&&... declarations) noexcept : Tape<linksCssFragment>{
            Link(Link::Type::CSS_RULE_START),
            std::move(selector).setType(Link::Type::CSS_SELECTOR),
            Link(flattenMany(
                Link(Link::Type::CSS_BLOCK_START),
                std::forward<T>(declarations)...,
                Link(Link::Type::CSS_BLOCK_END)
            )),
        } {}
        template<size_t N, class ...T>
        constexpr CssFragment (Link (&&selectors)[N], T&&... declarations) noexcept : Tape<linksCssFragment>{
            Link(Link::Type::CSS_RULE_START),
            Link(fromArray(std::move(selectors)).setType(Link::Type::CSS_SELECTOR)),
            Link(flattenMany(
                Link(Link::Type::CSS_BLOCK_START),
                std::forward<T>(declarations)...,
                Link(Link::Type::CSS_BLOCK_END)
            )),
        } {}
        // For declarations:
        constexpr explicit CssFragment (Declaration, const char* property, Link&& value) noexcept : Tape<linksCssFragment>{
            Link(std::move(property), Link::Type::CSS_DECLARATION_PROPERTY),
            value.setType(Link::Type::CSS_DECLARATION_VALUE),
            Link(),
        } {}
        // For "at" rules with single value:
        constexpr explicit CssFragment (AtRuleSingle, const char* label, Link&& value) noexcept : Tape<linksCssFragment>{
            Link(label, Link::Type::CSS_AT_RULE_START),
            value.setType(Link::Type::CSS_AT_RULE_VALUE),
            Link(),
        } {}
        // For "at" rules with nested contents:
        template<size_t N>
        constexpr explicit CssFragment (AtRuleNested, const char* label, Link&& selector, CssFragment (&&children)[N]) noexcept : Tape<linksCssFragment>{
            Link(label, Link::Type::CSS_AT_RULE_START),
            selector.setType(Link::Type::CSS_AT_RULE_SELECTOR),
            Link(flattenMany(
                Link(Link::Type::CSS_BLOCK_START),
                std::move(children),
                Link(Link::Type::CSS_BLOCK_END)
            )),
        } {}
    };

    struct CssDeclaration : public CssFragment {
        constexpr CssDeclaration (const char* property, Link&& value) noexcept :
            CssFragment(CssFragment::Declaration{}, property, std::move(value))
        {}
    };

    template<const char* LABEL>
    struct CssDeclarationLabelled : public CssDeclaration {
        constexpr CssDeclarationLabelled (Link&& value) noexcept :
            CssDeclaration(LABEL, std::move(value))
        {}
    };

    template<const char* LABEL>
    struct CssAtRuleSingleLabelled : public CssFragment {
        constexpr CssAtRuleSingleLabelled (Link&& value) noexcept :
            CssFragment(CssFragment::AtRuleSingle{}, LABEL, std::move(value))
        {}
    };

    template<const char* LABEL>
    struct CssAtRuleNestedLabelled : public CssFragment {
        template<size_t N>
        constexpr CssAtRuleNestedLabelled (Link&& selector, CssFragment (&&children)[N]) noexcept :
            CssFragment(CssFragment::AtRuleNested{}, LABEL, std::move(selector), std::move(children))
        {}
    };

    namespace exports {
        using prop = CssDeclaration;
        template<const char* PROP>
        using property = CssDeclarationLabelled<PROP>;
        using rule = CssFragment;
        using styles = std::initializer_list<CssFragment>;
        using declarations = std::initializer_list<CssDeclaration>;
    }
}}


////|        |////
////|  HTML  |////
////|        |////


namespace Webxx { namespace internal {

    template<class ...T>
    struct HtmlAttribute : public Tape<linkCount<T...>+2> {
        template<class ...T2>
        HtmlAttribute (const char* tag, T2&&... children) noexcept : Tape<linkCount<T...>+2>{
            Link(tag, Link::Type::HTML_ATTR_KEY, Link::Flag::insideTag),
            Link(std::forward<T2>(children)).setType(Link::Type::HTML_ATTR_VALUE).setFlags(Link::Flag::insideTag)...,
            Link(tag, Link::Type::HTML_ATTR_END, Link::Flag::insideTag)
        } {}
    };

    template<class ...T>
    struct HtmlNode : public Tape<linkCount<T...>+3> {
        template<class ...A, class ...T2>
        HtmlNode (const char* tag, TapeFragment&& attributes, T2&&... children) noexcept : Tape<linkCount<T...>+3>{flattenMany(
            Link(tag, Link::Type::HTML_TAG_START),
            Link(std::move(attributes)),
            flattenMany(std::forward<T2>(children)...),
            Link(tag, Link::Type::HTML_TAG_END)
        )} {}
        template<class ...T2>
        HtmlNode (const char* tag, T2&&... children) noexcept : Tape<linkCount<T...>+3>{flattenMany(
            Link(tag, Link::Type::HTML_TAG_START),
            Link(),
            flattenMany(std::forward<T2>(children)...),
            Link(tag, Link::Type::HTML_TAG_END)
        )} {}
    };

    template<class ...T>
    struct HtmlNodeSelfClosing : public Tape<linkCount<T...>+2> {
        template<class ...A, class ...T2>
        HtmlNodeSelfClosing (const char* tag, TapeFragment&& attributes, T2&&... children) noexcept : Tape<linkCount<T...>+2>{flattenMany(
            Link(tag, Link::Type::HTML_TAG_START),
            Link(std::move(attributes)),
            flattenMany(std::forward<T2>(children)...)
        )} {}
        template<class ...T2>
        HtmlNodeSelfClosing (const char* tag, T2&&... children) noexcept : Tape<linkCount<T...>+2>{flattenMany(
            Link(tag, Link::Type::HTML_TAG_START_SELF_CLOSE),
            Link(),
            flattenMany(std::forward<T2>(children)...)
        )} {}
    };

    template<class ...T>
    struct HtmlNodeNoClosing : public Tape<linkCount<T...>+2> {
        template<class ...A, class ...T2>
        HtmlNodeNoClosing (const char* tag, TapeFragment&& attributes, T2&&... children) noexcept : Tape<linkCount<T...>+2>{flattenMany(
            Link(tag, Link::Type::HTML_TAG_START),
            Link(std::move(attributes)),
            flattenMany(std::forward<T2>(children)...),
            Link(tag, Link::Type::HTML_TAG_END)
        )} {}
        template<class ...T2>
        HtmlNodeNoClosing (const char* tag, T2&&... children) noexcept : Tape<linkCount<T...>+2>{flattenMany(
            Link(tag, Link::Type::HTML_TAG_START),
            Link(),
            flattenMany(std::forward<T2>(children)...)
        )} {}
    };

    namespace exports {
        // HTML extensibility:
        // template<const char* TAG>
        // using el = HtmlNodeDefined<TAG>;
        // template<const char* NAME>
        // using attr = HtmlAttrDef<NAME>;

        // using node = HtmlNode;
        // using nodes = std::vector<HtmlNode>;
        // using children = std::initializer_list<HtmlNode>;
        using node = TapeFragment;
        using nodes = TapeFragment;
        using children = std::initializer_list<TapeFragment>;
        using attrs = TapeFragment;

        // HTML special purpose nodes:
        using text = Link;

        struct style : public Tape<3> {
            style (std::initializer_list<CssFragment> declarations) : Tape<3>{
                Link(styleTag, Link::Type::HTML_TAG_START),
                Link(flatten(std::move(declarations))),
                Link(styleTag, Link::Type::HTML_TAG_END)
            } {}
            template<class ...Ts>
            style (Ts... declarations) : Tape<3>{
                Link(styleTag, Link::Type::HTML_TAG_START),
                Link(flattenMany(std::forward<Ts>(declarations)...)),
                Link(styleTag, Link::Type::HTML_TAG_END)
            } {}
        };
    }
}}


////|             |////
////|  Component  |////
////|             |////


namespace Webxx { namespace internal {
    #ifdef _MSC_VER
    #define WEBXX_FN_SIG __FUNCSIG__
    #else
    #define WEBXX_FN_SIG __PRETTY_FUNCTION__
    #endif

    typedef uint32_t ComHash;
    template <typename T>
    constexpr ComHash constHash () {
        ComHash hash{0};
        for (const char& c : WEBXX_FN_SIG) {
            (hash ^= static_cast<ComHash>(c)) <<= 1;
        }
        return hash;
    }

    // Adapted from https://stackoverflow.com/a/74838061
    constexpr const size_t ComIdLen = 6;
    typedef std::array<char,ComIdLen+1> ComId;
    constexpr static const char maps[64] = "0123456789abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ_";
    constexpr ComId constStringHash (ComHash hash) {
        ComId buffer{"000000"};
        char* c = buffer.end() - 1;
        do {
            *--c = maps[hash & 0b111111];
            hash >>= 6;
        } while (hash > 0);
        return buffer;
    }

    template <typename T>
    constexpr ComHash compileTimeTypeId = constHash<T>();

    template<ComHash K>
    struct ComponentNode : public Tape<3> { // @todo Perhaps could be compile-time width?
        typedef std::integral_constant<ComHash, K> hash;
        constexpr static const ComId id{constStringHash(K)};

        template<size_t N>
        ComponentNode (Tape<N>&& children) : Tape<3>{
            Link(id.begin(), Link::Type::COM_START).setSpare(hash::value),
            Link(std::move(children)),
            Link(Link::Type::COM_END).setSpare(hash::value),
        } {}
    };

    template<typename C>
    struct Component : ComponentNode<compileTimeTypeId<C>> {
        template<size_t NR>
        Component (
            Tape<NR> rootNode
        ) : ComponentNode<compileTimeTypeId<C>>(flattenMany(
            std::move(rootNode)
        )) {}
        template<size_t NR>
        Component (
            Empty,
            Tape<NR> rootNode
        ) : ComponentNode<compileTimeTypeId<C>>(flattenMany(
            std::move(rootNode)
        )) {}
        template<size_t NR, size_t NC>
        Component (
            CssFragment (&&stylesNode)[NC],
            Tape<NR> rootNode
        ) : ComponentNode<compileTimeTypeId<C>>(flattenMany(
            flatten(std::move(stylesNode)).setFlags(Link::Flag::passComCss),
            std::move(rootNode)
        )) {}
        template<size_t NR>
        Component (
            std::initializer_list<CssFragment> stylesNode,
            Tape<NR> rootNode
        ) : ComponentNode<compileTimeTypeId<C>>(flattenMany(
            flatten(std::move(stylesNode)).setFlags(Link::Flag::passComCss),
            std::move(rootNode)
        )) {}
        template<size_t NR, typename ...Ts>
        Component (
            Empty,
            Tape<NR> rootNode,
            TapeFragment headNodes
        ) : ComponentNode<compileTimeTypeId<C>>(flattenMany(
            std::move(rootNode),
            std::move(headNodes).setFlags(Link::Flag::passComHead)
        )) {}
        template<size_t NR, size_t NC, typename ...Ts>
        Component (
            CssFragment (&&stylesNode)[NC],
            Tape<NR> rootNode,
            TapeFragment headNodes
        ) : ComponentNode<compileTimeTypeId<C>>(flattenMany(
            flatten(std::move(stylesNode)).setFlags(Link::Flag::passComCss),
            std::move(rootNode),
            std::move(headNodes).setFlags(Link::Flag::passComHead)
        )) {}
        template<size_t NR>
        Component (
            std::initializer_list<CssFragment> stylesNode,
            Tape<NR> rootNode,
            TapeFragment headNodes
        ) : ComponentNode<compileTimeTypeId<C>>(flattenMany(
            flatten(std::move(stylesNode)).setFlags(Link::Flag::passComCss),
            std::move(rootNode),
            std::move(headNodes).setFlags(Link::Flag::passComHead)
        )) {}
    };

    struct ComCssTarget : public Tape<3> {
        ComCssTarget () : Tape<3>{
            Link(styleTag, Link::Type::HTML_TAG_START),
            Link(Link::Type::BRANCH_COM_CSS),
            Link(styleTag, Link::Type::HTML_TAG_END),
        } {}
    };

    struct ComHtmlTarget : public Tape<1> {
        ComHtmlTarget () : Tape<1>{
            Link(Link::Type::BRANCH_COM_HEAD),
        } {}
    };

    static constexpr const size_t comAttrLen = 9 + ComIdLen + 2;
    inline constexpr std::array<char,comAttrLen> makeComponentTypeIdAttribute (const char* comId) noexcept {
        std::array<char,comAttrLen> out{" lang=\"x-000000\""};
        std::copy(comId, comId + ComIdLen, out.begin() + 9);
        return out;
    }

    static constexpr const size_t comSelectorLen = 8 + ComIdLen + 2;
    inline constexpr std::array<char,comSelectorLen> makeComponentTypeIdSelector (const char* comId) noexcept {
        std::array<char,comSelectorLen> out{":lang(x-000000)"};
        std::copy(comId, comId + ComIdLen, out.begin() + 8);
        return out;
    }

    namespace exports {
        template <class T>
        using component = Component<T>;
        using styleTarget = ComCssTarget;
        using headTarget = ComHtmlTarget;
    }
}}


////|             |////
////|  Rendering  |////
////|             |////


namespace Webxx { namespace internal {
    static const Link nullLink{};

    inline const std::string_view noopPopulator (
        const std::string_view& value,
        const std::string_view&
    ) {
        return value;
    }

    inline void appendToInternalBuffer (
        const std::string_view& data,
        std::string& buffer
    ) {
        buffer.append(data);
    }

    struct RenderMachine {
        enum class Pass : uint8_t {
            MAIN     = 0b001,
            COM_CSS  = 0b010,
            COM_HEAD = 0b100,
        };

        struct Options {
            static constexpr const std::size_t renderBufferDefaultSize{4 * 1024};

            typedef std::function<const std::string_view(
                const std::string_view&,
                const std::string_view&
            )> PlaceholderPopulator;
            typedef std::function<void(
                const std::string_view&,
                std::string&
            )> RenderReceiverFn;

            PlaceholderPopulator placeholderPopulator{noopPopulator};
            RenderReceiverFn renderReceiverFn{appendToInternalBuffer};
            std::size_t renderBufferSize{renderBufferDefaultSize};
            Pass pass{Pass::MAIN};

            Options() :
                placeholderPopulator{noopPopulator}
            {}
            Options(PlaceholderPopulator tPlaceholderPopulator) :
                placeholderPopulator{tPlaceholderPopulator}
            {}
            Options(PlaceholderPopulator tPlaceholderPopulator, RenderReceiverFn tReceiverFn) :
                placeholderPopulator{tPlaceholderPopulator},
                renderReceiverFn{tReceiverFn},
                renderBufferSize(0)
            {}
            Options(PlaceholderPopulator tPlaceholderPopulator, RenderReceiverFn tReceiverFn, std::size_t tRenderBufferSize) :
                placeholderPopulator{tPlaceholderPopulator},
                renderReceiverFn(tReceiverFn),
                renderBufferSize(tRenderBufferSize)
            {}
        };

        struct Token {
            static constexpr const char* htmlAttrAssign = "=\"";
            static constexpr const char* htmlTagCloseEnd = ">";
            static constexpr const char* htmlTagCloseStart = "</";
            static constexpr const char* htmlTagOpenEnd = ">";
            static constexpr const char* htmlTagOpenEndSelfClose = "/>";
            static constexpr const char* htmlTagOpenStart = "<";
            static constexpr const char* braceClose = "}";
            static constexpr const char* braceOpen = "{";
            static constexpr const char* comma = ",";
            static constexpr const char* colon = ":";
            static constexpr const char* quote = "\"";
            static constexpr const char* semicolon = ";";
            static constexpr const char* space = " ";
        };

        // Input:
        const Link* tapeBegin;
        const Link* tapeEnd;
        Options options;

        // Output:
        std::string buffer;

        // Internal state:
        Pass pass{Pass::MAIN};
        const Link* prev{&nullLink};
        std::deque<const Link*> componentStack{};
        std::unordered_set<ComHash> seenComponentHashes{};
        bool isComponentRootElement{false};
        bool hasHtmlTagOpen{false};
        bool isHtmlTagSelfClosing{false};

        // Emitted pass/flag combinations:
        static constexpr const uint8_t cPassMFlagN =
            static_cast<uint8_t>(Pass::MAIN);
        static constexpr const uint8_t cPassCFlagC =
            static_cast<uint8_t>(Pass::COM_CSS) | Link::Flag::passComCss;
        static constexpr const uint8_t cPassHFlagH =
            static_cast<uint8_t>(Pass::COM_HEAD) | Link::Flag::passComHead;

        // Entrypoint:

        template<typename C>
        RenderMachine (
            const C& tape,
            Options&& tOptions
        ) :
            tapeBegin{&*tape.begin()},
            tapeEnd{&*tape.end()},
            options{std::move(tOptions)},
            pass{options.pass}
        {
            this->buffer.reserve(this->options.renderBufferSize);
            this->renderPass(this->tapeBegin, this->tapeEnd);
        }

        // Helpers:

        inline constexpr bool shouldEmit (uint8_t flags) {
            switch(static_cast<uint8_t>(this->pass) | (flags & Link::Flag::maskPass)) {
                case cPassMFlagN: {
                    return true;
                };
                case cPassCFlagC:
                case cPassHFlagH: {
                    return this->seenComponentHashes.count(this->componentStack.front()->spare) == 0;
                }
                default: {
                    // Not a combination of pass and flags that should be emitted.
                    return false;
                }
            }
        }

        template<typename ...Ts>
        inline constexpr void emit (uint8_t flags, const Ts&... views) {
            if (shouldEmit(flags)) {
                if (flags & Link::Flag::placeholder) {
                    (options.renderReceiverFn(options.placeholderPopulator(views, none), this->buffer),...); // @todo Provide context.
                } else {
                    (options.renderReceiverFn(views, this->buffer),...);
                }
            }
        }

        // Force close of any open HTML tag:
        inline void maybeEndHtmlOpenTag (uint8_t flags = 0) {
            flags &= Link::Flag::maskPass;
            if (this->hasHtmlTagOpen) {
                this->emit(flags, this->isHtmlTagSelfClosing ? Token::htmlTagOpenEndSelfClose : Token::htmlTagOpenEnd);
                this->hasHtmlTagOpen = false;
                this->isHtmlTagSelfClosing = false;
            }
        }

        // Main loop:
        // template<typename T>
        inline void renderPass (const Link* passBegin, const Link* passEnd) {
            const Link* curr = passBegin;
            while (curr != passEnd) {
                const Link& link = *curr;

                // Automatically close any dangling open HTML tags if present:
                if (!(link.flags & Link::Flag::insideTag)) {
                    this->maybeEndHtmlOpenTag(link.flags);
                }

                switch (link.type) {
                    // Branches into other passes:
                    case Link::Type::BRANCH_DYNAMIC: {
                        // Enter into nested tape:
                        this->renderPass(&*(link.dynamic->begin()), &*(link.dynamic->end()));
                    } break;
                    case Link::Type::BRANCH_LAZY: {
                        // Enter into tape generated at render time:
                        auto generated = (*(link.generator))();
                        this->renderPass(&*(generated.begin()), &*(generated.end()));
                    } break;
                    case Link::Type::BRANCH_COM_CSS: {
                        // Begin a pass to gather up component CSS (only from main pass):
                        if (this->pass == Pass::MAIN) {
                            this->pass = Pass::COM_CSS;
                            this->seenComponentHashes.clear();
                            this->renderPass(this->tapeBegin, this->tapeEnd);
                            this->pass = Pass::MAIN;
                        }
                    } break;
                    case Link::Type::BRANCH_COM_HEAD: {
                        // Begin a pass to gather up component <head> HTML (only from main pass):
                        if (this->pass == Pass::MAIN) {
                            this->pass = Pass::COM_HEAD;
                            this->seenComponentHashes.clear();
                            this->renderPass(this->tapeBegin, this->tapeEnd);
                            this->pass = Pass::MAIN;
                        }
                    } break;

                    // Components:
                    case Link::Type::COM_END: {
                        this->componentStack.pop_front();
                        this->seenComponentHashes.insert(link.spare);
                    } break;
                    case Link::Type::COM_START: {
                        this->componentStack.push_front(&link);
                        this->isComponentRootElement = true;
                    } break;

                    // CSS:
                    case Link::Type::CSS_AT_RULE_START: {
                        this->emit(link.flags, link.view);
                    } break;
                    case Link::Type::CSS_AT_RULE_SELECTOR: {
                        this->emit(link.flags, Token::space, link.view);
                    } break;
                    case Link::Type::CSS_AT_RULE_VALUE: {
                        this->emit(link.flags, Token::space, link.view, Token::semicolon);
                    } break;
                    case Link::Type::CSS_BLOCK_END: {
                        this->emit(link.flags, Token::braceClose);
                    } break;
                    case Link::Type::CSS_BLOCK_START: {
                        this->emit(link.flags, Token::braceOpen);
                    } break;
                    case Link::Type::CSS_DECLARATION_PROPERTY: {
                        this->emit(link.flags, link.view, Token::colon);
                    } break;
                    case Link::Type::CSS_DECLARATION_VALUE: {
                        this->emit(link.flags, link.view, Token::semicolon);
                    } break;
                    case Link::Type::CSS_SELECTOR: {
                        if (prev->type == Link::Type::CSS_SELECTOR) {
                            this->emit(link.flags,  Token::comma);
                        }
                        if (link.flags & Link::Flag::passComCss) {
                            auto selector = makeComponentTypeIdSelector(this->componentStack.front()->view.ptr);
                            this->emit(link.flags, link.view, std::string_view(selector.begin(), comSelectorLen - 1));
                        } else {
                            this->emit(link.flags, link.view);
                        }
                    } break;

                    // HTML:
                    case Link::Type::HTML_ATTR_KEY: {
                        this->emit(link.flags, prev->type == Link::Type::NONE ? none : Token::space, link.view);
                    } break;
                    case Link::Type::HTML_ATTR_VALUE: {
                        this->emit(link.flags, prev->type == Link::Type::HTML_ATTR_KEY ? Token::htmlAttrAssign : Token::space, link.view);
                    } break;
                    case Link::Type::HTML_ATTR_END: {
                        this->emit(link.flags, prev->type == Link::Type::HTML_ATTR_KEY ? none : Token::quote);
                    } break;
                    case Link::Type::HTML_TAG_START_SELF_CLOSE: {
                        this->isHtmlTagSelfClosing = true;
                    };  // Intentional fall through.
                    case Link::Type::HTML_TAG_START: {
                        this->emit(link.flags, Token::htmlTagOpenStart, link.view);
                        if (this->isComponentRootElement) {
                            auto attr = makeComponentTypeIdAttribute(this->componentStack.front()->view.ptr);
                            this->emit(link.flags, std::string_view(attr.begin(), comAttrLen - 1));
                            this->isComponentRootElement = false;
                        }
                        this->hasHtmlTagOpen = true;
                    } break;
                    case Link::Type::HTML_TAG_END: {
                        this->emit(link.flags, Token::htmlTagCloseStart, link.view, Token::htmlTagCloseEnd);
                    } break;

                    // Text:
                    case Link::Type::TEXT: {
                        this->emit(link.flags, link.view);
                    } break;

                    // Skip:
                    case Link::Type::NONE:
                    case Link::Type::CSS_RULE_START: {
                    } break;
                }

                if (!(link.flags & Link::Flag::invisible)) {
                    this->prev = &link;
                }

                ++curr;
            }

            this->maybeEndHtmlOpenTag(this->prev->flags);
        }
    };

    namespace exports {
        using PlaceholderPopulator = RenderMachine::Options::PlaceholderPopulator;

        template<size_t T>
        inline std::string render (Tape<T>&& thing, RenderMachine::Options options = {}) {
            return std::move(RenderMachine(thing, std::move(options)).buffer);
        }

        template<size_t T>
        inline std::string render (const Tape<T>& thing, RenderMachine::Options options = {}) {
            return std::move(RenderMachine(thing, std::move(options)).buffer);
        }

        inline std::string render (FlexiTape&& thing, RenderMachine::Options options = {}) {
            return std::move(RenderMachine(thing, std::move(options)).buffer);
        }

        inline std::string render (const FlexiTape& thing, RenderMachine::Options options = {}) {
            return std::move(RenderMachine(thing, std::move(options)).buffer);
        }

        template<typename T>
        inline std::string render (std::initializer_list<T> thing, RenderMachine::Options options = {}) {
            return render(flatten(std::move(thing)), std::move(options));
        }

        template<typename T>
        inline std::string renderCss (T&& thing, RenderMachine::Options options = {}) {
            options.pass = RenderMachine::Pass::COM_CSS;
            return render(std::forward<T>(thing), std::move(options));
        }
    }
}}


////|           |////
////|  Utility  |////
////|           |////


namespace Webxx { namespace internal {

    namespace exports {

        template<class T, typename F>
        fragment each (const T& items, F&& cb) {
            FlexiTape tNodes;
            tNodes.reserve(items.size());

            for (const auto& item : items) {
                tNodes.push_back(Link(cb(item)));
            }

            return fragment{std::move(tNodes)};
        }

        template<class T, typename F>
        fragment each (T &&items, F&& cb) {
            FlexiTape tNodes;
            tNodes.reserve(items.size());

            for (auto&& item : items) {
                tNodes.push_back(Link(cb(std::forward<decltype(item)>(item))));
            }

            return fragment{std::move(tNodes)};
        }

        template<typename C, class T>
        fragment each (const T& items) {
            FlexiTape tNodes;
            tNodes.reserve(items.size());

            for (const auto& item : items) {
                tNodes.push_back(Link(C{item}));
            }

            return fragment{std::move(tNodes)};
        }

        template<typename C, class T, typename V = typename std::remove_reference<T>::type::value_type>
        fragment each (T&& items) {
            FlexiTape tNodes;
            tNodes.reserve(items.size());

            for (auto&& item : items) {
                tNodes.push_back(Link(C{std::forward<V>(item)}));
            }

            return fragment{std::move(tNodes)};
        }

        struct Loop {
            size_t index;
            size_t count;
        };

        template<class T, typename F>
        typename std::enable_if_t<has_size_v<T>, Tape<T::size::value>> loop (T&& items, F&& cb) {
            Tape<T::size::value> tNodes;

            Loop loop {0, T::size::value};
            for (size_t i = 0; i < T::size::value; ++i) {
                loop.index = i;
                tNodes[i] = Link(cb(items[i], loop));
            }

            return tNodes;
        }

        template<class T, typename F>
        fragment loop (T&& items, F&& cb) {
            FlexiTape tNodes;
            tNodes.reserve(items.size());

            Loop loop {0, items.size()};
            for (auto&& item : items) {
                tNodes.push_back(Link(cb(std::forward<decltype(item)>(item), loop)));
                ++loop.index;
            }

            return fragment{std::move(tNodes)};
        }

        template<class T, typename F>
        typename std::enable_if_t<has_size_v<T>, Tape<T::size::value>> loop (T& items, F&& cb) {
            Tape<T::size::value> tNodes;

            Loop loop {0, T::size::value};
            for (size_t i = 0; i < T::size::value; ++i) {
                loop.index = i;
                tNodes[i] = Link(cb(items[i], loop));
            }

            return tNodes;
        }

        template<class T, typename F>
        fragment loop (const T& items, F&& cb) {
            FlexiTape tNodes;
            tNodes.reserve(items.size());

            Loop loop {0, items.size()};
            for (const auto& item : items) {
                tNodes.push_back(Link(cb(item, loop)));
                ++loop.index;
            }

            return fragment{std::move(tNodes)};
        }

        template<typename C, class T>
        fragment loop (const T& items) {
            FlexiTape tNodes;
            tNodes.reserve(items.size());

            Loop loop {0, items.size()};
            for (const auto& item : items) {
                tNodes.push_back(Link(C{item, loop}));
                ++loop.index;
            }

            return tNodes;
        }

        template<typename C, class T, typename V = typename std::remove_reference<T>::type::value_type>
        fragment loop (T&& items) {
            FlexiTape tNodes;
            tNodes.reserve(items.size());

            Loop loop {0, items.size()};
            for (auto&& item : items) {
                tNodes.push_back(Link(C{std::forward<V>(item), loop}));
                ++loop.index;
            }
            return tNodes;
        }

        template<typename T, typename F>
        fragment maybe (T&& condition, F&& cb) {
            if (condition) {
                return fragment{cb()};
            }
            return fragment{};
        }

        template<typename T, typename V, typename F>
        fragment maybe (T&& condition, V&& forward, F&& cb) {
            if (condition) {
                return fragment{cb(std::forward<V>(forward))};
            }
            return fragment{};
        }

        template<typename T, typename F>
        fragment maybeAttr (T&& condition, F&& attr) {
            if (condition) {
                return attr;
            }
            return fragment{};
        }
    }
}}


////|          |////
////|  Public  |////
////|          |////


#define WEBXX_CSS_PROP(NAME)\
    namespace internal::res { static constexpr const char NAME ## WXP[] = #NAME; }\
    using NAME = Webxx::internal::CssDeclarationLabelled<internal::res::NAME ## WXP>
#define WEBXX_CSS_PROP_ALIAS(NAME,ALIAS)\
    namespace internal::res { static constexpr const char ALIAS ## WXP[] = #NAME; }\
    using ALIAS = Webxx::internal::CssDeclarationLabelled<internal::res::ALIAS ## WXP>
#define WEBXX_CSS_AT_SINGLE(NAME,ALIAS)\
    namespace internal::res { static constexpr const char ALIAS ## WXR[] = #NAME; }\
    using ALIAS = Webxx::internal::CssAtRuleSingleLabelled<internal::res::ALIAS ## WXR>
#define WEBXX_CSS_AT_NESTED(NAME,ALIAS)\
    namespace internal::res { static constexpr const char ALIAS ## WXR[] = #NAME; }\
    using ALIAS = Webxx::internal::CssAtRuleNestedLabelled<internal::res::ALIAS ## WXR>
#define WEBXX_HTML_EL(TAG)\
    template <class ...T> struct TAG : Webxx::internal::HtmlNode<T...> {\
        template<class ...TC> TAG (TC&&... c) noexcept : Webxx::internal::HtmlNode<TC...>(#TAG, std::forward<TC>(c)...) {}\
        template<class ...TC> TAG (Webxx::internal::TapeFragment&& a, TC&&... c) noexcept : Webxx::internal::HtmlNode<TC...>(#TAG, std::move(a), std::forward<TC>(c)...) {}\
    };\
    template <typename ...T> TAG (T&&...) -> TAG <T...>;\
    template <typename ...T> TAG (Webxx::internal::TapeFragment&&, T&&...) -> TAG <T...>
#define WEBXX_HTML_EL_ALIAS(TAG,ALIAS)\
    template <class ...T> struct ALIAS : Webxx::internal::HtmlNode<T...> {\
        template<class ...TC> ALIAS (TC&&... c) noexcept : Webxx::internal::HtmlNode<TC...>(#TAG, std::forward<TC>(c)...) {}\
        template<class ...TC> ALIAS (Webxx::internal::TapeFragment&& a, TC&&... c) noexcept : Webxx::internal::HtmlNode<TC...>(#TAG, std::move(a), std::forward<TC>(c)...) {}\
    };\
    template <typename ...T> ALIAS (T&&...) -> ALIAS <T...>;\
    template <typename ...T> ALIAS (Webxx::internal::TapeFragment&&, T&&...) -> ALIAS <T...>
#define WEBXX_HTML_EL_SELF_CLOSING(TAG)\
    template <class ...T> struct TAG : Webxx::internal::HtmlNodeSelfClosing<T...> {\
        template<class ...TC> TAG (TC&&... c) noexcept : Webxx::internal::HtmlNodeSelfClosing<TC...>(#TAG, std::forward<TC>(c)...) {}\
        template<class ...TC> TAG (Webxx::internal::TapeFragment&& a, TC&&... c) noexcept : Webxx::internal::HtmlNodeSelfClosing<TC...>(#TAG, std::move(a), std::forward<TC>(c)...) {}\
    };\
    template <typename ...T> TAG (T&&...) -> TAG <T...>;\
    template <typename ...T> TAG (internal::TapeFragment&&, T&&...) -> TAG <T...>
#define WEBXX_HTML_EL_NO_CLOSING_ALIAS(TAG,ALIAS)\
    template <class ...T> struct ALIAS : Webxx::internal::HtmlNodeNoClosing<T...> {\
        template<class ...TC> ALIAS (TC&&... c) noexcept : Webxx::internal::HtmlNodeNoClosing<TC...>(#TAG, std::forward<TC>(c)...) {}\
        template<class ...TC> ALIAS (Webxx::internal::TapeFragment&& a, TC&&... c) noexcept : Webxx::internal::HtmlNodeNoClosing<TC...>(#TAG, std::move(a), std::forward<TC>(c)...) {}\
    };\
    template <typename ...T> ALIAS (T&&...) -> ALIAS <T...>;\
    template <typename ...T> ALIAS (Webxx::internal::TapeFragment&&, T&&...) -> ALIAS <T...>
#define WEBXX_HTML_ATTR(NAME)\
    template <class ...T> struct _ ## NAME : Webxx::internal::HtmlAttribute<T...> {\
        template<class ...TC> _ ## NAME (TC&&... c) noexcept : Webxx::internal::HtmlAttribute<TC...>(#NAME, std::forward<TC>(c)...) {}\
    };\
    template <typename ...T> _ ## NAME (T&&...) -> _ ## NAME <T...>
#define WEBXX_HTML_ATTR_ALIAS(NAME,ALIAS)\
    template <class ...T> struct _ ## ALIAS : Webxx::internal::HtmlAttribute<T...> {\
        template<class ...TC> _ ## ALIAS (TC&&... c) noexcept : Webxx::internal::HtmlAttribute<TC...>(#NAME, std::forward<TC>(c)...) {}\
    };\
    template <typename ...T> _ ## ALIAS (T&&...) -> _ ## ALIAS <T...>


namespace Webxx {
    // Modules:
    using namespace internal::exports;


    // CSS @ rules:
    WEBXX_CSS_AT_SINGLE(@charset,atCharset);
    WEBXX_CSS_AT_SINGLE(@import,atImport);
    WEBXX_CSS_AT_SINGLE(@namespace,atNamespace);
    WEBXX_CSS_AT_NESTED(@media,atMedia);
    WEBXX_CSS_AT_NESTED(@supports,atSupports);
    WEBXX_CSS_AT_NESTED(@document,atDocument);
    WEBXX_CSS_AT_NESTED(@page,atPage);
    WEBXX_CSS_AT_NESTED(@font-face,atFontFace);
    WEBXX_CSS_AT_NESTED(@keyframes,atKeyframes);
    WEBXX_CSS_AT_NESTED(@counter-style,atCounterStyle);
    WEBXX_CSS_AT_NESTED(@font-feature-values,atFontFeatureValues);
    WEBXX_CSS_AT_NESTED(@property,atProperty);
    WEBXX_CSS_AT_NESTED(@layer,atLayer);

    // CSS properties:
    WEBXX_CSS_PROP_ALIAS(accent-color, accentColor);
    WEBXX_CSS_PROP_ALIAS(align-content, alignContent);
    WEBXX_CSS_PROP_ALIAS(align-items, alignItems);
    WEBXX_CSS_PROP_ALIAS(align-self, alignSelf);
    WEBXX_CSS_PROP_ALIAS(alignment-baseline, alignmentBaseline);
    WEBXX_CSS_PROP(all);
    WEBXX_CSS_PROP(animation);
    WEBXX_CSS_PROP_ALIAS(animation-delay, animationDelay);
    WEBXX_CSS_PROP_ALIAS(animation-direction, animationDirection);
    WEBXX_CSS_PROP_ALIAS(animation-duration, animationDuration);
    WEBXX_CSS_PROP_ALIAS(animation-fill-mode, animationFillMode);
    WEBXX_CSS_PROP_ALIAS(animation-iteration-count, animationIterationCount);
    WEBXX_CSS_PROP_ALIAS(animation-name, animationName);
    WEBXX_CSS_PROP_ALIAS(animation-play-state, animationPlayState);
    WEBXX_CSS_PROP_ALIAS(animation-timing-function, animationTimingFunction);
    WEBXX_CSS_PROP(appearance);
    WEBXX_CSS_PROP_ALIAS(aspect-ratio, aspectRatio);
    WEBXX_CSS_PROP(azimuth);
    WEBXX_CSS_PROP_ALIAS(backdrop-filter, backdropFilter);
    WEBXX_CSS_PROP_ALIAS(backface-visibility, backfaceVisibility);
    WEBXX_CSS_PROP(background);
    WEBXX_CSS_PROP_ALIAS(background-attachment, backgroundAttachment);
    WEBXX_CSS_PROP_ALIAS(background-blend-mode, backgroundBlendMode);
    WEBXX_CSS_PROP_ALIAS(background-clip, backgroundClip);
    WEBXX_CSS_PROP_ALIAS(background-color, backgroundColor);
    WEBXX_CSS_PROP_ALIAS(background-image, backgroundImage);
    WEBXX_CSS_PROP_ALIAS(background-origin, backgroundOrigin);
    WEBXX_CSS_PROP_ALIAS(background-position, backgroundPosition);
    WEBXX_CSS_PROP_ALIAS(background-repeat, backgroundRepeat);
    WEBXX_CSS_PROP_ALIAS(background-size, backgroundSize);
    WEBXX_CSS_PROP_ALIAS(baseline-shift, baselineShift);
    WEBXX_CSS_PROP_ALIAS(baseline-source, baselineSource);
    WEBXX_CSS_PROP_ALIAS(block-ellipsis, blockEllipsis);
    WEBXX_CSS_PROP_ALIAS(block-size, blockSize);
    WEBXX_CSS_PROP_ALIAS(bookmark-label, bookmarkLabel);
    WEBXX_CSS_PROP_ALIAS(bookmark-level, bookmarkLevel);
    WEBXX_CSS_PROP_ALIAS(bookmark-state, bookmarkState);
    WEBXX_CSS_PROP(border);
    WEBXX_CSS_PROP_ALIAS(border-block, borderBlock);
    WEBXX_CSS_PROP_ALIAS(border-block-color, borderBlockColor);
    WEBXX_CSS_PROP_ALIAS(border-block-end, borderBlockEnd);
    WEBXX_CSS_PROP_ALIAS(border-block-end-color, borderBlockEndColor);
    WEBXX_CSS_PROP_ALIAS(border-block-end-style, borderBlockEndStyle);
    WEBXX_CSS_PROP_ALIAS(border-block-end-width, borderBlockEndWidth);
    WEBXX_CSS_PROP_ALIAS(border-block-start, borderBlockStart);
    WEBXX_CSS_PROP_ALIAS(border-block-start-color, borderBlockStartColor);
    WEBXX_CSS_PROP_ALIAS(border-block-start-style, borderBlockStartStyle);
    WEBXX_CSS_PROP_ALIAS(border-block-start-width, borderBlockStartWidth);
    WEBXX_CSS_PROP_ALIAS(border-block-style, borderBlockStyle);
    WEBXX_CSS_PROP_ALIAS(border-block-width, borderBlockWidth);
    WEBXX_CSS_PROP_ALIAS(border-bottom, borderBottom);
    WEBXX_CSS_PROP_ALIAS(border-bottom-color, borderBottomColor);
    WEBXX_CSS_PROP_ALIAS(border-bottom-left-radius, borderBottomLeftRadius);
    WEBXX_CSS_PROP_ALIAS(border-bottom-right-radius, borderBottomRightRadius);
    WEBXX_CSS_PROP_ALIAS(border-bottom-style, borderBottomStyle);
    WEBXX_CSS_PROP_ALIAS(border-bottom-width, borderBottomWidth);
    WEBXX_CSS_PROP_ALIAS(border-boundary, borderBoundary);
    WEBXX_CSS_PROP_ALIAS(border-collapse, borderCollapse);
    WEBXX_CSS_PROP_ALIAS(border-color, borderColor);
    WEBXX_CSS_PROP_ALIAS(border-end-end-radius, borderEndEndRadius);
    WEBXX_CSS_PROP_ALIAS(border-end-start-radius, borderEndStartRadius);
    WEBXX_CSS_PROP_ALIAS(border-image, borderImage);
    WEBXX_CSS_PROP_ALIAS(border-image-outset, borderImageOutset);
    WEBXX_CSS_PROP_ALIAS(border-image-repeat, borderImageRepeat);
    WEBXX_CSS_PROP_ALIAS(border-image-slice, borderImageSlice);
    WEBXX_CSS_PROP_ALIAS(border-image-source, borderImageSource);
    WEBXX_CSS_PROP_ALIAS(border-image-width, borderImageWidth);
    WEBXX_CSS_PROP_ALIAS(border-inline, borderInline);
    WEBXX_CSS_PROP_ALIAS(border-inline-color, borderInlineColor);
    WEBXX_CSS_PROP_ALIAS(border-inline-end, borderInlineEnd);
    WEBXX_CSS_PROP_ALIAS(border-inline-end-color, borderInlineEndColor);
    WEBXX_CSS_PROP_ALIAS(border-inline-end-style, borderInlineEndStyle);
    WEBXX_CSS_PROP_ALIAS(border-inline-end-width, borderInlineEndWidth);
    WEBXX_CSS_PROP_ALIAS(border-inline-start, borderInlineStart);
    WEBXX_CSS_PROP_ALIAS(border-inline-start-color, borderInlineStartColor);
    WEBXX_CSS_PROP_ALIAS(border-inline-start-style, borderInlineStartStyle);
    WEBXX_CSS_PROP_ALIAS(border-inline-start-width, borderInlineStartWidth);
    WEBXX_CSS_PROP_ALIAS(border-inline-style, borderInlineStyle);
    WEBXX_CSS_PROP_ALIAS(border-inline-width, borderInlineWidth);
    WEBXX_CSS_PROP_ALIAS(border-left, borderLeft);
    WEBXX_CSS_PROP_ALIAS(border-left-color, borderLeftColor);
    WEBXX_CSS_PROP_ALIAS(border-left-style, borderLeftStyle);
    WEBXX_CSS_PROP_ALIAS(border-left-width, borderLeftWidth);
    WEBXX_CSS_PROP_ALIAS(border-radius, borderRadius);
    WEBXX_CSS_PROP_ALIAS(border-right, borderRight);
    WEBXX_CSS_PROP_ALIAS(border-right-color, borderRightColor);
    WEBXX_CSS_PROP_ALIAS(border-right-style, borderRightStyle);
    WEBXX_CSS_PROP_ALIAS(border-right-width, borderRightWidth);
    WEBXX_CSS_PROP_ALIAS(border-spacing, borderSpacing);
    WEBXX_CSS_PROP_ALIAS(border-start-end-radius, borderStartEndRadius);
    WEBXX_CSS_PROP_ALIAS(border-start-start-radius, borderStartStartRadius);
    WEBXX_CSS_PROP_ALIAS(border-style, borderStyle);
    WEBXX_CSS_PROP_ALIAS(border-top, borderTop);
    WEBXX_CSS_PROP_ALIAS(border-top-color, borderTopColor);
    WEBXX_CSS_PROP_ALIAS(border-top-left-radius, borderTopLeftRadius);
    WEBXX_CSS_PROP_ALIAS(border-top-right-radius, borderTopRightRadius);
    WEBXX_CSS_PROP_ALIAS(border-top-style, borderTopStyle);
    WEBXX_CSS_PROP_ALIAS(border-top-width, borderTopWidth);
    WEBXX_CSS_PROP_ALIAS(border-width, borderWidth);
    WEBXX_CSS_PROP(bottom);
    WEBXX_CSS_PROP_ALIAS(box-decoration-break, boxDecorationBreak);
    WEBXX_CSS_PROP_ALIAS(box-shadow, boxShadow);
    WEBXX_CSS_PROP_ALIAS(box-sizing, boxSizing);
    WEBXX_CSS_PROP_ALIAS(box-snap, boxSnap);
    WEBXX_CSS_PROP_ALIAS(break-after, breakAfter);
    WEBXX_CSS_PROP_ALIAS(break-before, breakBefore);
    WEBXX_CSS_PROP_ALIAS(break-inside, breakInside);
    WEBXX_CSS_PROP_ALIAS(caption-side, captionSide);
    WEBXX_CSS_PROP(caret);
    WEBXX_CSS_PROP_ALIAS(caret-color, caretColor);
    WEBXX_CSS_PROP_ALIAS(caret-shape, caretShape);
    WEBXX_CSS_PROP(chains);
    WEBXX_CSS_PROP(clear);
    WEBXX_CSS_PROP(clip);
    WEBXX_CSS_PROP_ALIAS(clip-path, clipPath);
    WEBXX_CSS_PROP_ALIAS(clip-rule, clipRule);
    WEBXX_CSS_PROP(color);
    WEBXX_CSS_PROP_ALIAS(color-adjust, colorAdjust);
    WEBXX_CSS_PROP_ALIAS(color-interpolation-filters, colorInterpolationFilters);
    WEBXX_CSS_PROP_ALIAS(color-scheme, colorScheme);
    WEBXX_CSS_PROP_ALIAS(column-count, columnCount);
    WEBXX_CSS_PROP_ALIAS(column-fill, columnFill);
    WEBXX_CSS_PROP_ALIAS(column-gap, columnGap);
    WEBXX_CSS_PROP_ALIAS(column-rule, columnRule);
    WEBXX_CSS_PROP_ALIAS(column-rule-color, columnRuleColor);
    WEBXX_CSS_PROP_ALIAS(column-rule-style, columnRuleStyle);
    WEBXX_CSS_PROP_ALIAS(column-rule-width, columnRuleWidth);
    WEBXX_CSS_PROP_ALIAS(column-span, columnSpan);
    WEBXX_CSS_PROP_ALIAS(column-width, columnWidth);
    WEBXX_CSS_PROP(columns);
    WEBXX_CSS_PROP(contain);
    WEBXX_CSS_PROP_ALIAS(contain-intrinsic-block-size, containIntrinsicBlockSize);
    WEBXX_CSS_PROP_ALIAS(contain-intrinsic-height, containIntrinsicHeight);
    WEBXX_CSS_PROP_ALIAS(contain-intrinsic-inline-size, containIntrinsicInlineSize);
    WEBXX_CSS_PROP_ALIAS(contain-intrinsic-size, containIntrinsicSize);
    WEBXX_CSS_PROP_ALIAS(contain-intrinsic-width, containIntrinsicWidth);
    WEBXX_CSS_PROP(container);
    WEBXX_CSS_PROP_ALIAS(container-name, containerName);
    WEBXX_CSS_PROP_ALIAS(container-type, containerType);
    WEBXX_CSS_PROP(content);
    WEBXX_CSS_PROP_ALIAS(content-visibility, contentVisibility);
    WEBXX_CSS_PROP_ALIAS(continue, continue_);
    WEBXX_CSS_PROP_ALIAS(counter-increment, counterIncrement);
    WEBXX_CSS_PROP_ALIAS(counter-reset, counterReset);
    WEBXX_CSS_PROP_ALIAS(counter-set, counterSet);
    WEBXX_CSS_PROP(cue);
    WEBXX_CSS_PROP_ALIAS(cue-after, cueAfter);
    WEBXX_CSS_PROP_ALIAS(cue-before, cueBefore);
    WEBXX_CSS_PROP(cursor);
    WEBXX_CSS_PROP(direction);
    WEBXX_CSS_PROP(display);
    WEBXX_CSS_PROP_ALIAS(dominant-baseline, dominantBaseline);
    WEBXX_CSS_PROP(elevation);
    WEBXX_CSS_PROP_ALIAS(empty-cells, emptyCells);
    WEBXX_CSS_PROP(filter);
    WEBXX_CSS_PROP(flex);
    WEBXX_CSS_PROP_ALIAS(flex-basis, flexBasis);
    WEBXX_CSS_PROP_ALIAS(flex-direction, flexDirection);
    WEBXX_CSS_PROP_ALIAS(flex-flow, flexFlow);
    WEBXX_CSS_PROP_ALIAS(flex-grow, flexGrow);
    WEBXX_CSS_PROP_ALIAS(flex-shrink, flexShrink);
    WEBXX_CSS_PROP_ALIAS(flex-wrap, flexWrap);
    WEBXX_CSS_PROP_ALIAS(float, float_);
    WEBXX_CSS_PROP_ALIAS(flood-color, floodColor);
    WEBXX_CSS_PROP_ALIAS(flood-opacity, floodOpacity);
    WEBXX_CSS_PROP(flow);
    WEBXX_CSS_PROP_ALIAS(flow-from, flowFrom);
    WEBXX_CSS_PROP_ALIAS(flow-into, flowInto);
    WEBXX_CSS_PROP(font);
    WEBXX_CSS_PROP_ALIAS(font-family, fontFamily);
    WEBXX_CSS_PROP_ALIAS(font-feature-settings, fontFeatureSettings);
    WEBXX_CSS_PROP_ALIAS(font-kerning, fontKerning);
    WEBXX_CSS_PROP_ALIAS(font-language-override, fontLanguageOverride);
    WEBXX_CSS_PROP_ALIAS(font-optical-sizing, fontOpticalSizing);
    WEBXX_CSS_PROP_ALIAS(font-palette, fontPalette);
    WEBXX_CSS_PROP_ALIAS(font-size, fontSize);
    WEBXX_CSS_PROP_ALIAS(font-size-adjust, fontSizeAdjust);
    WEBXX_CSS_PROP_ALIAS(font-stretch, fontStretch);
    WEBXX_CSS_PROP_ALIAS(font-style, fontStyle);
    WEBXX_CSS_PROP_ALIAS(font-synthesis, fontSynthesis);
    WEBXX_CSS_PROP_ALIAS(font-synthesis-small-caps, fontSynthesisSmallCaps);
    WEBXX_CSS_PROP_ALIAS(font-synthesis-style, fontSynthesisStyle);
    WEBXX_CSS_PROP_ALIAS(font-synthesis-weight, fontSynthesisWeight);
    WEBXX_CSS_PROP_ALIAS(font-variant, fontVariant);
    WEBXX_CSS_PROP_ALIAS(font-variant-alternates, fontVariantAlternates);
    WEBXX_CSS_PROP_ALIAS(font-variant-caps, fontVariantCaps);
    WEBXX_CSS_PROP_ALIAS(font-variant-east-asian, fontVariantEastAsian);
    WEBXX_CSS_PROP_ALIAS(font-variant-emoji, fontVariantEmoji);
    WEBXX_CSS_PROP_ALIAS(font-variant-ligatures, fontVariantLigatures);
    WEBXX_CSS_PROP_ALIAS(font-variant-numeric, fontVariantNumeric);
    WEBXX_CSS_PROP_ALIAS(font-variant-position, fontVariantPosition);
    WEBXX_CSS_PROP_ALIAS(font-variation-settings, fontVariationSettings);
    WEBXX_CSS_PROP_ALIAS(font-weight, fontWeight);
    WEBXX_CSS_PROP_ALIAS(footnote-display, footnoteDisplay);
    WEBXX_CSS_PROP_ALIAS(footnote-policy, footnotePolicy);
    WEBXX_CSS_PROP_ALIAS(forced-color-adjust, forcedColorAdjust);
    WEBXX_CSS_PROP(gap);
    WEBXX_CSS_PROP_ALIAS(glyph-orientation-vertical, glyphOrientationVertical);
    WEBXX_CSS_PROP(grid);
    WEBXX_CSS_PROP_ALIAS(grid-area, gridArea);
    WEBXX_CSS_PROP_ALIAS(grid-auto-columns, gridAutoColumns);
    WEBXX_CSS_PROP_ALIAS(grid-auto-flow, gridAutoFlow);
    WEBXX_CSS_PROP_ALIAS(grid-auto-rows, gridAutoRows);
    WEBXX_CSS_PROP_ALIAS(grid-column, gridColumn);
    WEBXX_CSS_PROP_ALIAS(grid-column-end, gridColumnEnd);
    WEBXX_CSS_PROP_ALIAS(grid-column-start, gridColumnStart);
    WEBXX_CSS_PROP_ALIAS(grid-row, gridRow);
    WEBXX_CSS_PROP_ALIAS(grid-row-end, gridRowEnd);
    WEBXX_CSS_PROP_ALIAS(grid-row-start, gridRowStart);
    WEBXX_CSS_PROP_ALIAS(grid-template, gridTemplate);
    WEBXX_CSS_PROP_ALIAS(grid-template-areas, gridTemplateAreas);
    WEBXX_CSS_PROP_ALIAS(grid-template-columns, gridTemplateColumns);
    WEBXX_CSS_PROP_ALIAS(grid-template-rows, gridTemplateRows);
    WEBXX_CSS_PROP_ALIAS(hanging-punctuation, hangingPunctuation);
    WEBXX_CSS_PROP(height);
    WEBXX_CSS_PROP_ALIAS(hyphenate-character, hyphenateCharacter);
    WEBXX_CSS_PROP_ALIAS(hyphenate-limit-chars, hyphenateLimitChars);
    WEBXX_CSS_PROP_ALIAS(hyphenate-limit-last, hyphenateLimitLast);
    WEBXX_CSS_PROP_ALIAS(hyphenate-limit-lines, hyphenateLimitLines);
    WEBXX_CSS_PROP_ALIAS(hyphenate-limit-zone, hyphenateLimitZone);
    WEBXX_CSS_PROP(hyphens);
    WEBXX_CSS_PROP_ALIAS(image-orientation, imageOrientation);
    WEBXX_CSS_PROP_ALIAS(image-rendering, imageRendering);
    WEBXX_CSS_PROP_ALIAS(image-resolution, imageResolution);
    WEBXX_CSS_PROP_ALIAS(initial-letter, initialLetter);
    WEBXX_CSS_PROP_ALIAS(initial-letter-align, initialLetterAlign);
    WEBXX_CSS_PROP_ALIAS(initial-letter-wrap, initialLetterWrap);
    WEBXX_CSS_PROP_ALIAS(inline-size, inlineSize);
    WEBXX_CSS_PROP_ALIAS(inline-sizing, inlineSizing);
    WEBXX_CSS_PROP(inset);
    WEBXX_CSS_PROP_ALIAS(inset-block, insetBlock);
    WEBXX_CSS_PROP_ALIAS(inset-block-end, insetBlockEnd);
    WEBXX_CSS_PROP_ALIAS(inset-block-start, insetBlockStart);
    WEBXX_CSS_PROP_ALIAS(inset-inline, insetInline);
    WEBXX_CSS_PROP_ALIAS(inset-inline-end, insetInlineEnd);
    WEBXX_CSS_PROP_ALIAS(inset-inline-start, insetInlineStart);
    WEBXX_CSS_PROP(isolation);
    WEBXX_CSS_PROP_ALIAS(justify-content, justifyContent);
    WEBXX_CSS_PROP_ALIAS(justify-items, justifyItems);
    WEBXX_CSS_PROP_ALIAS(justify-self, justifySelf);
    WEBXX_CSS_PROP_ALIAS(leading-trim, leadingTrim);
    WEBXX_CSS_PROP(left);
    WEBXX_CSS_PROP_ALIAS(letter-spacing, letterSpacing);
    WEBXX_CSS_PROP_ALIAS(lighting-color, lightingColor);
    WEBXX_CSS_PROP_ALIAS(line-break, lineBreak);
    WEBXX_CSS_PROP_ALIAS(line-clamp, lineClamp);
    WEBXX_CSS_PROP_ALIAS(line-grid, lineGrid);
    WEBXX_CSS_PROP_ALIAS(line-height, lineHeight);
    WEBXX_CSS_PROP_ALIAS(line-padding, linePadding);
    WEBXX_CSS_PROP_ALIAS(line-snap, lineSnap);
    WEBXX_CSS_PROP_ALIAS(list-style, listStyle);
    WEBXX_CSS_PROP_ALIAS(list-style-image, listStyleImage);
    WEBXX_CSS_PROP_ALIAS(list-style-position, listStylePosition);
    WEBXX_CSS_PROP_ALIAS(list-style-type, listStyleType);
    WEBXX_CSS_PROP(margin);
    WEBXX_CSS_PROP_ALIAS(margin-block, marginBlock);
    WEBXX_CSS_PROP_ALIAS(margin-block-end, marginBlockEnd);
    WEBXX_CSS_PROP_ALIAS(margin-block-start, marginBlockStart);
    WEBXX_CSS_PROP_ALIAS(margin-bottom, marginBottom);
    WEBXX_CSS_PROP_ALIAS(margin-inline, marginInline);
    WEBXX_CSS_PROP_ALIAS(margin-inline-end, marginInlineEnd);
    WEBXX_CSS_PROP_ALIAS(margin-inline-start, marginInlineStart);
    WEBXX_CSS_PROP_ALIAS(margin-left, marginLeft);
    WEBXX_CSS_PROP_ALIAS(margin-right, marginRight);
    WEBXX_CSS_PROP_ALIAS(margin-top, marginTop);
    WEBXX_CSS_PROP_ALIAS(margin-trim, marginTrim);
    WEBXX_CSS_PROP_ALIAS(marker-side, markerSide);
    WEBXX_CSS_PROP(mask);
    WEBXX_CSS_PROP_ALIAS(mask-border, maskBorder);
    WEBXX_CSS_PROP_ALIAS(mask-border-mode, maskBorderMode);
    WEBXX_CSS_PROP_ALIAS(mask-border-outset, maskBorderOutset);
    WEBXX_CSS_PROP_ALIAS(mask-border-repeat, maskBorderRepeat);
    WEBXX_CSS_PROP_ALIAS(mask-border-slice, maskBorderSlice);
    WEBXX_CSS_PROP_ALIAS(mask-border-source, maskBorderSource);
    WEBXX_CSS_PROP_ALIAS(mask-border-width, maskBorderWidth);
    WEBXX_CSS_PROP_ALIAS(mask-clip, maskClip);
    WEBXX_CSS_PROP_ALIAS(mask-composite, maskComposite);
    WEBXX_CSS_PROP_ALIAS(mask-image, maskImage);
    WEBXX_CSS_PROP_ALIAS(mask-mode, maskMode);
    WEBXX_CSS_PROP_ALIAS(mask-origin, maskOrigin);
    WEBXX_CSS_PROP_ALIAS(mask-position, maskPosition);
    WEBXX_CSS_PROP_ALIAS(mask-repeat, maskRepeat);
    WEBXX_CSS_PROP_ALIAS(mask-size, maskSize);
    WEBXX_CSS_PROP_ALIAS(mask-type, maskType);
    WEBXX_CSS_PROP_ALIAS(max-block-size, maxBlockSize);
    WEBXX_CSS_PROP_ALIAS(max-height, maxHeight);
    WEBXX_CSS_PROP_ALIAS(max-inline-size, maxInlineSize);
    WEBXX_CSS_PROP_ALIAS(max-lines, maxLines);
    WEBXX_CSS_PROP_ALIAS(max-width, maxWidth);
    WEBXX_CSS_PROP_ALIAS(min-block-size, minBlockSize);
    WEBXX_CSS_PROP_ALIAS(min-height, minHeight);
    WEBXX_CSS_PROP_ALIAS(min-inline-size, minInlineSize);
    WEBXX_CSS_PROP_ALIAS(min-intrinsic-sizing, minIntrinsicSizing);
    WEBXX_CSS_PROP_ALIAS(min-width, minWidth);
    WEBXX_CSS_PROP_ALIAS(mix-blend-mode, mixBlendMode);
    WEBXX_CSS_PROP_ALIAS(nav-down, navDown);
    WEBXX_CSS_PROP_ALIAS(nav-left, navLeft);
    WEBXX_CSS_PROP_ALIAS(nav-right, navRight);
    WEBXX_CSS_PROP_ALIAS(nav-up, navUp);
    WEBXX_CSS_PROP_ALIAS(object-fit, objectFit);
    WEBXX_CSS_PROP_ALIAS(object-position, objectPosition);
    WEBXX_CSS_PROP(offset);
    WEBXX_CSS_PROP_ALIAS(offset-anchor, offsetAnchor);
    WEBXX_CSS_PROP_ALIAS(offset-distance, offsetDistance);
    WEBXX_CSS_PROP_ALIAS(offset-path, offsetPath);
    WEBXX_CSS_PROP_ALIAS(offset-position, offsetPosition);
    WEBXX_CSS_PROP_ALIAS(offset-rotate, offsetRotate);
    WEBXX_CSS_PROP(opacity);
    WEBXX_CSS_PROP(order);
    WEBXX_CSS_PROP(orphans);
    WEBXX_CSS_PROP(outline);
    WEBXX_CSS_PROP_ALIAS(outline-color, outlineColor);
    WEBXX_CSS_PROP_ALIAS(outline-offset, outlineOffset);
    WEBXX_CSS_PROP_ALIAS(outline-style, outlineStyle);
    WEBXX_CSS_PROP_ALIAS(outline-width, outlineWidth);
    WEBXX_CSS_PROP(overflow);
    WEBXX_CSS_PROP_ALIAS(overflow-anchor, overflowAnchor);
    WEBXX_CSS_PROP_ALIAS(overflow-block, overflowBlock);
    WEBXX_CSS_PROP_ALIAS(overflow-clip-margin, overflowClipMargin);
    WEBXX_CSS_PROP_ALIAS(overflow-inline, overflowInline);
    WEBXX_CSS_PROP_ALIAS(overflow-wrap, overflowWrap);
    WEBXX_CSS_PROP_ALIAS(overflow-x, overflowX);
    WEBXX_CSS_PROP_ALIAS(overflow-y, overflowY);
    WEBXX_CSS_PROP(padding);
    WEBXX_CSS_PROP_ALIAS(padding-block, paddingBlock);
    WEBXX_CSS_PROP_ALIAS(padding-block-end, paddingBlockEnd);
    WEBXX_CSS_PROP_ALIAS(padding-block-start, paddingBlockStart);
    WEBXX_CSS_PROP_ALIAS(padding-bottom, paddingBottom);
    WEBXX_CSS_PROP_ALIAS(padding-inline, paddingInline);
    WEBXX_CSS_PROP_ALIAS(padding-inline-end, paddingInlineEnd);
    WEBXX_CSS_PROP_ALIAS(padding-inline-start, paddingInlineStart);
    WEBXX_CSS_PROP_ALIAS(padding-left, paddingLeft);
    WEBXX_CSS_PROP_ALIAS(padding-right, paddingRight);
    WEBXX_CSS_PROP_ALIAS(padding-top, paddingTop);
    WEBXX_CSS_PROP(page);
    WEBXX_CSS_PROP_ALIAS(page-break-after, pageBreakAfter);
    WEBXX_CSS_PROP_ALIAS(page-break-before, pageBreakBefore);
    WEBXX_CSS_PROP_ALIAS(page-break-inside, pageBreakInside);
    WEBXX_CSS_PROP(pause);
    WEBXX_CSS_PROP_ALIAS(pause-after, pauseAfter);
    WEBXX_CSS_PROP_ALIAS(pause-before, pauseBefore);
    WEBXX_CSS_PROP(perspective);
    WEBXX_CSS_PROP_ALIAS(perspective-origin, perspectiveOrigin);
    WEBXX_CSS_PROP(pitch);
    WEBXX_CSS_PROP_ALIAS(pitch-range, pitchRange);
    WEBXX_CSS_PROP_ALIAS(place-content, placeContent);
    WEBXX_CSS_PROP_ALIAS(place-items, placeItems);
    WEBXX_CSS_PROP_ALIAS(place-self, placeSelf);
    WEBXX_CSS_PROP_ALIAS(play-during, playDuring);
    WEBXX_CSS_PROP_ALIAS(pointer-events, pointerEvents);
    WEBXX_CSS_PROP(position);
    WEBXX_CSS_PROP_ALIAS(print-color-adjust, printColorAdjust);
    WEBXX_CSS_PROP(quotes);
    WEBXX_CSS_PROP_ALIAS(region-fragment, regionFragment);
    WEBXX_CSS_PROP(resize);
    WEBXX_CSS_PROP(rest);
    WEBXX_CSS_PROP_ALIAS(rest-after, restAfter);
    WEBXX_CSS_PROP_ALIAS(rest-before, restBefore);
    WEBXX_CSS_PROP(richness);
    WEBXX_CSS_PROP(right);
    WEBXX_CSS_PROP(rotate);
    WEBXX_CSS_PROP_ALIAS(row-gap, rowGap);
    WEBXX_CSS_PROP_ALIAS(ruby-align, rubyAlign);
    WEBXX_CSS_PROP_ALIAS(ruby-merge, rubyMerge);
    WEBXX_CSS_PROP_ALIAS(ruby-overhang, rubyOverhang);
    WEBXX_CSS_PROP_ALIAS(ruby-position, rubyPosition);
    WEBXX_CSS_PROP(running);
    WEBXX_CSS_PROP(scale);
    WEBXX_CSS_PROP_ALIAS(scroll-behavior, scrollBehavior);
    WEBXX_CSS_PROP_ALIAS(scroll-margin, scrollMargin);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-block, scrollMarginBlock);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-block-end, scrollMarginBlockEnd);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-block-start, scrollMarginBlockStart);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-bottom, scrollMarginBottom);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-inline, scrollMarginInline);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-inline-end, scrollMarginInlineEnd);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-inline-start, scrollMarginInlineStart);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-left, scrollMarginLeft);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-right, scrollMarginRight);
    WEBXX_CSS_PROP_ALIAS(scroll-margin-top, scrollMarginTop);
    WEBXX_CSS_PROP_ALIAS(scroll-padding, scrollPadding);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-block, scrollPaddingBlock);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-block-end, scrollPaddingBlockEnd);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-block-start, scrollPaddingBlockStart);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-bottom, scrollPaddingBottom);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-inline, scrollPaddingInline);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-inline-end, scrollPaddingInlineEnd);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-inline-start, scrollPaddingInlineStart);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-left, scrollPaddingLeft);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-right, scrollPaddingRight);
    WEBXX_CSS_PROP_ALIAS(scroll-padding-top, scrollPaddingTop);
    WEBXX_CSS_PROP_ALIAS(scroll-snap-align, scrollSnapAlign);
    WEBXX_CSS_PROP_ALIAS(scroll-snap-stop, scrollSnapStop);
    WEBXX_CSS_PROP_ALIAS(scroll-snap-type, scrollSnapType);
    WEBXX_CSS_PROP_ALIAS(scrollbar-color, scrollbarColor);
    WEBXX_CSS_PROP_ALIAS(scrollbar-gutter, scrollbarGutter);
    WEBXX_CSS_PROP_ALIAS(scrollbar-width, scrollbarWidth);
    WEBXX_CSS_PROP_ALIAS(shape-image-threshold, shapeImageThreshold);
    WEBXX_CSS_PROP_ALIAS(shape-inside, shapeInside);
    WEBXX_CSS_PROP_ALIAS(shape-margin, shapeMargin);
    WEBXX_CSS_PROP_ALIAS(shape-outside, shapeOutside);
    WEBXX_CSS_PROP_ALIAS(spatial-navigation-action, spatialNavigationAction);
    WEBXX_CSS_PROP_ALIAS(spatial-navigation-contain, spatialNavigationContain);
    WEBXX_CSS_PROP_ALIAS(spatial-navigation-function, spatialNavigationFunction);
    WEBXX_CSS_PROP(speak);
    WEBXX_CSS_PROP_ALIAS(speak-as, speakAs);
    WEBXX_CSS_PROP_ALIAS(speak-header, speakHeader);
    WEBXX_CSS_PROP_ALIAS(speak-numeral, speakNumeral);
    WEBXX_CSS_PROP_ALIAS(speak-punctuation, speakPunctuation);
    WEBXX_CSS_PROP_ALIAS(speech-rate, speechRate);
    WEBXX_CSS_PROP(src);
    WEBXX_CSS_PROP(stress);
    WEBXX_CSS_PROP_ALIAS(string-set, stringSet);
    WEBXX_CSS_PROP_ALIAS(tab-size, tabSize);
    WEBXX_CSS_PROP_ALIAS(table-layout, tableLayout);
    WEBXX_CSS_PROP_ALIAS(text-align, textAlign);
    WEBXX_CSS_PROP_ALIAS(text-align-all, textAlignAll);
    WEBXX_CSS_PROP_ALIAS(text-align-last, textAlignLast);
    WEBXX_CSS_PROP_ALIAS(text-combine-upright, textCombineUpright);
    WEBXX_CSS_PROP_ALIAS(text-decoration, textDecoration);
    WEBXX_CSS_PROP_ALIAS(text-decoration-color, textDecorationColor);
    WEBXX_CSS_PROP_ALIAS(text-decoration-line, textDecorationLine);
    WEBXX_CSS_PROP_ALIAS(text-decoration-skip, textDecorationSkip);
    WEBXX_CSS_PROP_ALIAS(text-decoration-skip-box, textDecorationSkipBox);
    WEBXX_CSS_PROP_ALIAS(text-decoration-skip-ink, textDecorationSkipInk);
    WEBXX_CSS_PROP_ALIAS(text-decoration-skip-inset, textDecorationSkipInset);
    WEBXX_CSS_PROP_ALIAS(text-decoration-skip-self, textDecorationSkipSelf);
    WEBXX_CSS_PROP_ALIAS(text-decoration-skip-spaces, textDecorationSkipSpaces);
    WEBXX_CSS_PROP_ALIAS(text-decoration-style, textDecorationStyle);
    WEBXX_CSS_PROP_ALIAS(text-decoration-thickness, textDecorationThickness);
    WEBXX_CSS_PROP_ALIAS(text-edge, textEdge);
    WEBXX_CSS_PROP_ALIAS(text-emphasis, textEmphasis);
    WEBXX_CSS_PROP_ALIAS(text-emphasis-color, textEmphasisColor);
    WEBXX_CSS_PROP_ALIAS(text-emphasis-position, textEmphasisPosition);
    WEBXX_CSS_PROP_ALIAS(text-emphasis-skip, textEmphasisSkip);
    WEBXX_CSS_PROP_ALIAS(text-emphasis-style, textEmphasisStyle);
    WEBXX_CSS_PROP_ALIAS(text-group-align, textGroupAlign);
    WEBXX_CSS_PROP_ALIAS(text-indent, textIndent);
    WEBXX_CSS_PROP_ALIAS(text-justify, textJustify);
    WEBXX_CSS_PROP_ALIAS(text-orientation, textOrientation);
    WEBXX_CSS_PROP_ALIAS(text-overflow, textOverflow);
    WEBXX_CSS_PROP_ALIAS(text-shadow, textShadow);
    WEBXX_CSS_PROP_ALIAS(text-space-collapse, textSpaceCollapse);
    WEBXX_CSS_PROP_ALIAS(text-space-trim, textSpaceTrim);
    WEBXX_CSS_PROP_ALIAS(text-spacing, textSpacing);
    WEBXX_CSS_PROP_ALIAS(text-transform, textTransform);
    WEBXX_CSS_PROP_ALIAS(text-underline-offset, textUnderlineOffset);
    WEBXX_CSS_PROP_ALIAS(text-underline-position, textUnderlinePosition);
    WEBXX_CSS_PROP_ALIAS(text-wrap, textWrap);
    WEBXX_CSS_PROP(top);
    WEBXX_CSS_PROP(transform);
    WEBXX_CSS_PROP_ALIAS(transform-box, transformBox);
    WEBXX_CSS_PROP_ALIAS(transform-origin, transformOrigin);
    WEBXX_CSS_PROP_ALIAS(transform-style, transformStyle);
    WEBXX_CSS_PROP(transition);
    WEBXX_CSS_PROP_ALIAS(transition-delay, transitionDelay);
    WEBXX_CSS_PROP_ALIAS(transition-duration, transitionDuration);
    WEBXX_CSS_PROP_ALIAS(transition-property, transitionProperty);
    WEBXX_CSS_PROP_ALIAS(transition-timing-function, transitionTimingFunction);
    WEBXX_CSS_PROP(translate);
    WEBXX_CSS_PROP_ALIAS(unicode-bidi, unicodeBidi);
    WEBXX_CSS_PROP_ALIAS(user-select, userSelect);
    WEBXX_CSS_PROP_ALIAS(vertical-align, verticalAlign);
    WEBXX_CSS_PROP(visibility);
    WEBXX_CSS_PROP_ALIAS(voice-balance, voiceBalance);
    WEBXX_CSS_PROP_ALIAS(voice-duration, voiceDuration);
    WEBXX_CSS_PROP_ALIAS(voice-family, voiceFamily);
    WEBXX_CSS_PROP_ALIAS(voice-pitch, voicePitch);
    WEBXX_CSS_PROP_ALIAS(voice-range, voiceRange);
    WEBXX_CSS_PROP_ALIAS(voice-rate, voiceRate);
    WEBXX_CSS_PROP_ALIAS(voice-stress, voiceStress);
    WEBXX_CSS_PROP_ALIAS(voice-volume, voiceVolume);
    WEBXX_CSS_PROP(volume);
    WEBXX_CSS_PROP_ALIAS(white-space, whiteSpace);
    WEBXX_CSS_PROP(widows);
    WEBXX_CSS_PROP(width);
    WEBXX_CSS_PROP_ALIAS(will-change, willChange);
    WEBXX_CSS_PROP_ALIAS(word-boundary-detection, wordBoundaryDetection);
    WEBXX_CSS_PROP_ALIAS(word-boundary-expansion, wordBoundaryExpansion);
    WEBXX_CSS_PROP_ALIAS(word-break, wordBreak);
    WEBXX_CSS_PROP_ALIAS(word-spacing, wordSpacing);
    WEBXX_CSS_PROP_ALIAS(word-wrap, wordWrap);
    WEBXX_CSS_PROP_ALIAS(wrap-after, wrapAfter);
    WEBXX_CSS_PROP_ALIAS(wrap-before, wrapBefore);
    WEBXX_CSS_PROP_ALIAS(wrap-flow, wrapFlow);
    WEBXX_CSS_PROP_ALIAS(wrap-inside, wrapInside);
    WEBXX_CSS_PROP_ALIAS(wrap-through, wrapThrough);
    WEBXX_CSS_PROP_ALIAS(writing-mode, writingMode);
    WEBXX_CSS_PROP_ALIAS(z-index, zIndex);

    // HTML elements:
    WEBXX_HTML_EL(a);
    WEBXX_HTML_EL(abbr);
    WEBXX_HTML_EL(address);
    WEBXX_HTML_EL(area);
    WEBXX_HTML_EL(article);
    WEBXX_HTML_EL(aside);
    WEBXX_HTML_EL(audio);
    WEBXX_HTML_EL(b);
    WEBXX_HTML_EL(base);
    WEBXX_HTML_EL(bdi);
    WEBXX_HTML_EL(bdo);
    WEBXX_HTML_EL(blockquote);
    WEBXX_HTML_EL(body);
    WEBXX_HTML_EL(br);
    WEBXX_HTML_EL(button);
    WEBXX_HTML_EL(canvas);
    WEBXX_HTML_EL(caption);
    WEBXX_HTML_EL(cite);
    WEBXX_HTML_EL(code);
    WEBXX_HTML_EL(col);
    WEBXX_HTML_EL(colgroup);
    WEBXX_HTML_EL(data);
    WEBXX_HTML_EL(datalist);
    WEBXX_HTML_EL(dd);
    WEBXX_HTML_EL(del);
    WEBXX_HTML_EL(details);
    WEBXX_HTML_EL(dfn);
    WEBXX_HTML_EL(dialog);
    WEBXX_HTML_EL_ALIAS(div,dv);
    WEBXX_HTML_EL_NO_CLOSING_ALIAS(!doctype html,doc);
    WEBXX_HTML_EL(dl);
    WEBXX_HTML_EL(dt);
    WEBXX_HTML_EL(em);
    WEBXX_HTML_EL(embed);
    WEBXX_HTML_EL(fieldset);
    WEBXX_HTML_EL(figcaption);
    WEBXX_HTML_EL(figure);
    WEBXX_HTML_EL(footer);
    WEBXX_HTML_EL(form);
    WEBXX_HTML_EL(h1);
    WEBXX_HTML_EL(h2);
    WEBXX_HTML_EL(h3);
    WEBXX_HTML_EL(h4);
    WEBXX_HTML_EL(h5);
    WEBXX_HTML_EL(h6);
    WEBXX_HTML_EL(head);
    WEBXX_HTML_EL(header);
    WEBXX_HTML_EL_SELF_CLOSING(hr);
    WEBXX_HTML_EL(html);
    WEBXX_HTML_EL(i);
    WEBXX_HTML_EL(iframe);
    WEBXX_HTML_EL_SELF_CLOSING(img);
    WEBXX_HTML_EL(input);
    WEBXX_HTML_EL(ins);
    WEBXX_HTML_EL(kbd);
    WEBXX_HTML_EL(label);
    WEBXX_HTML_EL(legend);
    WEBXX_HTML_EL(li);
    WEBXX_HTML_EL_SELF_CLOSING(link);
    WEBXX_HTML_EL(main);
    WEBXX_HTML_EL(map);
    WEBXX_HTML_EL(mark);
    WEBXX_HTML_EL(math);
    WEBXX_HTML_EL(menu);
    WEBXX_HTML_EL_SELF_CLOSING(meta);
    WEBXX_HTML_EL(meter);
    WEBXX_HTML_EL(nav);
    WEBXX_HTML_EL(noscript);
    WEBXX_HTML_EL(object);
    WEBXX_HTML_EL(ol);
    WEBXX_HTML_EL(optgroup);
    WEBXX_HTML_EL(option);
    WEBXX_HTML_EL(output);
    WEBXX_HTML_EL(p);
    WEBXX_HTML_EL(picture);
    WEBXX_HTML_EL(portal);
    WEBXX_HTML_EL(pre);
    WEBXX_HTML_EL(progress);
    WEBXX_HTML_EL(q);
    WEBXX_HTML_EL(rp);
    WEBXX_HTML_EL(rt);
    WEBXX_HTML_EL(ruby);
    WEBXX_HTML_EL(s);
    WEBXX_HTML_EL(samp);
    WEBXX_HTML_EL(script);
    WEBXX_HTML_EL(section);
    WEBXX_HTML_EL(select);
    WEBXX_HTML_EL(slot);
    WEBXX_HTML_EL(small);
    WEBXX_HTML_EL(source);
    WEBXX_HTML_EL(span);
    WEBXX_HTML_EL(strong);
    // WEBXX_HTML_EL(style); Previously defined to accept CSS rules as children.
    WEBXX_HTML_EL(sub);
    WEBXX_HTML_EL(summary);
    WEBXX_HTML_EL(sup);
    WEBXX_HTML_EL(svg);
    WEBXX_HTML_EL(table);
    WEBXX_HTML_EL(tbody);
    WEBXX_HTML_EL(td);
    WEBXX_HTML_EL_ALIAS(template,template_);
    WEBXX_HTML_EL(textarea);
    WEBXX_HTML_EL(tfoot);
    WEBXX_HTML_EL(th);
    WEBXX_HTML_EL(thead);
    WEBXX_HTML_EL(time);
    WEBXX_HTML_EL(title);
    WEBXX_HTML_EL(tr);
    WEBXX_HTML_EL(track);
    WEBXX_HTML_EL(u);
    WEBXX_HTML_EL(ul);
    WEBXX_HTML_EL(var);
    WEBXX_HTML_EL(video);
    WEBXX_HTML_EL(wbr);

    // HTML element attributes:
    WEBXX_HTML_ATTR(accept);
    WEBXX_HTML_ATTR_ALIAS(accept-charset,acceptCharset);
    WEBXX_HTML_ATTR(accesskey);
    WEBXX_HTML_ATTR(action);
    WEBXX_HTML_ATTR(align);
    WEBXX_HTML_ATTR(allow);
    WEBXX_HTML_ATTR(alt);
    WEBXX_HTML_ATTR(as);
    WEBXX_HTML_ATTR(async);
    WEBXX_HTML_ATTR(autocapitalize);
    WEBXX_HTML_ATTR(autocomplete);
    WEBXX_HTML_ATTR(autocorrect);
    WEBXX_HTML_ATTR(autofill);
    WEBXX_HTML_ATTR(autofocus);
    WEBXX_HTML_ATTR(autoplay);
    WEBXX_HTML_ATTR(blocking);
    WEBXX_HTML_ATTR(buffered);
    WEBXX_HTML_ATTR(capture);
    WEBXX_HTML_ATTR(challenge);
    WEBXX_HTML_ATTR(charset);
    WEBXX_HTML_ATTR(checked);
    WEBXX_HTML_ATTR(cite);
    WEBXX_HTML_ATTR(class);
    WEBXX_HTML_ATTR(code);
    WEBXX_HTML_ATTR(codebase);
    WEBXX_HTML_ATTR(cols);
    WEBXX_HTML_ATTR(colspan);
    WEBXX_HTML_ATTR(content);
    WEBXX_HTML_ATTR(contenteditable);
    WEBXX_HTML_ATTR(contextmenu);
    WEBXX_HTML_ATTR(controls);
    WEBXX_HTML_ATTR(coords);
    WEBXX_HTML_ATTR(crossorigin);
    WEBXX_HTML_ATTR(csp);
    WEBXX_HTML_ATTR(data);
    WEBXX_HTML_ATTR(datetime);
    WEBXX_HTML_ATTR(decoding);
    WEBXX_HTML_ATTR(default);
    WEBXX_HTML_ATTR(defer);
    WEBXX_HTML_ATTR(dir);
    WEBXX_HTML_ATTR(dirname);
    WEBXX_HTML_ATTR(disabled);
    WEBXX_HTML_ATTR(download);
    WEBXX_HTML_ATTR(draggable);
    WEBXX_HTML_ATTR(enctype);
    WEBXX_HTML_ATTR(enterkeyhint);
    WEBXX_HTML_ATTR(for);
    WEBXX_HTML_ATTR(form);
    WEBXX_HTML_ATTR(formaction);
    WEBXX_HTML_ATTR(formenctype);
    WEBXX_HTML_ATTR(formmethod);
    WEBXX_HTML_ATTR(formnovalidate);
    WEBXX_HTML_ATTR(formtarget);
    WEBXX_HTML_ATTR(headers);
    WEBXX_HTML_ATTR(height);
    WEBXX_HTML_ATTR(hidden);
    WEBXX_HTML_ATTR(high);
    WEBXX_HTML_ATTR(href);
    WEBXX_HTML_ATTR(hreflang);
    WEBXX_HTML_ATTR_ALIAS(http-equiv, httpEquiv);
    WEBXX_HTML_ATTR(icon);
    WEBXX_HTML_ATTR(id);
    WEBXX_HTML_ATTR(importance);
    WEBXX_HTML_ATTR(integrity);
    WEBXX_HTML_ATTR(inputmode);
    WEBXX_HTML_ATTR(ismap);
    WEBXX_HTML_ATTR(itemprop);
    WEBXX_HTML_ATTR(keytype);
    WEBXX_HTML_ATTR(kind);
    WEBXX_HTML_ATTR(label);
    WEBXX_HTML_ATTR(lang);
    WEBXX_HTML_ATTR(loading);
    WEBXX_HTML_ATTR(list);
    WEBXX_HTML_ATTR(loop);
    WEBXX_HTML_ATTR(low);
    WEBXX_HTML_ATTR(max);
    WEBXX_HTML_ATTR(maxlength);
    WEBXX_HTML_ATTR(minlength);
    WEBXX_HTML_ATTR(media);
    WEBXX_HTML_ATTR(method);
    WEBXX_HTML_ATTR(min);
    WEBXX_HTML_ATTR(multiple);
    WEBXX_HTML_ATTR(muted);
    WEBXX_HTML_ATTR(name);
    WEBXX_HTML_ATTR(novalidate);
    WEBXX_HTML_ATTR(open);
    WEBXX_HTML_ATTR(optimum);
    WEBXX_HTML_ATTR(pattern);
    WEBXX_HTML_ATTR(ping);
    WEBXX_HTML_ATTR(placeholder);
    WEBXX_HTML_ATTR(poster);
    WEBXX_HTML_ATTR(preload);
    WEBXX_HTML_ATTR(radiogroup);
    WEBXX_HTML_ATTR(readonly);
    WEBXX_HTML_ATTR(referrerpolicy);
    WEBXX_HTML_ATTR(rel);
    WEBXX_HTML_ATTR(required);
    WEBXX_HTML_ATTR(reversed);
    WEBXX_HTML_ATTR(role);
    WEBXX_HTML_ATTR(rows);
    WEBXX_HTML_ATTR(rowspan);
    WEBXX_HTML_ATTR(sandbox);
    WEBXX_HTML_ATTR(scope);
    WEBXX_HTML_ATTR(selected);
    WEBXX_HTML_ATTR(shape);
    WEBXX_HTML_ATTR(size);
    WEBXX_HTML_ATTR(sizes);
    WEBXX_HTML_ATTR(slot);
    WEBXX_HTML_ATTR(span);
    WEBXX_HTML_ATTR(spellcheck);
    WEBXX_HTML_ATTR(src);
    WEBXX_HTML_ATTR(srcdoc);
    WEBXX_HTML_ATTR(srclang);
    WEBXX_HTML_ATTR(srcset);
    WEBXX_HTML_ATTR(start);
    WEBXX_HTML_ATTR(step);
    WEBXX_HTML_ATTR(style);
    WEBXX_HTML_ATTR(tabindex);
    WEBXX_HTML_ATTR(target);
    WEBXX_HTML_ATTR(title);
    WEBXX_HTML_ATTR(translate);
    WEBXX_HTML_ATTR(type);
    WEBXX_HTML_ATTR(usemap);
    WEBXX_HTML_ATTR(value);
    WEBXX_HTML_ATTR(width);
    WEBXX_HTML_ATTR(wrap);
}

#endif // WEBXX_H
