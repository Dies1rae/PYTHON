#pragma once

#include <iosfwd>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <variant>
#include <vector>
#include <algorithm>

namespace parse {

    namespace token_type {
        struct Number {
            int value;
        };

        struct Id {
            std::string value;
        };

        struct Char {
            char value;
        };

        struct String {
            std::string value;
        };

        struct Class {};    
        struct Return {};   
        struct If {};      
        struct Else {};     
        struct Def {};      
        struct Newline {};  
        struct Print {};    
        struct Indent {}; 
        struct Dedent {};  
        struct Eof {};   
        struct And {};  
        struct Or {};     
        struct Not {};   
        struct Eq {};   
        struct NotEq {}; 
        struct LessOrEq {};  
        struct GreaterOrEq {};
        struct None {};
        struct True {};
        struct False {};
    }  // namespace token_type

    using TokenBase
        = std::variant<token_type::Number, token_type::Id, token_type::Char, token_type::String,
        token_type::Class, token_type::Return, token_type::If, token_type::Else,
        token_type::Def, token_type::Newline, token_type::Print, token_type::Indent,
        token_type::Dedent, token_type::And, token_type::Or, token_type::Not,
        token_type::Eq, token_type::NotEq, token_type::LessOrEq, token_type::GreaterOrEq,
        token_type::None, token_type::True, token_type::False, token_type::Eof>;

    struct Token : TokenBase {
        using TokenBase::TokenBase;

        template <typename T>
        [[nodiscard]] bool Is() const {
            return std::holds_alternative<T>(*this);
        }

        template <typename T>
        [[nodiscard]] const T& As() const {
            return std::get<T>(*this);
        }

        template <typename T>
        [[nodiscard]] const T* TryAs() const {
            return std::get_if<T>(this);
        }
    };

    bool operator==(const Token& lhs, const Token& rhs);
    bool operator!=(const Token& lhs, const Token& rhs);

    std::ostream& operator<<(std::ostream& os, const Token& rhs);

    class LexerError : public std::runtime_error {
    public:
        using std::runtime_error::runtime_error;
    };

    class Lexer {
    public:
        explicit Lexer(std::istream& input);

        [[nodiscard]] const Token& CurrentToken() const;

        Token NextToken();

        template <typename T>
        const T& Expect() const {
            using namespace std::literals;
            if (this->CurrentToken().Is<T>()) {
                return this->CurrentToken().As<T>();
            }
            throw LexerError("Lexer part error not expected");
        }

        template <typename T, typename U>
        void Expect(const U& value) const {
            using namespace std::literals;
            this->Expect<T>();
            if ((this->root_[this->token_ctr_].As<T>().value != value)) {
                throw LexerError("Lexer part error in values");
            }
        }

        template <typename T>
        const T& ExpectNext() {
            using namespace std::literals;
            this->token_ctr_++;
            if (this->token_ctr_ < this->root_.size()) {
                return this->Expect<T>();
            }
        }

        template <typename T, typename U>
        void ExpectNext(const U& value) {
            using namespace std::literals;
            this->token_ctr_++;
            if (this->token_ctr_ < this->root_.size()) {
                return this->Expect<T>(value);
            }
        }

    private:
        bool LineEmpty(const std::string& s) {
            for (const auto& c : s) {
                if (c != ' ' && c != '#'){
                    return false;
                }
                if (c == '#') {
                    break;
                }
            }
            return true;
        }

