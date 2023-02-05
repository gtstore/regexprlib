#include "regexpr.h"
#include "textbox.h"
#include <string>
#include <stack>
#include <list>

xsyntax _xsyntax;

RegExpr::RegExpr()
        : nfa{}, error_type{0} {}

RegExpr::RegExpr(std::string_view pattern) : RegExpr() {
    compile(pattern);
}

RegExpr::~RegExpr() {
    clear();
}

void RegExpr::Lex::start(std::string_view pattern) {
    begin = pattern.begin();
    end = pattern.end();
}

void RegExpr::Lex::next() {
    if (begin == end) p = 0;
    else p = *begin++;
}

bool RegExpr::Lex::is_letter() {

    return (p != '|' &&
            p != '(' &&
            p != ')' &&
            p != '{' &&
            p != '}' &&
            p != '*' &&
            p != '+' &&
            p != '?' &&
            p != '\\' &&
            p != '\0' ?
            1 : 0);
}

void RegExpr::error() {
    //_xsyntax.setErrorPos(j);
    throw _xsyntax;
}

void RegExpr::clear() {
    for (auto &s: states) delete s;
    states.clear();
    nfa = nullptr;
    error_type = no_error;
}

inline void RegExpr::set_error_type(short et) {
    error_type = et;
}

RegExpr::state_ptr RegExpr::letter(char ch) {
    states.push_back(new Consume{ch});
    return concat(states.back(), Match::get_match());
}

RegExpr::state_ptr RegExpr::concat(state_ptr first, state_ptr second) {
    first->concat(second);
    return first;
}

RegExpr::state_ptr RegExpr::o_r(state_ptr left, state_ptr right, state_ptr dummy) {
    states.push_back(new Split{left, right});
    if (dummy) {

        /*while (true) {
            auto next = left->next();
            if (next.first == Match::get_match() || next.second == Match::get_match()) {
                left->concat(dummy);
                break;
            }
        }
        while (true) {
            auto next = right->next();
            if (next.first == Match::get_match() || next.second == Match::get_match()) {
                right->concat(dummy);
                break;
            }
        }

        auto [next_left, next_right] = right->next();
        if (next_left == Match::get_match() || next_right == Match::get_match()) right->concat(dummy);
    */

        left->concat(dummy);
        right->concat(dummy);
    }
    /*else {
        left->concat(left);
        left->concat(Match::get_match());
    }*/

    return states.back();
}

RegExpr::state_ptr RegExpr::repeat(state_ptr s) {

    state_ptr split = o_r(s, Match::get_match());
    concat(s, split);
    return split;
}

RegExpr::state_ptr RegExpr::repeat(state_ptr first_group, state_ptr last_group) {

    state_ptr split = o_r(first_group, Match::get_match());
    concat(last_group, split);

    //return last_group;
    return split;
}

RegExpr::state_ptr RegExpr::new_dummy() {

    states.push_back(new Dummy);
    return states.back();
}

RegExpr::state_ptr RegExpr::get_last_group(state_ptr first_group) {

    auto start = first_group;
    std::stack<state_ptr> stack;

    do {
        if (!stack.empty()) {
            start = stack.top();
            stack.pop();
        }
        while (start != Match::get_match() && start != nullptr) {
            auto [left, right] = start->next();
            //if (left == Match::get_match())
            //return left;
            //return start;
            if (right) stack.push(right);
            start = left;
            //if (left == Match::get_match()) break;
        }
    } while (!stack.empty());

    return start;
}

void RegExpr::compile(std::string_view pattern) {
    clear();
    set_error_type(compiler_error);
    Lex::start(pattern);
    nfa = list();
    this->pattern = pattern;
}


RegExpr::state_ptr RegExpr::list(state_ptr dummy) {

    Lex::next();
    state_ptr last = element();
    if (Lex::p == '|') last = o_r(last, list(dummy), dummy);
    return last;
}

size_t RegExpr::quantifier(size_t s2, size_t next_s, size_t prev_s, size_t d0, size_t d1) {

    return size_t();
}

