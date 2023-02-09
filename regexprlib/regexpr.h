#ifndef REGEXPR_H_
#define REGEXPR_H_

//#include <memory>
#include <iostream>
#include <vector>
#include <optional>
#include <string_view>
#include <sstream>

class xsyntax {
public:
    size_t getErrorPos() const { return errorPos; }

    void setErrorPos(size_t pos) { errorPos = pos; }

protected:
    size_t errorPos;
};

class RegExpr {

private:
    class IState;

    using state_ptr = IState*;

    enum {
        no_error = 0,
        compiler_error,
        simulation_error
    };

    class IState {
    public:
        virtual ~IState() = default;
        virtual void concat(state_ptr second) = 0;
        virtual std::pair<state_ptr, state_ptr> next() = 0;
        virtual std::pair<state_ptr, state_ptr> next(std::string_view::iterator &it, std::string_view::iterator end, int &match) = 0;
        virtual std::string stringify() = 0;
    };

    class Consume : public IState {
    public:
        explicit Consume(const char ch) : ch{ch}, out{} {}

        void concat(state_ptr second) override {
            if (out == nullptr || out == Match::get_match()) {
                out = second;
                return;
            } else out->concat(second);
        }
        std::pair<state_ptr, state_ptr> next() override { return {out, nullptr}; }
        std::pair<state_ptr, state_ptr> next(std::string_view::iterator &it, std::string_view::iterator end, int &match) override {
            if (it == end) return {nullptr, nullptr};
            if (ch == *it) {
                ++it;
                ++match;
                return {out, nullptr};
            }
            return {nullptr, nullptr};
        }

        std::string stringify() override {
            return std::string() + "\"" + ch + "\"";
        }

    private:
        const char ch{0};
        state_ptr out{};
    };

    class Split : public IState {
    public:
        Split(state_ptr left, state_ptr right) : left{left}, right{right} {}
        void concat(state_ptr second) override { if (right == Match::get_match()) right = second; }
        std::pair<state_ptr, state_ptr> next() override {
            if (!visited) {
                visited = true;
                return {left, right};
            }
            else return {nullptr, nullptr};
        }
        std::pair<state_ptr, state_ptr> next(std::string_view::iterator &it, std::string_view::iterator end, int &match) override {
            if (last != it) {
                last = it;
                return {left, right};
            } else {
                last = nullptr;
                return {nullptr, nullptr};
            }
            return {left, right};
        }

        std::string stringify() override {
            std::stringstream ss;
            ss << reinterpret_cast<void*>(this);
            return "|split|" + ss.str();
        }

    private:
        state_ptr left{};
        state_ptr right{};
        std::string_view::iterator last{};
        bool visited{false};
    };

    class Dummy : public IState {
    public:
        Dummy() = default;
        void concat(state_ptr second) override { out = second; }
        std::pair<state_ptr, state_ptr> next() override { return {out, nullptr}; }
        std::pair<state_ptr, state_ptr> next(std::string_view::iterator &it, std::string_view::iterator end, int &match) override {
            return {out, nullptr};
        }

        std::string stringify() override {
            return "|dummy|";
        }

    private:
        state_ptr out{};
    };

    class Match : public IState {
    private:
        Match() = default;

    public:
        Match(const Match &) = delete;
        Match &operator=(const Match &) = delete;
        Match(Match &&) = delete;
        Match &operator=(Match &&) = delete;

        void concat(state_ptr second) override {}
        std::pair<state_ptr, state_ptr> next() override { return {get_match(), nullptr}; }
        std::pair<state_ptr, state_ptr> next(std::string_view::iterator &it,
                                                     std::string_view::iterator end, int &match) override {
            return {get_match(), nullptr};
        }

        static const state_ptr get_match() {
            static const state_ptr instance{new Match{}};
            return instance;
        }

        std::string stringify() override {
            return "|match|";
        }
    };

    class Lex {
    public:
        inline static void start(std::string_view pattern);
        inline static void next();
        inline static bool is_letter();
        inline static char p;
        inline static std::string_view::iterator begin;
        inline static std::string_view::iterator end;
    };

private:
    short error_type;
    void set_error_type(short et);
    void error();
    state_ptr list(state_ptr dummy = nullptr);
    size_t quantifier(size_t s2, size_t next_s, size_t prev_s, size_t d0, size_t d1);
    state_ptr element();
    state_ptr value();
    state_ptr letter(char ch);
    static state_ptr concat(state_ptr first, state_ptr second);
    state_ptr o_r(state_ptr left, state_ptr right, state_ptr dummy = nullptr);
    state_ptr repeat(state_ptr s);
    state_ptr repeat(state_ptr first_group, state_ptr last_group);
    state_ptr new_dummy();
    state_ptr get_last_group(state_ptr first_group);
    int run(std::string_view::iterator &it, std::string_view::iterator end);

private:
    state_ptr nfa;
    std::vector<state_ptr> states;
    std::string_view pattern;

public:
    RegExpr();
    explicit RegExpr(std::string_view pattern);
    ~RegExpr();

    void compile(std::string_view pattern);
    void clear();
    std::optional<std::string_view> match(std::string_view str);
    std::vector<std::string_view> full_match(std::string_view str);
    std::string stringify_tree();

    RegExpr &operator=(std::string_view pattern);
    friend std::ostream &operator<<(std::ostream &os, const RegExpr &re);
};

inline RegExpr &RegExpr::operator=(std::string_view pattern) {
    compile(pattern);
    return *this;
}

// error codes of RegExpr::search and RegExpr::searchLen
//constexpr auto REGEXPR_NOT_FOUND = -1;
//constexpr auto REGEXPR_NOT_COMPILED = -2;

#endif
