#include <iostream>
#include <string>
#include <variant>

using namespace std;

class Term {
    using Variable = int;
    using Abstraction = Term*;
    using Application = pair<Term*, Term*>;

    variant<Variable, Abstraction, Application> term;

    bool is_variable() { return holds_alternative<Variable>(term); }
    bool is_abstraction() { return holds_alternative<Abstraction>(term); }
    bool is_application() { return holds_alternative<Application>(term); }

    Variable get_variable() { return get<Variable>(term); }
    Abstraction get_abstraction() { return get<Abstraction>(term); }
    Application get_application() { return get<Application>(term); }

    void substitute(Term* substitution, int depth) {
        if (is_variable()) {
            if (depth == get_variable())
                term = substitution->term;
            return;
        }
        if (is_abstraction()) {
            get_abstraction()->substitute(substitution, depth + 1);
            return;
        }
        auto [function, argument] = get_application();
        function->substitute(substitution, depth);
        argument->substitute(substitution, depth);
    }

public:
    Term(int variable) : term(variable) {}
    Term(Term* body) : term(body) {}
    Term(Term* function, Term* argument) : term(make_pair(function, argument)) {}

    ~Term() {
        if (is_abstraction())
            delete get_abstraction();
        if (is_application()) {
            auto [function, argument] = get_application();
            delete function;
            delete argument;
        }
    }

    static Term* parse(string expression) {
        if (expression.empty())
            return nullptr;
        if (expression[0] == ' ')
            return parse(expression.substr(1));

        int end = expression.size() - 1;

        if (expression.back() == ' ')
            return parse(expression.substr(0, end));
        if (expression[0] == '\\') {
            auto result = parse(expression.substr(1));
            if (result)
                return new Term(result);
            return nullptr;
        }

        int last_term_index = end;
        if (expression.back() == ')') {
            int parentheses = 0;
            while (last_term_index > 0) {
                if (expression[last_term_index] == ')')
                    parentheses++;
                else if (expression[last_term_index] == '(') {
                    if (--parentheses == 0)
                        break;
                }
                last_term_index--;
            }
            while (last_term_index > 0) {
                if (expression[last_term_index - 1] != ' ' &&
                    expression[last_term_index - 1] != '\\')
                    break;
                last_term_index--;
            }
        } else {
            while (
                expression[last_term_index] != ' ' &&
                expression[last_term_index] != ')' &&
                last_term_index > 0)
                last_term_index--;
        }

        if (last_term_index == 0) {
            if (isdigit(expression[0]))
                return new Term(stoi(expression));
            return parse(expression.substr(1, end - 1));
        }

        Term* function;
        Term* argument;
        if (expression[0] == '(' && expression.back() != ')') {
            function = parse(expression.substr(0, last_term_index + 1));
            argument = parse(expression.substr(last_term_index + 1, end));
        } else {
            function = parse(expression.substr(0, last_term_index));
            argument = parse(expression.substr(last_term_index, end));
        }
        if (function && argument)
            return new Term(function, argument);
        return nullptr;
    }

    void print() {
        if (is_variable()) {
            cout << get_variable();
            return;
        }
        if (is_abstraction()) {
            cout << "λ";
            auto body = get_abstraction();
            if (!body->is_abstraction())
                cout << ' ';
            body->print();
            return;
        }
        auto [function, argument] = get_application();
        if (function->is_abstraction()) {
            cout << '(';
            function->print();
            cout << ')';
        } else {
            function->print();
        }
        cout << ' ';
        if (!argument->is_variable()) {
            cout << '(';
            argument->print();
            cout << ')';
        } else {
            argument->print();
        }
    }

    void reduce() {
        if (!is_application()) return;
        auto [function, argument] = get_application();
        function->reduce();
        if (function->is_abstraction()) {
            function->substitute(argument, 0);
            *function = *(function->get_abstraction());
            if (function->is_variable())
                term = function->get_variable();
            else if (function->is_abstraction())
                term = function->get_abstraction();
            else
                term = function->get_application();
            reduce();
        }
    }
};

int main() {
    while (true) {
        cout << "λ> ";
        string input;
        getline(cin, input);
        Term* term = Term::parse(input);
        if (term) {
            cout << "α= ";
            term->print();
            term->reduce();
            cout << endl << "β= ";
            term->print();
        } else {
            cout << "Syntax error";
        }
        cout << endl;
    }
    return 0;
}
