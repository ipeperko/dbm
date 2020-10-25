#include <dbm/xml.hpp>
#include <dbm/dbm_common.hpp>

namespace dbm::xml {

/*
 * -------------------------------------------------------------------
 * Parser
 * -------------------------------------------------------------------
 */

namespace {

class parser
{
public:
    enum class Token
    {
        document_begin, // default state
        pi_start,       // processing instruction start '<?'
        pi_end,         // processing instruction end '?>'
        begin_tag,      // '<'
        end_tag,        // '</'
        close_tag,      // '>'
        empty_element,  // '/>'
        eof,            // end of stream
        error           // any error
    };

    static constexpr size_t npos = -1;

    parser() = default;

    explicit parser(const parser&) = delete;

    explicit parser(parser&&) = delete;

    explicit parser(string_view data)
        : data(data)
        , p2(0)
    {
    }

    ~parser() = default;

    parser& operator=(const parser&) = delete;

    parser& operator=(parser&&) = delete;

    node parse();

    Token find_next_token();

    void begin_tag_parser();

    bool is_last_char() const
    {
        return p2 >= data.size() - 1;
    }

    static bool is_valid_space(char c)
    {
        return c == ' ' || c == '\t' || c == '\n';
    }

    string_view data;
    node* mnode { nullptr };
    size_t p1 { npos }; // current string pointer
    size_t p2 { npos }; // current string pointer
};

node parser::parse()
{
    node root;
    Token last_token(Token::document_begin);
    bool root_closed = false;
    p1 = 0;
    p2 = 0;

    do {
        Token token = find_next_token();

        switch (token) {

            case Token::close_tag:

                if (last_token == Token::begin_tag) {
                    begin_tag_parser();
                }
                else if (last_token == Token::end_tag) {
                    std::string substr(data, p1 + 2, p2 - p1 - 2);

                    if (!mnode) {
                        throw_exception<std::domain_error>("End token error: node is null");
                    }
                    if (substr != mnode->tag()) {
                        throw_exception<std::domain_error>("End token error: tag <" + mnode->tag() + "> not closed");
                    }
                    mnode = mnode->parent();
                    if (!mnode) {
                        root_closed = true;
                    }
                }
                else {
                    throw_exception<std::domain_error>(std::string("Close tag error - ") + data.substr(p1, p2 + 1).data());
                }

                p2 += 1;
                p1 = p2;
                break;

            case Token::empty_element:

                if (!mnode) {
                    throw_exception<std::domain_error>("Empty element token error: node is null");
                }
                if (last_token != Token::begin_tag) {
                    throw_exception<std::domain_error>("Empty element token error");
                }
                begin_tag_parser();
                mnode = mnode->parent();
                if (!mnode) {
                    root_closed = true;
                }

                p2 += 2;
                p1 = p2;
                break;

            case Token::begin_tag:

                if (root_closed) {
                    throw_exception<std::domain_error>("Begin tag error: root already closed");
                }
                else if (last_token == Token::close_tag || last_token == Token::empty_element || last_token == Token::pi_end || last_token == Token::document_begin) {
                    if (last_token == Token::document_begin) {
                        for (size_t r = 0; r < p2; r++) {
                            char c = data[r];
                            if (!is_valid_space(c)) {
                                throw_exception<std::domain_error>("Content is not allowed in prolog");
                            }
                        }
                    }

                    if (!mnode) {
                        mnode = &root;
                    }
                    else {
                        mnode = &mnode->add("");
                    }
                }
                else {
                    throw_exception<std::domain_error>(std::string("Begin tag error: ") + data.substr(p1, p2).data());
                }

                p1 = p2;
                p2 += 1;
                break;

            case Token::end_tag:

                if (!mnode) {
                    throw_exception<std::domain_error>("End tag error - node is null");
                }
                if (mnode->items().empty()) {
                    mnode->set_value(std::string(data, p1, p2 - p1));
                }

                p1 = p2;
                p2 += 2;
                break;

            case Token::pi_start:

                if (last_token != Token::document_begin) {
                    throw_exception<std::domain_error>("Processing instruction begin token");
                }

                for (size_t r = 0; r < p2; r++) {
                    char c = data[r];
                    if (!is_valid_space(c)) {
                        throw_exception<std::domain_error>("Content is not allowed in prolog");
                    }
                }

                p1 = p2;
                p2 += 2;
                break;

            case Token::pi_end:

                if (last_token != Token::pi_start) {
                    throw_exception<std::domain_error>("Processing instruction close token error");
                }

                p2 += 2;
                p1 = p2;
                break;

            default:;
        }

        last_token = token;

    } while (last_token != Token::eof && last_token != Token::error);

    if (!root_closed) {
        throw_exception<std::domain_error>("Root not closed");
    }

    // Check trailing characters
    for (size_t r = p1; r < data.size(); r++) {
        char c = data[r];
        if (!is_valid_space(c)) {
            throw_exception<std::domain_error>("Content is not allowed in prolog");
        }
    }

    return root;
}

parser::Token parser::find_next_token()
{
    for (; p2 < data.size(); ++p2) {

        char c = data[p2];

        switch (c) {
            case ('<'):
                if (is_last_char()) {
                    return Token::error;
                }
                else {
                    char c2 = data[p2 + 1];

                    if (c2 == '/') {
                        return Token::end_tag;
                    }
                    else if (c2 == '?') {
                        return Token::pi_start;
                    }
                    else {
                        return Token::begin_tag;
                    }
                }
                break;

            case ('>'):
                return Token::close_tag;

            case ('/'):
                if (is_last_char()) {
                    return Token::error;
                }
                else {
                    char c2 = data[p2 + 1];
                    if (c2 == '>') {
                        return Token::empty_element;
                    }
                }
                break;

            case ('?'):
                if (is_last_char()) {
                    return Token::error;
                }
                else {
                    char c2 = data[p2 + 1];
                    if (c2 == '>') {
                        return Token::pi_end;
                    }
                }
                break;

            default:;
        }
    }

    return Token::eof;
}

void parser::begin_tag_parser()
{
    size_t p = npos;             // relative position
    size_t s = npos;             // ' ' relative position
    size_t e = npos;             // '=' relative position
    size_t q1 = npos, q2 = npos; // '"' relative position
    bool tag_complete = false;
    std::string text(data, p1 + 1, p2 - p1 - 1);

    // Replace '\t' with space
    while ((p = text.find('\t')) != std::string::npos) {
        text[p] = ' ';
    }

    // Replace '\n' with space
    while ((p = text.find('\n')) != std::string::npos) {
        text[p] = ' ';
    }

    // Eliminate double spaces
    while ((p = text.find("  ")) != std::string::npos) {
        text.replace(p, 2, " ");
    }

    if (text.empty()) {
        throw_exception<std::domain_error>("Begin tag parse error: empty tag");
    }

    // Seek tokens
    for (p = 0; p < text.size(); ++p) {

        char c = text[p];

        if (c == ' ' && q1 == npos) {
            if (p == 0) {
                throw_exception<std::domain_error>("Begin tag parse error: tag not well formed");
            }
            else if (!tag_complete) {
                mnode->set_tag(text.substr(0, p));
                tag_complete = true;
            }

            s = p;
        }
        else if (c == '=') {
            if (s == npos && q1 != npos) {
                throw_exception<std::domain_error>("Begin tag parse error: '=' wrong position");
            }

            e = p;
        }
        else if (c == '"') {
            if (s == npos || e == npos) {
                throw_exception<std::domain_error>("Begin tag parse error: '\"' wrong position");
            }
            else if (q1 == npos) {
                q1 = p;
            }
            else {
                q2 = p;

                std::string attr_key = text.substr(s + 1, e - s - 1);
                if (attr_key.empty()) {
                    throw_exception<std::domain_error>("Begin tag parse error: attribute key is empty");
                }

                std::string attr_val = text.substr(q1 + 1, q2 - q1 - 1);

                while (!attr_val.empty() && attr_val.back() == ' ') {
                    attr_val.pop_back();
                }

                while (!attr_val.empty() && attr_val.front() == ' ') {
                    attr_val.erase(0, 1);
                }
                mnode->attributes()[attr_key] = attr_val;

                s = q1 = q2 = e = npos;
            }
        }
        else if (p == text.size() - 1) {
            if (!tag_complete) {
                mnode->set_tag(text.substr(0, p + 1));
                tag_complete = true;
            }
            break;
        }
    }

    if (!tag_complete) {
        throw_exception<std::domain_error>("Begin tag parse error: tag not complete ????");
    }

    if (e != npos || q1 != npos) {
        throw_exception<std::domain_error>("Begin tag parse error: attribute incomplete");
    }
}

} // namespace

/*
 * -------------------------------------------------------------------
 * Xml Node
 * -------------------------------------------------------------------
 */

node::node(string_view tag, string_view value)
    : tag_(tag)
    , value_(value)
    , parent_(nullptr)
{}

node::node(const node& oth)
{
    *this = oth;
}

node::node(node&& oth) // cannot be marked noexcept
{
    *this = std::move(oth);
}

node::~node()
{
    cleanup();
}

node& node::operator=(const node& oth)
{
    if (this != &oth) {
        tag_ = oth.tag_;
        value_ = oth.value_;
        attrs_ = oth.attrs_;
        items_.clear();

        for (auto const& it : oth.items_) {
            items_.emplace_back(it);
            items_.back().set_parent(this);
        }
    }

    return *this;
}

node& node::operator=(node&& oth) // cannot be marked noexcept
{
    if (this != &oth) {

        if (oth.is_root()) {
            tag_ = oth.tag_;
        }
        else {
            tag_ = std::move(oth.tag());
        }
        value_ = std::move(oth.value_);
        attrs_ = std::move(oth.attrs_);
        items_ = std::move(oth.items_);

        for (auto& it : items_) {
            it.set_parent(this);
        }
    }
    return *this;
}

void node::cleanup()
{
    items_.clear();
    tag_.clear();
    value_.clear();
}

node& node::add(string_view name, string_view param)
{
    items_.emplace_back(name, param);
    node& n = items_.back();
    n.set_parent(this);
    return n;
}

node& node::add(const node& oth)
{
    node& n = add();
    n = oth;
    return n;
}

node& node::add(node&& oth)
{
    node& n = add();
    n = std::move(oth);
    return n;
}

node& node::add(string_view name)
{
    return add(name, "");
}

node& node::add(string_view name, std::nullptr_t)
{
    return add(name, "");
}

std::string& node::tag()
{
    return tag_;
}

const std::string& node::tag() const
{
    return tag_;
}

void node::set_tag(string_view str)
{
    tag_ = str;
}

std::string& node::value()
{
    return value_;
}

const std::string& node::value() const
{
    return value_;
}

void node::set_value(string_view str)
{
    value_ = str;
}

attribute_map& node::attributes()
{
    return attrs_;
}

const attribute_map& node::attributes() const
{
    return attrs_;
}

node::iterator node::find(string_view tag)
{
    for (auto it = begin(); it != end(); it++) {
        if ((it)->tag() == tag) {
            return it;
        }
    }
    return end();
}

node::const_iterator node::find(string_view tag) const
{
    for (auto it = begin(); it != end(); it++) {
        if ((it)->tag() == tag) {
            return it;
        }
    }
    return end();
}

node* node::find_node_recursive(string_view tag)
{
    if (tag_ == tag) {
        return this;
    }

    for (auto& it : items_) {
        node* x = it.find_node_recursive(tag);
        if (x) {
            return x;
        }
    }

    return nullptr;
}

node const* node::find_node_recursive(string_view tag) const
{
    if (tag_ == tag) {
        return this;
    }

    for (auto& it : items_) {
        const node* x = it.find_node_recursive(tag);
        if (x) {
            return x;
        }
    }

    return nullptr;
}

std::vector<node::iterator> node::find_nodes(string_view key)
{
    std::vector<iterator> list;
    for (auto it = begin(); it != end(); it++) {
        if ((it)->tag() == key) {
            list.push_back(it);
        }
    }
    return list;
}

std::vector<node::const_iterator> node::find_nodes(string_view key) const
{
    std::vector<const_iterator> list;
    for (auto it = begin(); it != end(); it++) {
        if ((it)->tag() == key) {
            list.push_back(it);
        }
    }
    return list;
}

node& node::at(size_t idx)
{
    auto it = begin();
    std::advance(it, idx);
    return *it;
}

const node& node::at(size_t idx) const
{
    auto it = begin();
    std::advance(it, idx);
    return *it;
}

node& node::front()
{
    return items_.front();
}

const node& node::front() const
{
    return items_.front();
}

node& node::back()
{
    return items_.back();
}

const node& node::back() const
{
    return items_.back();
}

node::iterator node::begin()
{
    return items_.begin();
}

node::const_iterator node::begin() const
{
    return items_.begin();
}

node::iterator node::end()
{
    return items_.end();
}

node::const_iterator node::end() const
{
    return items_.end();
}

node::Items& node::items()
{
    return items_;
}

const node::Items& node::items() const
{
    return items_;
}

node* node::parent()
{
    return parent_;
}

const node* node::parent() const
{
    return parent_;
}

bool node::is_root() const
{
    return parent_ == nullptr;
}

int node::level() const
{
    int lev = 0;
    const node* p = parent_;
    while (p) {
        p = p->parent_;
        ++lev;
    }

    return lev;
}

node node::parse(string_view data)
{
    return parser(data).parse();
}

node node::parse(std::istream& is)
{
    std::ostringstream ss;
    ss << is.rdbuf();
    return parser(ss.str()).parse();
}

std::string node::to_string(unsigned ident) const
{
    return to_string(true, ident);
}

std::string node::to_string(bool declr, unsigned ident) const
{
    return to_string_helper(declr, 0, ident);
}

std::string node::to_string_helper(bool declr, int level, unsigned int ident) const
{
    std::ostringstream ss;
    unsigned item_ident;

    if (declr) {
        ss << stdXMLDeclaration;
    }

    // Ident
    if (ident) {
        item_ident = level * ident;
        if (is_root() && declr) {
            ss << "\n";
        }
        ss << std::string(item_ident, ' ');
    }

    // Open tag
    ss << "<" << tag_;
    for (const auto& it : attrs_) {
        ss << " " << it.first << "=\"" << it.second << "\"";
    }
    ss << ">";

    if (ident && !items_.empty()) {
        ss << "\n";
    }

    // Value
    ss << value_;

    // Items
    for (const auto& it : items_) {
        ss << it.to_string_helper(false, level + 1, ident);
    }

    if (ident && (value_.empty() && !items_.empty())) {
        ss << std::string(item_ident, ' ');
    }

    // Closing tag
    ss << "</" << tag_ << ">";
    if (ident) {
        ss << "\n";
    }

    return ss.str();
}

} // namespace
