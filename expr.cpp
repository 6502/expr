#include "expr.h"
#include <math.h>
#include <stdio.h>

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

std::map<std::string, std::pair<int, int> > Expr::functions;
std::vector<double (*)()> Expr::func0;
std::vector<double (*)(double)> Expr::func1;
std::vector<double (*)(double,double)> Expr::func2;

double Expr::eval() const {
    double *wp = &wrk[0];
    const int *cp = &code[0], *ce = cp+code.size();
    while (cp != ce) {
        switch(cp[0]) {
        case CONSTANT: wp[cp[1]] = wp[cp[2]]; cp+=3; break;
        case VARIABLE: wp[cp[1]] = *variables[cp[2]]; cp+=3; break;
        case NEG: wp[cp[1]] = -wp[cp[1]]; cp+=2; break;
        case ADD: wp[cp[1]] += wp[cp[2]]; cp+=3; break;
        case SUB: wp[cp[1]] -= wp[cp[2]]; cp+=3; break;
        case MUL: wp[cp[1]] *= wp[cp[2]]; cp+=3; break;
        case DIV: wp[cp[1]] /= wp[cp[2]]; cp+=3; break;
        case LT:  wp[cp[1]] = (wp[cp[1]] <  wp[cp[2]]); cp+=3; break;
        case LE:  wp[cp[1]] = (wp[cp[1]] <= wp[cp[2]]); cp+=3; break;
        case GT:  wp[cp[1]] = (wp[cp[1]] >  wp[cp[2]]); cp+=3; break;
        case GE:  wp[cp[1]] = (wp[cp[1]] >= wp[cp[2]]); cp+=3; break;
        case EQ:  wp[cp[1]] = (wp[cp[1]] == wp[cp[2]]); cp+=3; break;
        case NE:  wp[cp[1]] = (wp[cp[1]] != wp[cp[2]]); cp+=3; break;
        case AND: wp[cp[1]] = (wp[cp[1]] && wp[cp[2]]); cp+=3; break;
        case OR:  wp[cp[1]] = (wp[cp[1]] || wp[cp[2]]); cp+=3; break;
        case B_OR: wp[cp[1]] = (int(wp[cp[1]]) | int(wp[cp[2]])); cp+=3; break;
        case B_AND: wp[cp[1]] = (int(wp[cp[1]]) & int(wp[cp[2]])); cp+=3; break;
        case B_XOR: wp[cp[1]] = (int(wp[cp[1]]) ^ int(wp[cp[2]])); cp+=3; break;
        case B_SHL: wp[cp[1]] = (int(wp[cp[1]]) << int(wp[cp[2]])); cp+=3; break;
        case B_SHR: wp[cp[1]] = (int(wp[cp[1]]) >> int(wp[cp[2]])); cp+=3; break;
        case FUNC0: wp[cp[2]] = func0[cp[1]](); cp+=3; break;
        case FUNC1: wp[cp[2]] = func1[cp[1]](wp[cp[2]]); cp+=3; break;
        case FUNC2: wp[cp[2]] = func2[cp[1]](wp[cp[2]], wp[cp[3]]); cp+=4; break;
        }
    }
    return wp[0];
}

