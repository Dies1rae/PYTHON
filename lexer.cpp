#include "lexer.h"

#include <algorithm>
#include <charconv>
#include <unordered_map>
#include <iostream>

using namespace std;

namespace parse {

    bool operator==(const Token& lhs, const Token& rhs) {
        using namespace token_type;

        if (lhs.index() != rhs.index()) {
            return false;
        }
        if (lhs.Is<Char>()) {
            return lhs.As<Char>().value == rhs.As<Char>().value;
        }
        if (lhs.Is<Number>()) {
            return lhs.As<Number>().value == rhs.As<Number>().value;
        }
        if (lhs.Is<String>()) {
            return lhs.As<String>().value == rhs.As<String>().value;
        }
        if (lhs.Is<Id>()) {
            return lhs.As<Id>().value == rhs.As<Id>().value;
        }
        return true;
    }

    bool operator!=(const Token& lhs, const Token& rhs) {
        return !(lhs == rhs);
    }

    std::ostream& operator<<(std::ostream& os, const Token& rhs) {
        using namespace token_type;

#define VALUED_OUTPUT(type) \
    if (auto p = rhs.TryAs<type>()) return os << #type << '{' << p->value << '}';

        VALUED_OUTPUT(Number);
        VALUED_OUTPUT(Id);
        VALUED_OUTPUT(String);
        VALUED_OUTPUT(Char);

#undef VALUED_OUTPUT

#define UNVALUED_OUTPUT(type) \
    if (rhs.Is<type>()) return os << #type;

        UNVALUED_OUTPUT(Class);
        UNVALUED_OUTPUT(Return);
        UNVALUED_OUTPUT(If);
        UNVALUED_OUTPUT(Else);
        UNVALUED_OUTPUT(Def);
        UNVALUED_OUTPUT(Newline);
        UNVALUED_OUTPUT(Print);
        UNVALUED_OUTPUT(Indent);
        UNVALUED_OUTPUT(Dedent);
        UNVALUED_OUTPUT(And);
        UNVALUED_OUTPUT(Or);
        UNVALUED_OUTPUT(Not);
        UNVALUED_OUTPUT(Eq);
        UNVALUED_OUTPUT(NotEq);
        UNVALUED_OUTPUT(LessOrEq);
        UNVALUED_OUTPUT(GreaterOrEq);
        UNVALUED_OUTPUT(None);
        UNVALUED_OUTPUT(True);
        UNVALUED_OUTPUT(False);
        UNVALUED_OUTPUT(Eof);

#undef UNVALUED_OUTPUT

        return os << "Unknown token :("sv;
    }

    Lexer::Lexer(std::istream& input) : token_ctr_(0) {
        int last_sps = 0;
        int now_sps = 0;
        string line;
        while (!input.eof()) {
            std::getline(input, line);
            if (line.empty()) {
                continue;
            }
            if (this->LineEmpty(line)) {
                continue;
            }

            now_sps = 0;
            if (line[0] == 32) {
                int ptr = 0;
                while (line[ptr++] == 32) {
                    now_sps++;
                }
            }
            this->IndentDedentParser(now_sps, last_sps);
            last_sps = now_sps;

            stringstream ss;
            ss << line;
            char ch;
            while (!ss.eof()) {
                if (ss.peek() == ' ') {
                    ss >> std::noskipws >> ch;
                }
                if (ss.peek() == '#') {
                    break;
                }
                if (this->IsNumber(ss.peek())) {
                    this->root_.push_back(this->LoadNumber(this->ReadNumber(ss)));
                }
                if (ss.peek() == '\'' || ss.peek() == '\"') {
                    this->root_.push_back(this->LoadString(this->ReadString(ss)));
                }
                if (this->IsAlphabet(ss.peek())) {
                    this->root_.push_back(this->LoadId(this->ReadId(ss)));
                }
                if (this->IsChar(ss.peek())) {
                    if (ss.peek() == '=') {
                        ss >> ch;
                        if (ss.peek() == '=') {
                            this->root_.push_back(this->LoadId("=="));
                            ss >> ch;
                        } else {
                            this->root_.push_back(this->LoadChar(ch));
                        }
                    } else if (ss.peek() == '!') {
                        ss >> ch;
                        if (ss.peek() == '=') {
                            this->root_.push_back(this->LoadId("!="));
                            ss >> ch;
                        } else {
                            this->root_.push_back(this->LoadChar(ch));
                        }
                    } else if (ss.peek() == '>') {
                        ss >> ch;
                        if (ss.peek() == '=') {
                            this->root_.push_back(this->LoadId(">="));
                            ss >> ch;
                        } else {
                            this->root_.push_back(this->LoadChar(ch));
                        }
                    } else if (ss.peek() == '<') {
                        ss >> ch;
                        if (ss.peek() == '=') {
                            this->root_.push_back(this->LoadId("<="));
                            ss >> ch;
                        } else {
                            this->root_.push_back(this->LoadChar(ch));
                        }
                    } else {
                        this->root_.push_back(this->LoadChar(ss.peek()));
                        ss >> std::noskipws >> ch;
                    }
                }
            }
            this->root_.push_back(Token(token_type::Newline()));
        }
        this->IndentDedentParser(0, last_sps);
        this->root_.push_back(Token(token_type::Eof()));
    }

    const Token& Lexer::CurrentToken() const {
        if (this->token_ctr_ < this->root_.size()) {
            return this->root_[this->token_ctr_];
        }
    }

    Token Lexer::NextToken() {
        this->token_ctr_++;
        if (this->token_ctr_ < this->root_.size()) {
            return this->root_[this->token_ctr_];
        } else {
            return Token(token_type::Eof());
        }
    }

}  // namespace parse