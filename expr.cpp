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
    const Instruction *ip=&code[0], *end=ip+code.size();
    while (ip != end) {
        switch(ip->cc & 255) {
        case MOVE: wp[(ip->cc >> 8) & 255] = wp[(ip->cc >> 16) & 255]; break;
        case LOAD: wp[(ip->cc >> 8) & 255] = *((double *)ip->p); break;
        case NEG: wp[(ip->cc >> 8) & 255] = -wp[(ip->cc >> 16) & 255]; break;
        case ADD: wp[(ip->cc >> 8) & 255] = wp[(ip->cc >> 16) & 255] + wp[(ip->cc >> 24) & 255]; break;
        case SUB: wp[(ip->cc >> 8) & 255] = wp[(ip->cc >> 16) & 255] - wp[(ip->cc >> 24) & 255]; break;
        case MUL: wp[(ip->cc >> 8) & 255] = wp[(ip->cc >> 16) & 255] * wp[(ip->cc >> 24) & 255]; break;
        case DIV: wp[(ip->cc >> 8) & 255] = wp[(ip->cc >> 16) & 255] / wp[(ip->cc >> 24) & 255]; break;
        case LT:  wp[(ip->cc >> 8) & 255] = (wp[(ip->cc >> 16) & 255] <  wp[(ip->cc >> 24) & 255]); break;
        case LE:  wp[(ip->cc >> 8) & 255] = (wp[(ip->cc >> 16) & 255] <= wp[(ip->cc >> 24) & 255]); break;
        case GT:  wp[(ip->cc >> 8) & 255] = (wp[(ip->cc >> 16) & 255] >  wp[(ip->cc >> 24) & 255]); break;
        case GE:  wp[(ip->cc >> 8) & 255] = (wp[(ip->cc >> 16) & 255] >= wp[(ip->cc >> 24) & 255]); break;
        case EQ:  wp[(ip->cc >> 8) & 255] = (wp[(ip->cc >> 16) & 255] == wp[(ip->cc >> 24) & 255]); break;
        case NE:  wp[(ip->cc >> 8) & 255] = (wp[(ip->cc >> 16) & 255] != wp[(ip->cc >> 24) & 255]); break;
        case AND: wp[(ip->cc >> 8) & 255] = (wp[(ip->cc >> 16) & 255] && wp[(ip->cc >> 24) & 255]); break;
        case OR:  wp[(ip->cc >> 8) & 255] = (wp[(ip->cc >> 16) & 255] || wp[(ip->cc >> 24) & 255]); break;
        case B_OR: wp[(ip->cc >> 8) & 255] = (int(wp[(ip->cc >> 16) & 255]) | int(wp[(ip->cc >> 24) & 255])); break;
        case B_AND: wp[(ip->cc >> 8) & 255] = (int(wp[(ip->cc >> 16) & 255]) & int(wp[(ip->cc >> 24) & 255])); break;
        case B_XOR: wp[(ip->cc >> 8) & 255] = (int(wp[(ip->cc >> 16) & 255]) ^ int(wp[(ip->cc >> 24) & 255])); break;
        case B_SHL: wp[(ip->cc >> 8) & 255] = (int(wp[(ip->cc >> 16) & 255]) << int(wp[(ip->cc >> 24) & 255])); break;
        case B_SHR: wp[(ip->cc >> 8) & 255] = (int(wp[(ip->cc >> 16) & 255]) >> int(wp[(ip->cc >> 24) & 255])); break;
        case FFLOOR: wp[(ip->cc >> 8) & 255] = floor(wp[(ip->cc >> 16) & 255]); break;
        case FABS: wp[(ip->cc >> 8) & 255] = fabs(wp[(ip->cc >> 16) & 255]); break;
        case FSIN: wp[(ip->cc >> 8) & 255] = sin(wp[(ip->cc >> 16) & 255]); break;
        case FCOS: wp[(ip->cc >> 8) & 255] = cos(wp[(ip->cc >> 16) & 255]); break;
        case FSQRT: wp[(ip->cc >> 8) & 255] = sqrt(wp[(ip->cc >> 16) & 255]); break;
        case FTAN: wp[(ip->cc >> 8) & 255] = tan(wp[(ip->cc >> 16) & 255]); break;
        case FATAN: wp[(ip->cc >> 8) & 255] = atan(wp[(ip->cc >> 16) & 255]); break;
        case FLOG: wp[(ip->cc >> 8) & 255] = log(wp[(ip->cc >> 16) & 255]); break;
        case FEXP: wp[(ip->cc >> 8) & 255] = exp(wp[(ip->cc >> 16) & 255]); break;
        case FATAN2: wp[(ip->cc >> 8) & 255] = atan2(wp[(ip->cc >> 16) & 255], wp[(ip->cc >> 24) & 255]); break;
        case FPOW: wp[(ip->cc >> 8) & 255] = pow(wp[(ip->cc >> 16) & 255], wp[(ip->cc >> 24) & 255]); break;
        case FUNC0: wp[(ip->cc >> 8) & 255] = ((double (*)())ip->p)(); break;
        case FUNC1: wp[(ip->cc >> 8) & 255] = ((double (*)(double))ip->p)(wp[(ip->cc >> 16) & 255]); break;
        case FUNC2: wp[(ip->cc >> 8) & 255] = ((double (*)(double, double))ip->p)(wp[(ip->cc >> 16) & 255], wp[(ip->cc >> 24) & 255]); break;
        }
        ip++;
    }
    return wp[resreg];
}