Expr Expr::partialParse(const char *& s, std::map<std::string, double>& vars) {
    Expr result;
    std::vector<int> regs;
    result.wrk.resize(1);
    const char *s0 = s;
    try {
        result.compile(0, regs, s, vars, -1);
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
    return result;
}

void Expr::compile(int target, std::vector<int>& regs,
                   const char *& s, std::map<std::string, double>& vars, int level) {
    if (level == -1) level = max_level;
    if (level == 0) {
        skipsp(s);
        if (*s == '(') {
            s++;
            compile(target, regs, s, vars, -1);
            skipsp(s);
            if (*s != ')') throw Error("')' expected");
            s++;
        } else if (isdigit((unsigned char)*s) || (*s=='-' && isdigit((unsigned char)s[1]))) {
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
        } else if (*s && (*s == '_' || isalpha((unsigned char)*s))) {
            const char *s0 = s;
            while (*s && (isalpha((unsigned char)*s) || isdigit((unsigned char)*s) || *s == '_')) s++;
            std::string name(s0, s);
            if (*s == '(') {
                std::map<std::string, std::pair<int, int> >::iterator it = functions.find(name);
                if (it == functions.end()) throw Error(std::string("Unknown function '" + name + "'"));
                s++;
                std::vector<int> args;
                int id = it->second.first;
                int arity = it->second.second;
                for (int a=0; a<arity; a++) {
                    args.push_back(a == 0 ? target : reg(regs));
                    compile(args.back(), regs, s, vars, -1);
                    if (a != arity-1) {
                        skipsp(s);
                        if (*s != ',') throw Error("',' expected");
                        s++;
                    }
                }
                skipsp(s);
                if (*s != ')') throw Error("')' expected");
                s++;
                code.push_back(FUNC0 + arity);
                code.push_back(id);
                code.push_back(target);
                for (int i=1; i<arity; i++) {
                    code.push_back(args[i]);
                    regs.push_back(args[i]);
                }
            } else {
                std::map<std::string, double>::iterator it = vars.find(name);
                if (it != vars.end()) {
                    variables.push_back(&it->second);
                    code.push_back(VARIABLE);
                    code.push_back(target);
                    code.push_back(variables.size()-1);
                } else {
                    throw Error(std::string("Unknown variable '" + name + "'"));
                }
            }
        } else {
            throw Error("Syntax error");
        }
    } else {
        compile(target, regs, s, vars, level-1);
        while (skipsp(s), *s) {
            std::map<std::string, Operator>::iterator it = operators.find(std::string(s, s+2));
            if (it == operators.end()) it = operators.find(std::string(s, s+1));
            if (it == operators.end() || it->second.level != level) break;
            s += it->first.size();
            int x = reg(regs);
            compile(x, regs, s, vars, level-1);
            code.push_back(it->second.opcode);
            code.push_back(target);
            code.push_back(x);
            regs.push_back(x);
        }
    }
}

std::string Expr::disassemble() const {
    const char *opnames[] = { "CONSTANT", "VARIABLE",
                              "NEG",
                              "ADD", "SUB", "MUL", "DIV", "LT", "LE", "GT", "GE", "EQ", "NE", "AND", "OR",
                              "B_SHL", "B_SHR", "B_AND", "B_OR", "B_XOR",
                              "FUNC0", "FUNC1", "FUNC2" };
    std::string result;
    char buf[30];
    const char *fn = "?";
    for (int i=0,n=code.size(); i<n; i++) {
        sprintf(buf, "%i: ", i);
        result += buf;
        result += opnames[code[i]];
        switch(code[i]) {
        case CONSTANT:
            sprintf(buf, "(%i = %0.3f)\n", code[i+1], wrk[code[i+2]]);
            i += 2;
            break;
        case VARIABLE:
            sprintf(buf, "(%i = %p -> %0.3f)\n", code[i+1], variables[code[i+2]], *variables[code[i+2]]);
            i += 2;
            break;
        case NEG:
            sprintf(buf, "(%i)\n", code[i+1]);
            i += 1;
            break;
        case FUNC0:
        case FUNC1:
        case FUNC2:
            fn = "?";
            for (std::map<std::string, std::pair<int, int> >::iterator it=functions.begin();
                 it!=functions.end(); ++it) {
                if (it->second.second == code[i]-FUNC0 && it->second.first == code[i+1]) {
                    fn=it->first.c_str();
                }
            }
            switch(code[i]) {
            case FUNC0: sprintf(buf, " %p=%s() -> %i\n",
                                func0[code[i+1]], fn, code[i+2]);
                i+=2; break;
            case FUNC1: sprintf(buf, " %p=%s(%i) -> %i\n",
                                func1[code[i+1]], fn, code[i+2], code[i+2]);
                i+=2; break;
            case FUNC2: sprintf(buf, " %p=%s(%i, %i) -> %i\n",
                                func2[code[i+1]], fn, code[i+2], code[i+3], code[i+2]);
                i+=3; break;
            }
            break;
        default:
            sprintf(buf, "(%i, %i) -> %i\n", code[i+1], code[i+2], code[i+1]);
            i += 2;
            break;
        }
        result += buf;
    }
    return result;
}

int Expr::max_level;
std::map<std::string, Expr::Operator> Expr::operators;

class Expr::Init {
    static double random() {
        return double(rand()) / RAND_MAX;
    }

public:
    Init() {
        const Expr::Operator ops[] =
            {{"*", 1, MUL}, {"/", 1, DIV},
             {"+", 2, ADD}, {"-", 2, SUB},

             {"<<", 3, B_SHL}, {">>", 3, B_SHR},
             {"&", 4, B_AND},
             {"|", 5, B_OR}, {"^", 5, B_XOR},

             {"<", 6, LT}, {">", 6, GT}, {"<=", 6, LE}, {">=", 6, GE}, {"==", 6, EQ}, {"!=", 6, NE},

             {"&&", 7, AND},

             {"||", 8, OR}};
        int n = sizeof(ops) / sizeof(ops[0]);
        for (int i=0; i<n; i++) {
            Expr::operators[ops[i].name] = ops[i];
        }
        Expr::max_level = ops[n-1].level;

        Expr::addFunction("random", random);

        Expr::addFunction("floor", floor);
        Expr::addFunction("abs", fabs);
        Expr::addFunction("sqrt", sqrt);
        Expr::addFunction("sin", sin);
        Expr::addFunction("cos", cos);
        Expr::addFunction("tan", tan);
        Expr::addFunction("atan", atan);

        Expr::addFunction("atan2", atan2);
        Expr::addFunction("pow", pow);
    }
} init_instance;