RegExpr::state_ptr RegExpr::element() {

    state_ptr last{nullptr};
    state_ptr first_group{nullptr};
    state_ptr last_group{nullptr};

    if (Lex::p == '(') {
        //j++;

        last_group = new_dummy();
        first_group = list(last_group);

        if (Lex::p == ')') {
            //last_group = get_last_group(first_group);
            Lex::next();
            /*nfa[state].n1 = nfa[state].n2 = state + 1;
            nfa.emplace_back();
            state++;
            j++;*/
        } else
            error();
    } else {
        last = value();
        Lex::next();
    }

    size_t d0{0};
    size_t d1{0};
    if (Lex::p == '{') {
        //j++;
        if (std::isdigit(Lex::p)) {
            d0 = strtoull(&Lex::p, nullptr, 10);
            //j++;
        } else error();

        if (Lex::p == ',') {
            d1 = std::string::npos;
            //j++;
        }
        if (std::isdigit(Lex::p)) {
            d1 = strtoull(&Lex::p, nullptr, 10);
            //j++;
        } else if (Lex::p == '}') {
            //d1 = 0;
            //j++;
        } else error();

        //quantifier(s2, s2, state, d0, d1);
    }

    if (Lex::p == '*') {
        if (last) last = repeat(last);
        else if (first_group) last = repeat(first_group, last_group);
        Lex::next();

    } else {

    }

    if (Lex::p != '|' &&
        Lex::p != ')' &&
        Lex::p != '*' &&
        Lex::p != '+' &&
        Lex::p != '?' &&
        Lex::p != '\0')
        last = concat(last, element());

    return last;
}

RegExpr::state_ptr RegExpr::value() {

    if (Lex::p == '\\') Lex::next();
    else if (!Lex::is_letter()) {
        error();
        return nullptr;
    }
    return letter(Lex::p);
}

int RegExpr::run(std::string_view::iterator &it, std::string_view::iterator end) {

    if (!nfa) return -1;

    int match{0};
    std::stack<state_ptr> stack;
    state_ptr start = nfa;

    while (start != nullptr || !stack.empty()) {
        if (!stack.empty()) {
            start = stack.top();
            stack.pop();
        }
        while (start != nullptr) {
            auto [left, right] = start->next(it, end, match);
            if (right) stack.push(right);
            if (left == Match::get_match())
                return match;
            start = left;
        }
    }
    ++it;
    return -1;
}

std::optional<std::string_view> RegExpr::match(std::string_view str) {

    auto it = str.begin();
    auto end = str.end();
    int m{-1};

    while (m < 0 && it != end)
        m = run(it, end);

    if (m < 0) return std::nullopt;
    return std::string_view{it - m, it};
}

std::vector<std::string_view> RegExpr::full_match(std::string_view str) {

    auto it = str.begin();
    auto end = str.end();
    std::vector<std::string_view> result;
    int m{0};

    while (it != end) {
        m = run(it, end);
        if (m >= 0) {
            result.emplace_back(it - m, it);
            //++it;
        }
    }
    return result;
}

std::string RegExpr::stringify_tree() {
    textbox result;
    state_ptr last{nullptr};
    bool break_cycle{false};
    result.putbox(2, 0, create_tree_graph(nfa, 132 - 2,
                                          [](state_ptr &e) {
                                              std::string p = e->stringify(), k = p;
                                              //return e.params.empty() ? (k + ' ' + p) : std::move(k);
                                              return p;
                                          },
                                          [&last, &break_cycle](state_ptr &e) {
                                              auto *branches = new std::list<state_ptr>;
                                              if (!break_cycle) {
                                                  if (e != Match::get_match()) {
                                                      std::pair<state_ptr, state_ptr> next{};
                                                      next = e->next();
                                                      //if (last == next.first) next = {nullptr, nullptr};
                                                      if (last == next.first) break_cycle = true;
                                                      if (next.first) branches->push_back(next.first);
                                                      if (next.second) branches->push_back(next.second);
                                                      last = e;
                                                  }
                                              } else break_cycle = false;
                                              return std::make_pair(branches->cbegin(), branches->cend());
                                          },
                                          [](state_ptr &e) { return true; }, // whether simplified horizontal layout can be used
                                          [](state_ptr) { return true; },                 // whether extremely simplified horiz layout can be used
                                          [](state_ptr &e) { return false; }));
    return result.to_string();
}

std::ostream &operator<<(std::ostream &os, const RegExpr &re) {
    os << re.pattern;
    return os;
}