Expr Expr::partialParse(const char *& s, std::map<std::string, double>& vars) {
    Expr result;
    std::vector<int> regs;
    const char *s0 = s;
    try {
        result.resreg = result.compile(regs, s, vars, -1)&~READONLY;
        skipsp(s);
    } catch (const Error& re) {
        throw Error(re.what(), s - s0);
    }
    //printf("s0 = \"%s\":\n%s\n\n", s0, result.disassemble().c_str());
    return result;
}

int Expr::compile(std::vector<int>& regs,
                  const char *& s, std::map<std::string, double>& vars, int level) {
    if (level == -1) level = max_level;
    if (level == 0) {
        skipsp(s);
        if (*s == '(') {
            s++;
            int res = compile(regs, s, vars, -1);
            skipsp(s);
            if (*s != ')') throw Error("')' expected");
            s++;
            return res;
        } else if (isdigit((unsigned char)*s) || (*s=='-' && isdigit((unsigned char)s[1]))) {
            char *ss = 0;
            double v = strtod(s, &ss);
            if (ss && ss!=s) {
                int x = wrk.size(); wrk.push_back(v);
                s = (const char *)ss;
                return x | READONLY;
            } else {
                throw Error("Invalid number");
            }
        } else if (*s == '-') {
            s++;
            int x = compile(regs, s, vars, 0);
            freeReg(x, regs);
            int res = reg(regs);
            emit(NEG, res, x);
            return res;
        } else if (*s && (*s == '_' || isalpha((unsigned char)*s))) {
            const char *s0 = s;
            while (*s && (isalpha((unsigned char)*s) || isdigit((unsigned char)*s) || *s == '_')) s++;
            std::string name(s0, s);
            if (*s == '(') {
                bool ii = false;
                std::map<std::string, std::pair<int, int> >::iterator it = functions.find(name);
                if (it == functions.end()) {
                    ii = true;
                    it = inlined.find(name);
                    if (it == inlined.end()) throw Error(std::string("Unknown function '" + name + "'"));
                }
                s++;
                int args[2] = {0};
                int id = it->second.first;
                int arity = it->second.second;
                for (int a=0; a<arity; a++) {
                    args[a] = compile(regs, s, vars, -1);
                    if (a != arity-1) {
                        skipsp(s);
                        if (*s != ',') throw Error("',' expected");
                        s++;
                    }
                }
                skipsp(s);
                if (*s != ')') throw Error("')' expected");
                s++;
                for (int i=0; i<arity; i++) {
                    freeReg(args[i], regs);
                }
                int target = reg(regs);
                if (ii) {
                    emit(id, target, args[0], args[1]);
                } else {
                    emit(FUNC0 + arity, target, args[0], args[1],
                         arity == 0 ? (void *)func0[id] :
                         arity == 1 ? (void *)func1[id] :
                         (void *)func2[id]);
                }
                return target;
            } else {
                std::map<std::string, double>::iterator it = vars.find(name);
                if (it != vars.end()) {
                    int target = reg(regs);
                    emit(LOAD, target, 0, 0, &it->second);
                    return target;
                } else {
                    throw Error(std::string("Unknown variable '" + name + "'"));
                }
            }
        } else {
            throw Error("Syntax error");
        }
    }
    int res = compile(regs, s, vars, level-1);
    while (skipsp(s), *s) {
        std::map<std::string, Operator>::iterator it = operators.find(std::string(s, s+2));
        if (it == operators.end()) it = operators.find(std::string(s, s+1));
        if (it == operators.end() || it->second.level != level) break;
        s += it->first.size();
        int x = compile(regs, s, vars, level-1);
        freeReg(x, regs);
        freeReg(res, regs);
        int nr = reg(regs);
        emit(it->second.opcode, nr, res, x);
        res = nr;
    }
    return res;
}

