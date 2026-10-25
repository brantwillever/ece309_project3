// src/tools/calculator_eval.cpp — YOU IMPLEMENT (spec §4.6).
//
// calc_evaluate: evaluate a well-formed token list with two stacks
// (one for numbers, one for operators and '('). See spec §4.6 and the
// worked trace in Appendix C. std::stack or std::vector are both fine.

#include "tools/calculator.h"

#include <vector>

namespace {

int precedence(CalcToken::Kind kind) {
    if (kind == CalcToken::Kind::Times || kind == CalcToken::Kind::Divide) {
        return 2;
    }
    return 1;
}

bool apply_operator(CalcToken::Kind op, std::vector<double>* values,
                    std::string* error) {
    if (values->size() < 2) {
        *error = "not enough values for operator";
        return false;
    }

    const double right = values->back();
    values->pop_back();
    const double left = values->back();
    values->pop_back();

    double result = 0.0;
    switch (op) {
        case CalcToken::Kind::Plus:
            result = left + right;
            break;
        case CalcToken::Kind::Minus:
            result = left - right;
            break;
        case CalcToken::Kind::Times:
            result = left * right;
            break;
        case CalcToken::Kind::Divide:
            if (right == 0.0) {
                *error = "division by zero";
                return false;
            }
            result = left / right;
            break;
        default:
            *error = "invalid operator";
            return false;
    }

    values->push_back(result);
    return true;
}

}

bool calc_evaluate(const std::vector<CalcToken>& tokens, double* out,
                   std::string* error) {
    if (out == nullptr || error == nullptr) {
        return false;
    }
    if (tokens.empty()) {
        *error = "empty token list";
        return false;
    }

    std::vector<double> values;
    std::vector<CalcToken::Kind> operators;

    for (const CalcToken& token : tokens) {
        if (token.kind == CalcToken::Kind::Number) {
            values.push_back(token.value);
        } else if (token.kind == CalcToken::Kind::LParen) {
            operators.push_back(token.kind);
        } else if (token.kind == CalcToken::Kind::RParen) {
            while (!operators.empty() &&
                   operators.back() != CalcToken::Kind::LParen) {
                const CalcToken::Kind op = operators.back();
                operators.pop_back();
                if (!apply_operator(op, &values, error)) {
                    return false;
                }
            }
            if (operators.empty()) {
                *error = "unmatched ')'";
                return false;
            }
            operators.pop_back();
        } else if (token.kind == CalcToken::Kind::Plus ||
                   token.kind == CalcToken::Kind::Minus ||
                   token.kind == CalcToken::Kind::Times ||
                   token.kind == CalcToken::Kind::Divide) {
            while (!operators.empty() &&
                   operators.back() != CalcToken::Kind::LParen &&
                   precedence(operators.back()) >= precedence(token.kind)) {
                const CalcToken::Kind op = operators.back();
                operators.pop_back();
                if (!apply_operator(op, &values, error)) {
                    return false;
                }
            }
            operators.push_back(token.kind);
        } else {
            *error = "invalid token";
            return false;
        }
    }

    while (!operators.empty()) {
        const CalcToken::Kind op = operators.back();
        operators.pop_back();
        if (op == CalcToken::Kind::LParen) {
            *error = "unmatched '('";
            return false;
        }
        if (!apply_operator(op, &values, error)) {
            return false;
        }
    }

    if (values.size() != 1) {
        *error = "invalid token sequence";
        return false;
    }

    *out = values.back();
    error->clear();
    return true;
}