        bool IsAlphabet(const char& c) {
            if ((c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z') || (c == '_')) {
                return true;
            }
            return false;
        }

        bool IsNumber(const std::string& s) {
            std::string::const_iterator it = s.begin();
            s[0] == '-' ? it += 1 : it += 0;
            while (it != s.end() && std::isdigit(*it)) ++it;
            return !s.empty() && it == s.end();
        }

        bool IsNumber(const char& c) {
            return std::isdigit(c);
        }

        bool IsOperation(const std::string& c) {
            if (c == "==" || c == ">=" || c == "<=" || c == "!="){
                return true;
            }
            return false;
        }

        bool IsChar(const std::string& c) {
            if (c == "." || c == "," || c == "(" || c == "+" || c == ")" || c == "-" || c == "*" || c == "/" || c == ":"
                || c == "@" || c == "%" || c == "$" || c == "^" || c == "&" || c == ";" || c == "?" || c == "=" || c == "<"
                || c == ">" || c == "!" || c == "{" || c == "}" || c == "[" || c == "]") {
                return true;
            }
            return false;
        }

        bool IsChar(const char c) {
            if (c == '.' || c == ',' || c == '(' || c == '+' || c == ')' || c == '-' || c == '*' || c == '/' || c == ':'
                || c == '@' || c == '%' || c == '$' || c == '^' || c == '&' || c == ';' || c == '?' || c == '=' || c == '<'
                || c == '>' || c == '!' || c == '{' || c == '}' || c == '[' || c == ']') {
                return true;
            }
            return false;
        }

        std::string ReadNumber(std::istream& input) {
            char ch;
            std::string num;
            while (this->IsNumber(input.peek())) {
                input >> ch;
                num += ch;
            }
            return num;
        }

        std::string ReadString(std::istream& input) {
            std::string result;
            char quote;
            input >> std::noskipws >> quote;
            char ch;
            while (true) {
                if (input >> std::noskipws >> ch) {
                    if (ch == quote) {
                        break;
                    } else if (ch == '\\') {
                        char escaped_char;
                        if (input >> escaped_char) {
                            switch (escaped_char) {
                            case 'n':
                                result.push_back('\n');
                                break;
                            case 't':
                                result.push_back('\t');
                                break;
                            case '"':
                                result.push_back('"');
                                break;
                            case '\'':
                                result.push_back('\'');
                                break;
                            case '\\':
                                result.push_back('\\');
                                break;
                            default:
                                throw LexerError("Unrecognized escape sequence \\" + escaped_char);
                            }
                        } else {
                            throw LexerError("Unexpected end of line");
                        }
                    } else if (ch == '\n' || ch == '\r') {
                        throw LexerError("Unexpected end of line");
                    } else {
                        result.push_back(ch);
                    }
                } else {
                    throw LexerError("Unexpected end of line");
                }
            }
            return result;
        }

        std::string ReadId(std::istream& input) {
            char ch;
            std::string id;
            while (this->IsAlphabet(input.peek()) || this->IsNumber(input.peek())) {
                input >> ch;
                id += ch;
            }
            return id;
        }

        std::string ReadCharOperation(std::istream& input) {
            char ch;
            input >> ch;
            std::string id;
            return id;
        }

        void IndentDedentParser(int now, int last) {
            if (now > last) {
                if (now % 2 != 0) {
                    return;
                }
                while (now > last) {
                    this->root_.push_back( { (token_type::Indent()) });
                    now -= 2;
                }
            } else if (last > now) {
                if (last % 2 != 0) {
                    return;
                }
                while (last > now) {
                    this->root_.push_back({ (token_type::Dedent()) });
                    last -= 2;
                }
            }
            return;
        }

        Token LoadNumber(const std::string input) {
            token_type::Number token = { std::stoi(input) };
            return token;
        }

        Token LoadString(const std::string& input) {
            std::string tmp_in = input;
            if (tmp_in.find_first_of('\'') != tmp_in.find_last_of('\'')) {
                tmp_in.erase(std::remove(tmp_in.begin(), tmp_in.end(), '\\'), tmp_in.end());
            }
            if (tmp_in.find_first_of('\"') != tmp_in.find_last_of('\"')) {
                tmp_in.erase(std::remove(tmp_in.begin(), tmp_in.end(), '\\'), tmp_in.end());
            }
            token_type::String token = { tmp_in };
            return token;
        }

        Token LoadChar(const char& input) {
            token_type::Char token = { input };
            return token;
        }

        Token LoadId(const std::string& input) {
            if (input == "class") {
                token_type::Class token = {};
                return token;
            } else if (input == "if") {
                token_type::If token = {};
                return token;
            } else if (input == "else") {
                token_type::Else token = {};
                return token;
            } else if (input == "def") {
                token_type::Def token = {};
                return token;
            } else if (input == "\n") {
                token_type::Newline token = {};
                return token;
            } else if (input == "print") {
                token_type::Print token = {};
                return token;
            } else if (input == "ident") {
                token_type::Indent token = {};
                return token;
            } else if (input == "dedent") {
                token_type::Dedent token = {};
                return token;
            } else if (input == "&&" || input == "and") {
                token_type::And token = {};
                return token;
            } else if (input == "||" || input == "or") {
                token_type::Or token = {};
                return token;
            } else if (input == "not") {
                token_type::Not token = {};
                return token;
            } else if (input == "==") {
                token_type::Eq token = {};
                return token;
            } else if (input == "!=") {
                token_type::NotEq token = {};
                return token;
            } else if (input == "<=") {
                token_type::LessOrEq token = {};
                return token;
            } else if (input == ">=") {
                token_type::GreaterOrEq token = {};
                return token;
            } else if (input == "None") {
                token_type::None token = {};
                return token;
            } else if (input == "True") {
                token_type::True token = {};
                return token;
            } else if (input == "False") {
                token_type::False token = {};
                return token;
            } else if (input == "eof") {
                token_type::Eof token = {};
                return token;
            } else if (input == "return") {
                token_type::Return token = {};
                return token;
            } else {
                token_type::Id token = {input};
                return token;
            }
        }

        size_t token_ctr_ = 0;
        std::vector<Token> root_;
    };

}  // namespace parse