std::string Expr::disassemble() const {
    const char *opnames[] = { "MOVE", "LOAD",
                              "NEG",
                              "ADD", "SUB", "MUL", "DIV", "LT", "LE", "GT", "GE", "EQ", "NE", "AND", "OR",
                              "B_SHL", "B_SHR", "B_AND", "B_OR", "B_XOR",
                              "FSIN", "FCOS", "FFLOOR", "FABS", "FSQRT", "FTAN", "FATAN", "FLOG", "FEXP",
                              "FATAN2", "FPOW",
                              "FUNC0", "FUNC1", "FUNC2" };
    std::string result;
    char buf[30];
    const char *fn = "?";
    for (int i=0,n=code.size(); i<n; i++) {
        int opcode = code[i].cc & 255;
        int r1 = (code[i].cc >> 8) & 255;
        int r2 = (code[i].cc >> 16) & 255;
        int r3 = (code[i].cc >> 24) & 255;
        sprintf(buf, "%i: ", i);
        result += buf;
        result += opnames[opcode];
        switch(opcode) {
        case MOVE:
            sprintf(buf, "(%i = %i) v=%0.3f\n", r1, r2, wrk[r2]);
            break;
        case LOAD:
            sprintf(buf, "(%i = %p)\n", r1, code[i].p);
            break;
        case NEG:
        case FSIN:
        case FCOS:
        case FFLOOR:
        case FABS:
        case FSQRT:
        case FTAN:
        case FATAN:
        case FLOG:
        case FEXP:
            sprintf(buf, "(%i) -> %i\n", r2, r1);
            break;
        case FATAN2:
        case FPOW:
            sprintf(buf, "(%i, %i) -> %i\n", r2, r3, r1);
            break;
        case FUNC0:
        case FUNC1:
        case FUNC2:
            fn = "?";
            for (std::map<std::string, std::pair<int, int> >::iterator it=functions.begin();
                 it!=functions.end(); ++it) {
                if (it->second.second == opcode-FUNC0 &&
                    (it->second.second == 0 ? (void *)func0[it->second.first] == code[i].p :
                     it->second.second == 0 ? (void *)func1[it->second.first] == code[i].p :
                     (void *)func2[it->second.first] == code[i].p)) {
                    fn=it->first.c_str();
                }
            }
            switch(opcode) {
            case FUNC0: sprintf(buf, " %p=%s() -> %i\n",
                                code[i].p, fn, r1);
                break;
            case FUNC1: sprintf(buf, " %p=%s(%i) -> %i\n",
                                code[i].p, fn, r2, r1);
                break;
            case FUNC2: sprintf(buf, " %p=%s(%i, %i) -> %i\n",
                                code[i].p, fn, r2, r3, r1);
                break;
            }
            break;
        default:
            sprintf(buf, "(%i, %i) -> %i\n", r2, r3, r1);
            break;
        }
        result += buf;
    }
    return result;
}

std::map<std::string, std::pair<int, int> > Expr::inlined;

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

        Expr::inlined["floor"] = std::make_pair(Expr::FFLOOR, 1);
        Expr::inlined["abs"] = std::make_pair(Expr::FABS, 1);
        Expr::inlined["sqrt"] = std::make_pair(Expr::FSQRT, 1);
        Expr::inlined["sin"] = std::make_pair(Expr::FSIN, 1);
        Expr::inlined["cos"] = std::make_pair(Expr::FCOS, 1);
        Expr::inlined["tan"] = std::make_pair(Expr::FTAN, 1);
        Expr::inlined["atan"] = std::make_pair(Expr::FATAN, 1);
        Expr::inlined["log"] = std::make_pair(Expr::FLOG, 1);
        Expr::inlined["exp"] = std::make_pair(Expr::FEXP, 1);
        Expr::inlined["atan2"] = std::make_pair(Expr::FATAN2, 2);
        Expr::inlined["pow"] = std::make_pair(Expr::FPOW, 2);
    }
} init_instance;
