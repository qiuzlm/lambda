#include <cctype>
#include <fstream>
#include <iostream>
#include <map>
#include <string>
#include <variant>

using namespace std;

const string DEFNS_FILE_NAME = "defns.lambda";

void replace_string(string& subject, const string& search, const string& replace) {
    size_t pos = 0;
    while ((pos = subject.find(search, pos)) != std::string::npos) {
        subject.replace(pos, search.length(), replace);
        pos += replace.length();
    }
}

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

    string term_string() {
        if (is_variable())
            return to_string(get_variable());
        string result;
        if (is_abstraction()) {
            result += '\\'; 
            auto body = get_abstraction();
            if (!body->is_abstraction())
                result += ' ';
            return result + body->term_string();
        }
        auto [function, argument] = get_application();
        if (function->is_abstraction()) {
            result += '(' + function->term_string() + ')';
        } else {
            result += function->term_string();
        }
        result += ' ';
        if (!argument->is_variable()) {
            result += '(' + argument->term_string() + ')';
        } else {
            result += argument->term_string();
        }
        return result;
    }

    void print() {
        string print_string = term_string();
        replace_string(print_string, "\\", "λ");
        cout << print_string << endl;
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

struct Length {
    bool operator()(const std::string& a, const std::string& b) const {
        return a.size() > b.size() || (a.size() == b.size() && a < b);
    }
};

bool add_definition(string input, map<string, string, Length>& definitions) {
    if (input.compare(0, 4, "let ") != 0)
        return false;
    if (input.size() < 7) {
        cout << "syntax error" << endl << endl;
        return false;
    }
    int index = 4;
    string name;
    while (input[index] != '=' && index < input.size()) {
        name += input[index];
        index++;
    }
    if (index + 1 >= input.size()) {
        cout << "syntax error" << endl << endl;
        return false;
    }
    replace_string(name, " ", "");
    string stored_term_string = input.substr(index + 1);
    if (name == "" || stored_term_string == "") {
        cout << "syntax error" << endl << endl;
        return false;
    }
    for (const auto& [name, stored_term] : definitions)
        replace_string(stored_term_string, name, stored_term);
    auto stored_term = Term::parse(stored_term_string);
    if (!stored_term) {
        cout << "syntax error" << endl << endl;
        return false;
    }
    stored_term->reduce();
    stored_term_string = stored_term->term_string();
    cout << ":: let " << name << " = "; 
    stored_term->print();
    definitions[name] = "(" + stored_term_string + ")";
    return true;
}

int main() {
    map<string, string, Length> definitions;
    ifstream file(DEFNS_FILE_NAME);
    if (file.is_open()) {
        string line;
        int line_number = 1;
        while (getline(file, line)) {
            if (!add_definition(line, definitions))
                cout << "line " << line_number << ": invalid defn" << endl;
            line_number++;
        }
        cout << endl;
    }
    while (true) {
        cout << "λ> ";
        string input;
        getline(cin, input);
        if (add_definition(input, definitions)) {
            cout << endl;
            continue;
        }
        for (const auto& [name, stored_term] : definitions)
            replace_string(input, name, stored_term);
        Term* term = Term::parse(input);
        if (term) {
            cout << "α= ";
            term->print();
            term->reduce();
            cout << "β= ";
            term->print();
        } else {
            cout << "syntax error";
        }
        cout << endl;
    }
    return 0;
}
