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

#include <cstring>
#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#define WEBXX_CSS_PROP(NAME)\
    namespace internal { namespace res { constexpr char NAME ## P[] = #NAME; }}\
    using NAME = internal::CssProperty<internal::res::NAME ## P>
#define WEBXX_CSS_PROP_ALIAS(NAME,ALIAS)\
    namespace internal { namespace res { constexpr char ALIAS ## P[] = #NAME; }}\
    using ALIAS = internal::CssProperty<internal::res::ALIAS ## P>
#define WEBXX_CSS_AT_SINGLE(NAME,ALIAS)\
    namespace internal { namespace res { constexpr char ALIAS ## CA[] = #NAME; }}\
    using ALIAS = internal::CssAtSingle<internal::res::ALIAS ## CA>
#define WEBXX_CSS_AT_NESTED(NAME,ALIAS)\
    namespace internal { namespace res { constexpr char ALIAS ## CA[] = #NAME; }}\
    using ALIAS = internal::CssAtNested<internal::res::ALIAS ## CA>
#define WEBXX_HTML_EL(TAG)\
    namespace internal { namespace res { constexpr char TAG ## T[] = #TAG; }}\
    using TAG = internal::HtmlNodeDefined<internal::res::TAG ## T>
#define WEBXX_HTML_EL_ALIAS(TAG,ALIAS)\
    namespace internal { namespace res { constexpr char ALIAS ## T[] = #TAG; }}\
    using ALIAS = internal::HtmlNodeDefined<internal::res::ALIAS ## T>
#define WEBXX_HTML_EL_SELF_CLOSING(TAG)\
    namespace internal { namespace res { constexpr char TAG ## T[] = #TAG; }}\
    using TAG = internal::HtmlNodeDefined<internal::res::TAG ## T,internal::none,true>
#define WEBXX_HTML_ATTR(NAME)\
    namespace internal { namespace res { constexpr char NAME ## A[] = #NAME; }}\
    using _ ## NAME = internal::HtmlAttributeDefined<internal::res::NAME ## A>
#define WEBXX_HTML_ATTR_ALIAS(NAME,ALIAS)\
    namespace internal { namespace res { constexpr char ALIAS ## A[] = #NAME; }}\
    using _ ## ALIAS = internal::HtmlAttributeDefined<internal::res::ALIAS ## A>

namespace Webxx { namespace internal {
    constexpr char none[] = "";
    constexpr size_t hashLen = 8;
}}


////|       |////
////|  CSS  |////
////|       |////


namespace Webxx { namespace internal {
    typedef const char* CssSelector;
    struct CssSelectors : std::vector<CssSelector> {
        CssSelectors(CssSelector selector) :
            std::vector<CssSelector>{selector} {}
        CssSelectors(std::initializer_list<CssSelector> &&selectors) :
            std::vector<CssSelector>{std::move(selectors)} {}
    };

    struct CssRule;
    typedef std::vector<CssRule> CssRules;
    struct CssRule {
        const bool canNest;
        const char* label;
        const std::string value;
        const CssSelectors selectors;
        const CssRules children;

        template<typename ...T>
        CssRule (CssSelectors &&tCssSelectors, T&& ...rules) :
            canNest{true}, label{none}, value{none}, selectors{std::move(tCssSelectors)}, children{std::forward<T>(rules)...} {}
        CssRule (bool tCanNest, const char* tLabel, std::string &&tValue, CssSelectors &&tCssSelectors, CssRules &&tChildren) :
            canNest{tCanNest}, label{tLabel}, value{tValue}, selectors{std::move(tCssSelectors)}, children{std::move(tChildren)} {}
    };

    template<const char* NAME>
    struct CssProperty : CssRule {
        template<typename V>
        CssProperty (V&& tValue) :
            CssRule{false, NAME, std::forward<V>(tValue), {}, {}} {}
    };

    template<const char* PREFIX>
    struct CssAtSingle : CssRule {
        CssAtSingle (CssSelectors &&tSelectors) :
            CssRule{false, PREFIX, none, std::move(tSelectors), {}} {}
    };

    template<const char* PREFIX>
    struct CssAtNested : CssRule {
        CssAtNested (CssSelectors &&tSelectors, CssRules &&tChildren) :
            CssRule{true, PREFIX, none, std::move(tSelectors), std::move(tChildren)} {}
    };

    typedef std::shared_ptr<CssRules> CssRulesSharedPointer;
    struct CssRulesShared : public CssRulesSharedPointer {
        CssRulesShared () :
            CssRulesSharedPointer(std::make_shared<CssRules>(CssRules())) {}
        CssRulesShared (std::initializer_list<CssRule> rules) :
            CssRulesSharedPointer(std::make_shared<CssRules>(CssRules{rules})) {}
    };

    namespace exports {
        template<const char* NAME>
        using property = CssProperty<NAME>;
        using rule = CssRule;
        using styles = CssRulesShared;
    }
}}


////|        |////
////|  HTML  |////
////|        |////


namespace Webxx { namespace internal {
    constexpr char doctype[] = "<!doctype html>";
    constexpr char styleTag[] = "style";

    typedef const char* HtmlAttributeName;
    struct HtmlAttribute {
        const HtmlAttributeName name;
        const std::vector<std::string> values;
    };

    template<HtmlAttributeName NAME>
    struct HtmlAttributeDefined : public HtmlAttribute {
        HtmlAttributeDefined () :
            HtmlAttribute{NAME, {}} {}
        template <typename V>
        HtmlAttributeDefined (V&& tValue) :
            HtmlAttribute{NAME, {std::forward<V>(tValue)}} {}
        template <typename ...V>
        HtmlAttributeDefined (V&& ...tValues) :
            HtmlAttribute{NAME, {std::forward<V>(tValues)...}} {}
    };

    typedef const char* TagName;
    typedef const char* Prefix;
    typedef const bool SelfClosing;
    typedef const bool StyleTarget;

    enum CollectionTarget {
        NONE = 0,
        CSS = 1,
        SCRIPT = 2,
    };

    struct HtmlNodeOptions {
        TagName tagName;
        Prefix prefix;
        SelfClosing selfClosing;
        CollectionTarget collectionTarget;
    };

    struct Component;
    typedef Component* ComponentPointer;
    typedef std::vector<HtmlAttribute> HtmlAttributes;
    struct HtmlNode;
    typedef std::vector<HtmlNode> HtmlNodes;

    struct HtmlNode {
        const HtmlNodeOptions options{none, none, false, NONE};
        const HtmlAttributes attributes;
        const HtmlNodes children;
        const std::string content;
        const CssRules css{};
        const ComponentPointer component{nullptr};

        HtmlNode (std::string &&tContent) :
            content{std::move(tContent)} {}
        HtmlNode (const std::string &tContent) :
            content{tContent} {}
        HtmlNode (const char *tContent) :
            content{tContent} {}
        HtmlNode (
            HtmlNodeOptions &&tOptions,
            HtmlAttributes &&tAttributes,
            HtmlNodes &&tChildren,
            CssRules &&tCss,
            ComponentPointer tComponent
        ) :
            options{std::move(tOptions)},
            attributes{std::move(tAttributes)},
            children{std::move(tChildren)},
            css{std::move(tCss)},
            component{tComponent}
        {}
    };

    template<
        TagName TAG = none,
        Prefix PREFIX = none,
        SelfClosing SELF_CLOSING = false,
        CollectionTarget COLLECTS = NONE
    >
    struct HtmlNodeDefined : public HtmlNode {
        HtmlNodeDefined () :
            HtmlNode{{TAG, PREFIX, SELF_CLOSING, COLLECTS}, {}, {}, {}, nullptr} {}
        HtmlNodeDefined (std::initializer_list<HtmlNode> &&tChildren) :
            HtmlNode{{TAG, PREFIX, SELF_CLOSING, COLLECTS}, {}, std::move(tChildren), {}, nullptr} {}
        HtmlNodeDefined (HtmlNodes &&tChildren) :
            HtmlNode{{TAG, PREFIX, SELF_CLOSING, COLLECTS}, {}, std::move(tChildren), {}, nullptr} {}
        template<typename ...T>
        HtmlNodeDefined (HtmlAttributes &&tAttributes, T&& ...tChildren) :
            HtmlNode{{TAG, PREFIX, SELF_CLOSING, COLLECTS}, std::move(tAttributes), {std::forward<T>(tChildren)...}, {}, nullptr} {}
    };

    struct HtmlStyleNode : HtmlNode {
        HtmlStyleNode () :
            HtmlNode{{styleTag, none, false, NONE}, {}, {}, {}, nullptr} {}
        HtmlStyleNode (std::initializer_list<CssRule> &&tCss) :
            HtmlNode{{styleTag, none, false, NONE}, {}, {}, std::move(tCss), nullptr} {}
        template<typename ...T>
        HtmlStyleNode (HtmlAttributes &&tAttributes, T&& ...tCss) :
            HtmlNode{{styleTag, none, false, NONE}, std::move(tAttributes), {}, {std::forward<T>(tCss)...}, nullptr} {}
    };

    namespace exports {
        // HTML extensibility:
        template<TagName TAG>
        using el = HtmlNodeDefined<TAG>;
        template<HtmlAttributeName NAME>
        using attr = HtmlAttributeDefined<NAME>;

        using attrs = HtmlAttributes;

        // HTML special purpose nodes:
        using doc = HtmlNodeDefined<none, doctype>;
        using text = HtmlNodeDefined<>;
        using fragment = HtmlNodeDefined<>;
        using style = HtmlStyleNode;
        using styleTarget = HtmlNodeDefined<styleTag, none, false, CSS>;
    }
}}


////|             |////
////|  Component  |////
////|             |////


namespace Webxx { namespace internal {

    struct Component : public HtmlNode {
        const std::string name{};
        const CssRulesSharedPointer css{};

        template<typename T, typename N>
        Component (T&& tName, CssRulesShared &&tCss, N&& tHtml) :
            HtmlNode({none, none, false, NONE}, {}, {std::forward<N>(tHtml)}, {}, this),
            name{std::forward<T>(tName)},
            css{std::move(tCss)}
        {}
        template<typename T, typename N>
        Component (T&& tName, const CssRulesShared &tCss, N&& tHtml) :
            HtmlNode({none, none, false, NONE}, {}, {std::forward<N>(tHtml)}, {}, this),
            name{std::forward<T>(tName)},
            css{tCss}
        {}
    };

    namespace exports {
        using component = Component;
    }
}}


////|             |////
////|  Rendering  |////
////|             |////


namespace Webxx { namespace internal {
    constexpr char componentScopePrefix[] = "data-c";
    typedef const CssRules* CssRulesPointer;

    struct CollectedCss {
        const ComponentPointer component;
        CssRulesPointer css;

        bool operator == (const CollectedCss &other) const noexcept {
            return (other.component == component && other.css == css);
        }
    };
}}

template<>
struct std::hash<Webxx::internal::CollectedCss> {
    std::size_t operator() (const Webxx::internal::CollectedCss &collectedCss) const noexcept {
        std::size_t componentHash = std::hash<Webxx::internal::ComponentPointer>{}(collectedCss.component);
        std::size_t cssHash = std::hash<Webxx::internal::CssRulesPointer>{}(collectedCss.css);
        return componentHash ^ (cssHash << 1);
    }
};

namespace Webxx { namespace internal {
    typedef std::unordered_set<CollectedCss> CollectedCsses;

    struct Collector {
        CollectedCsses csses{};

        void collect (const HtmlNode* node) {
            if (node->component) {
                csses.insert({node->component, node->component->css.get()});
            }
            this->collect(&node->children);
        }

        void collect (const HtmlNodes* nodes) {
            for (auto &node : *nodes) {
                this->collect(&node);
            }
        }

        void collect (const void*) {}
    };

    template<size_t LEN>
    std::string hash (std::string input) {
        std::size_t hashI = std::hash<std::string>{}(input);
        char hexString[LEN];
        snprintf(hexString, LEN, "%x", static_cast<int>(hashI));
        return std::string(hexString);
    }

    struct RenderOptions {
        bool hashComponentNames;
    };

    struct Renderer {
        Collector collector;
        std::string out;
        RenderOptions options;

        Renderer(const Collector &tCollector, RenderOptions &&tOptions) :
            collector{tCollector}, options{tOptions} {};

        void render (const HtmlAttribute &attribute, ComponentPointer) {
            out.append(attribute.name);
            if (!attribute.values.empty()) {
                out.append("=\"");
                bool shouldSeparate = false;
                for(auto &value : attribute.values) {
                    if (shouldSeparate) {
                        out.append(" ");
                    }
                    out.append(value);
                    shouldSeparate = true;
                }
                out.append("\"");
            }
        }

        void render (const HtmlAttributes &attributes, ComponentPointer currentComponent) {
            for (auto &attribute : attributes) {
                out.append(" ");
                render(attribute, currentComponent);
            }
        }

        void render (const HtmlNode &node, ComponentPointer currentComponent) {
            if (node.component) {
                currentComponent = node.component;
            }

            if (strlen(node.options.prefix)) {
                out.append(node.options.prefix);
            }

            if (strlen(node.options.tagName)) {
                out.append("<");
                out.append(node.options.tagName);
                if (!node.attributes.empty()) {
                    render(node.attributes, currentComponent);
                }
                if (node.options.selfClosing) {
                    out.append("/");
                }
                if (currentComponent) {
                    out.append(" ");
                    out.append(componentScopePrefix);
                    options.hashComponentNames ?
                        out.append(hash<hashLen>(currentComponent->name)):
                        out.append(currentComponent->name);
                }
                out.append(">");
            }

            if (!node.content.empty()) {
                out.append(node.content);
            }

            if (!node.children.empty()) {
                render(node.children, currentComponent);
            }

            if (!node.css.empty()) {
                render(node.css, nullptr);
            }

            if (node.options.collectionTarget == CSS) {
                render(collector.csses, currentComponent);
            }

            if ((!node.options.selfClosing) && strlen(node.options.tagName)) {
                out.append("</");
                out.append(node.options.tagName);
                out.append(">");
            }
        }

        void render (const HtmlNodes &nodes, ComponentPointer currentComponent) {
            for (auto &node : nodes) {
                render(node, currentComponent);
            }
        }

        void render(const CssSelectors &selectors, ComponentPointer currentComponent) {
            bool shouldSeparate = false;
            for (auto &selector : selectors) {
                if (shouldSeparate) {
                    out.append(",");
                }

                if (currentComponent) {
                    out.append("[");
                    out.append(componentScopePrefix);
                    options.hashComponentNames ?
                        out.append(hash<hashLen>(currentComponent->name)):
                        out.append(currentComponent->name);
                    out.append("]");
                }

                out.append(selector);
                shouldSeparate = true;
            }
        }

        void render (const CssRule &rule, ComponentPointer currentComponent) {
            if (!rule.canNest) {
                // Single line rule:
                out.append(rule.label);
                if (!rule.selectors.empty()) {
                    out.append(" ");
                    render(rule.selectors, nullptr);
                }
                if (!rule.value.empty()) {
                    out.append(":").append(rule.value);
                }
                out.append(";");
            } else {
                // Nested rule:
                if(strlen(rule.label)) {
                    // @rule (has a label and selectors):
                    out.append(rule.label).append(" ");
                    render(rule.selectors, nullptr);
                } else {
                    // Style rule (has no label, only selectors):
                    render(rule.selectors, currentComponent);
                }

                out.append("{");

                for (auto &child : rule.children) {
                    render(child, currentComponent);
                }

                out.append("}");
            }
        }

        void render (const CssRulesShared &css, ComponentPointer currentComponent) {
            return render(*css, currentComponent);
        }

        void render (const CssRules &css, ComponentPointer currentComponent) {
            for (auto &rule : css) {
                render(rule, currentComponent);
            }
        }

        void render (const CollectedCsses &csses, ComponentPointer) {
            for (auto &css : csses) {
                render(*css.css, css.component);
            }
        }
    };

    namespace exports {
        template<typename V>
        Collector collect (V&& node) {
            Collector collector;
            collector.collect(&node);
            return collector;
        }

        template<typename T>
        std::string render (T&& thing, RenderOptions &&options) {
            Collector collector = collect(thing);
            Renderer renderer(collector, std::move(options));
            renderer.render(std::forward<T>(thing), nullptr);
            return renderer.out;
        }

        template<typename T>
        std::string render (T&& thing) {
            return render(std::forward<T>(thing), {});
        }

        template<typename T>
        std::string renderCss (T&& thing, RenderOptions &&options) {
            Collector collector = collect(thing);
            Renderer renderer(collector, std::move(options));
            renderer.render(collector.csses, nullptr);
            return renderer.out;
        }

        template<typename T>
        std::string renderCss (T&& thing) {
            return renderCss(std::forward<T>(thing), {});
        }
    }
}}


////|           |////
////|  Utility  |////
////|           |////


namespace Webxx { namespace internal {

    namespace exports {

        template<typename T, typename F>
        fragment each (T&& items, F&& cb) {
            HtmlNodes nodes;
            nodes.reserve(items.size());

            for (auto &item : items) {
                nodes.push_back(cb(item));
            }

            return fragment{std::move(nodes)};
        }

        template<typename T, typename F>
        fragment maybe (T&& condition, F&& cb) {
            if (condition) {
                return fragment{{}, cb()};
            }
            return fragment{};
        }

        template<typename T, typename V, typename F>
        fragment maybe (T&& condition, V&& forward, F&& cb) {
            if (condition) {
                return fragment{{}, cb(std::forward<V>(forward))};
            }
            return fragment{};
        }
    }
}}


////|          |////
////|  Public  |////
////|          |////


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
    WEBXX_HTML_ATTR(async);
    WEBXX_HTML_ATTR(autocapitalize);
    WEBXX_HTML_ATTR(autocomplete);
    WEBXX_HTML_ATTR(autofocus);
    WEBXX_HTML_ATTR(autoplay);
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
