#if !defined(EXPR2_H_INCLUDED)
#define EXPR_H_INCLUDED
/*
The MIT License (MIT)

Copyright (c) 2014 Andrea Griffini

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

#include <vector>
#include <string>
#include <map>
#include <stdexcept>
#include <stdlib.h>
#include <string.h>

class Expr {

public:
    struct Error : std::runtime_error {
        Error(const std::string& msg)
            : std::runtime_error(msg)
        {}
    };

    double eval() const {
        for (int i=0,n=code.size(); i<n; i++) {
            switch(code[i]) {
            case CONSTANT: wrk[code[i+1]] = wrk[code[i+2]]; i+=2; break;
            case VARIABLE: wrk[code[i+1]] = *variables[code[i+2]]; i+=2; break;
            case NEG: wrk[code[i+1]] = -wrk[code[i+1]]; i+=1; break;
            case ADD: wrk[code[i+1]] += wrk[code[i+2]]; i+=2; break;
            case SUB: wrk[code[i+1]] -= wrk[code[i+2]]; i+=2; break;
            case MUL: wrk[code[i+1]] *= wrk[code[i+2]]; i+=2; break;
            case DIV: wrk[code[i+1]] /= wrk[code[i+2]]; i+=2; break;
            case LT:  wrk[code[i+1]] = (wrk[code[i+1]] <  wrk[code[i+2]]); i+=2; break;
            case LE:  wrk[code[i+1]] = (wrk[code[i+1]] <= wrk[code[i+2]]); i+=2; break;
            case GT:  wrk[code[i+1]] = (wrk[code[i+1]] >  wrk[code[i+2]]); i+=2; break;
            case GE:  wrk[code[i+1]] = (wrk[code[i+1]] >= wrk[code[i+2]]); i+=2; break;
            case EQ:  wrk[code[i+1]] = (wrk[code[i+1]] == wrk[code[i+2]]); i+=2; break;
            case NE:  wrk[code[i+1]] = (wrk[code[i+1]] != wrk[code[i+2]]); i+=2; break;
            case AND: wrk[code[i+1]] = (wrk[code[i+1]] && wrk[code[i+2]]); i+=2; break;
            case OR:  wrk[code[i+1]] = (wrk[code[i+1]] || wrk[code[i+2]]); i+=2; break;
            }
        }
        return wrk[0];
    }

    static void skipsp(const char *& s) {
        for(;;) {
            while (*s && isspace(*s)) s++;
            if (*s == ';') {
                while (*s && *s != '\n') s++;
            } else break;
        }
    }

    static Expr partialParse(const char *& s, std::map<std::string, double>& vars) {
        Expr result;
        std::vector<int> regs;
        result.wrk.resize(1);
        const char *s0 = s;
        try {
            result.compile(0, regs, s, vars, 5);
            skipsp(s);
        } catch (const Error& re) {
            std::string msg(re.what());
            msg += "\n";
            msg += s0;
            msg += "\n";
            for (int i=0; i<s-s0; i++) {
                msg += " ";
            }
            msg += "^ HERE\n";
            throw Error(msg);
        }
        if (result.variables.size() == 0) {
            // Constant expression: just keep result
            std::vector<double> nw(1, result.eval());
            nw.swap(result.wrk);
            std::vector<int>().swap(result.code);
        }
        return result;
    }

    static Expr parse(const char *s, std::map<std::string, double>& vars) {
        Expr expr = partialParse(s, vars);
        if (*s) throw Error("Unexpected extra characters\n");
        return expr;
    }

    Expr(double x = 0.0) {
        wrk.push_back(x);
    }

    void swap(Expr& other) {
        code.swap(other.code);
        wrk.swap(other.wrk);
        variables.swap(other.variables);
    }

    Expr(const char *s, std::map<std::string, double>& m) {
        Expr e = parse(s, m);
        swap(e);
    }

    operator double() const {
        return eval();
    }

private:
    enum { CONSTANT, VARIABLE,
           NEG,
           ADD, SUB, MUL, DIV, LT, LE, GT, GE, EQ, NE, AND, OR };

    std::vector<int> code;
    mutable std::vector<double> wrk;
    std::vector<double *> variables;

    int reg(std::vector<int>& regs) {
        if (regs.size() == 0) {
            wrk.resize(1 + wrk.size());
            regs.push_back(wrk.size()-1);
        }
        int r = regs.back();
        regs.pop_back();
        return r;
    }

    struct Operator {
        const char *name;
        int level;
        int opcode;
    };

    void compile(int target, std::vector<int>& regs,
                 const char *& s, std::map<std::string, double>& vars, int level) {
        const Operator ops[] = {{"*", 1, MUL}, {"/", 1, DIV},
                                {"+", 2, ADD}, {"-", 2, SUB},
                                {"<", 3, LT}, {">", 3, GT}, {"<=", 3, LE}, {">=", 3, GE}, {"==", 3, EQ}, {"!=", 3, NE},
                                {"&&", 4, AND},
                                {"||", 5, OR}};
        if (level == 0) {
            skipsp(s);
            if (*s == '(') {
                s++;
                compile(target, regs, s, vars, 5);
                skipsp(s);
                if (*s != ')') throw Error("')' expected");
                s++;
            } else if (*s >= '0' && *s <= '9') {
                char *ss = 0;
                double v = strtod(s, &ss);
                if (ss && ss!=s) {
                    int x = wrk.size(); wrk.push_back(v);
                    code.push_back(CONSTANT); code.push_back(target); code.push_back(x);
                    s = (const char *)ss;
                } else {
                    throw Error("Invalid number");
                }
            } else if (*s == '-') {
                s++; compile(target, regs, s, vars, 0);
                code.push_back(NEG); code.push_back(target);
            } else if (*s && isalpha(*s)) {
                const char *s0 = s;
                while (*s && (isalpha(*s) || isdigit(*s) || *s == '_')) s++;
                std::map<std::string, double>::iterator it = vars.find(std::string(s0, s));
                if (it != vars.end()) {
                    variables.push_back(&it->second);
                    code.push_back(VARIABLE);
                    code.push_back(target);
                    code.push_back(variables.size()-1);
                } else {
                    throw Error(std::string("Unknown variable '" + std::string(s0, s) + "'"));
                }
            } else {
                throw Error("Syntax error");
            }
        } else {
            compile(target, regs, s, vars, level-1);
            for(;;) {
                skipsp(s);
                int i = sizeof(ops)/sizeof(ops[0]) - 1;
                while (i >= 0 &&
                       (ops[i].level != level ||
                        strncmp(s, ops[i].name, strlen(ops[i].name)) != 0)) --i;
                if (i == -1) break;
                s += strlen(ops[i].name);
                int x = reg(regs);
                compile(x, regs, s, vars, level-1);
                code.push_back(ops[i].opcode);
                code.push_back(target);
                code.push_back(x);
                regs.push_back(x);
            }
        }
    }

};

#endif
