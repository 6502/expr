#include "expr.h"
#include <math.h>
#include <stdio.h>
#include <functional>

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
    const Instruction *ip=&code[0], *end=ip+code.size();
    while (ip != end) {
        switch(ip++->i) {
        case NEG: *ip[0].p = -*ip[1].p; ip+=2; break;
        case ADD: *ip[0].p = *ip[1].p + *ip[2].p; ip+=3; break;
        case SUB: *ip[0].p = *ip[1].p - *ip[2].p; ip+=3; break;
        case MUL: *ip[0].p = *ip[1].p * *ip[2].p; ip+=3; break;
        case DIV: *ip[0].p = *ip[1].p / *ip[2].p; ip+=3; break;
        case LT: *ip[0].p = *ip[1].p < *ip[2].p; ip+=3; break;
        case LE: *ip[0].p = *ip[1].p <= *ip[2].p; ip+=3; break;
        case GT: *ip[0].p = *ip[1].p > *ip[2].p; ip+=3; break;
        case GE: *ip[0].p = *ip[1].p >= *ip[2].p; ip+=3; break;
        case EQ: *ip[0].p = *ip[1].p == *ip[2].p; ip+=3; break;
        case NE: *ip[0].p = *ip[1].p != *ip[2].p; ip+=3; break;
        case AND: *ip[0].p = *ip[1].p && *ip[2].p; ip+=3; break;
        case OR: *ip[0].p = *ip[1].p || *ip[2].p; ip+=3; break;
        case B_AND: *ip[0].p = int(*ip[1].p) & int(*ip[2].p); ip+=3; break;
        case B_OR: *ip[0].p = int(*ip[1].p) | int(*ip[2].p); ip+=3; break;
        case B_XOR: *ip[0].p = int(*ip[1].p) ^ int(*ip[2].p); ip+=3; break;
        case B_SHL: *ip[0].p = int(*ip[1].p) << int(*ip[2].p); ip+=3; break;
        case B_SHR: *ip[0].p = int(*ip[1].p) >> int(*ip[2].p); ip+=3; break;
        case FFLOOR: *ip[0].p = floor(*ip[1].p); ip+=2; break;
        case FABS: *ip[0].p = abs(*ip[1].p); ip+=2; break;
        case FSIN: *ip[0].p = sin(*ip[1].p); ip+=2; break;
        case FCOS: *ip[0].p = cos(*ip[1].p); ip+=2; break;
        case FSQRT: *ip[0].p = sqrt(*ip[1].p); ip+=2; break;
        case FTAN: *ip[0].p = tan(*ip[1].p); ip+=2; break;
        case FATAN: *ip[0].p = atan(*ip[1].p); ip+=2; break;
        case FLOG: *ip[0].p = log(*ip[1].p); ip+=2; break;
        case FEXP: *ip[0].p = exp(*ip[1].p); ip+=2; break;
        case FATAN2: *ip[0].p = atan2(*ip[1].p, *ip[2].p); ip+=3; break;
        case FPOW: *ip[0].p = pow(*ip[1].p, *ip[2].p); ip+=3; break;
        case FUNC0: *ip[1].p = ip[0].f0(); ip+=2; break;
        case FUNC1: *ip[1].p = ip[0].f1(*ip[2].p); ip+=3; break;
        case FUNC2: *ip[1].p = ip[0].f2(*ip[2].p, *ip[3].p); ip+=4; break;
        }
    }
    return *resreg;
}

void Expr::freeReg(double *r, std::vector<double *>& regs) {
    if (std::less_equal<double *>()(&wrk[0], r) &&
        std::less<double *>()(r, &wrk[0]+wrk.size())) {
        regs.push_back(r);
    }
}

double *Expr::reg(std::vector<double *>& regs) {
    if (regs.size() == 0 && wrk.size() < wrk.capacity()) {
        wrk.push_back(0);
        regs.push_back(&wrk.back());
    }
    if (regs.size() == 0) throw MoreRegs();
    double *x = regs.back();
    regs.pop_back();
    return x;
}

Expr Expr::partialParse(const char *& s, std::map<std::string, double>& vars) {
    Expr result;
    const char *s0 = s;
    for (;;) {
        result.wrk.clear();
        result.constants.clear();
        result.code.clear();
        try {
            std::vector<double *> regs;
            result.resreg = result.compile(regs, s, vars, -1);
            skipsp(s);
            break;
        } catch (const Error& re) {
            throw Error(re.what(), s - s0);
        } catch (const MoreRegs&) {
            result.wrk.clear();
            result.wrk.reserve(result.wrk.capacity() + 10);
            s = s0;
        } catch (const MoreConstants&) {
            result.constants.clear();
            result.constants.reserve(result.constants.capacity() + 10);
            s = s0;
        }
    }
    //printf("s0 = \"%s\":\n%s\n\n", s0, result.disassemble().c_str());
    return result;
}

double *Expr::compile(std::vector<double *>& regs,
                      const char *& s, std::map<std::string, double>& vars, int level) {
    if (level == -1) level = max_level;
    if (level == 0) {
        skipsp(s);
        if (*s == '(') {
            s++;
            double *res = compile(regs, s, vars, -1);
            skipsp(s);
            if (*s != ')') throw Error("')' expected");
            s++;
            return res;
        } else if (isdigit((unsigned char)*s) || (*s=='-' && isdigit((unsigned char)s[1]))) {
            char *ss = 0;
            double v = strtod(s, &ss);
            if (ss && ss!=s) {
                s = ss;
                if (constants.size() == constants.capacity()) throw MoreConstants();
                constants.push_back(v);
                return &constants.back();
            } else {
                throw Error("Invalid number");
            }
        } else if (*s == '-') {
            s++;
            double *x = compile(regs, s, vars, 0);
            freeReg(x, regs);
            double *res = reg(regs);
            emit(NEG); emit(res); emit(x);
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
                double *args[2];
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
                double *target = reg(regs);
                if (ii) {
                    emit(id);
                } else {
                    emit(FUNC0 + arity);
                    if (arity == 0) emit(func0[id]);
                    else if (arity == 1) emit(func1[id]);
                    else emit(func2[id]);
                }
                emit(target);
                for (int i=0; i<arity; i++) {
                    emit(args[i]);
                }
                return target;
            } else {
                std::map<std::string, double>::iterator it = vars.find(name);
                if (it != vars.end()) {
                    return &it->second;
                } else {
                    throw Error(std::string("Unknown variable '" + name + "'"));
                }
            }
        } else {
            throw Error("Syntax error");
        }
    }
    double *res = compile(regs, s, vars, level-1);
    while (skipsp(s), *s) {
        std::map<std::string, Operator>::iterator it = operators.find(std::string(s, s+2));
        if (it == operators.end()) it = operators.find(std::string(s, s+1));
        if (it == operators.end() || it->second.level != level) break;
        s += it->first.size();
        double *x = compile(regs, s, vars, level-1);
        freeReg(x, regs);
        freeReg(res, regs);
        double *nr = reg(regs);
        emit(it->second.opcode); emit(nr); emit(res); emit(x);
        res = nr;
    }
    return res;
}

std::string Expr::disassemble() const {
    const char *opnames[] = { "NEG",
                              "ADD", "SUB", "MUL", "DIV", "LT", "LE", "GT", "GE", "EQ", "NE", "AND", "OR",
                              "B_SHL", "B_SHR", "B_AND", "B_OR", "B_XOR",
                              "FSIN", "FCOS", "FFLOOR", "FABS", "FSQRT", "FTAN", "FATAN", "FLOG", "FEXP",
                              "FATAN2", "FPOW",
                              "FUNC0", "FUNC1", "FUNC2" };
    struct AddrName {
        const std::vector<double>& wrk;
        const std::vector<double>& constants;
        std::string operator()(double *p) {
            char buf[100];
            if (wrk.size() &&
                std::less_equal<const double *>()(&wrk[0], p) &&
                std::less_equal<const double *>()(p, &wrk.back())) {
                sprintf(buf, "r%i", int(p - &wrk[0]));
            } else if (constants.size() &&
                std::less_equal<const double *>()(&constants[0], p) &&
                std::less_equal<const double *>()(p, &constants.back())) {
                sprintf(buf, "c%i=%.18g", int(p - &constants[0]), *p);
            } else {
                sprintf(buf, "%p", p);
            }
            return buf;
        }
        AddrName(const std::vector<double>& wrk,
                 const std::vector<double>& constants)
            : wrk(wrk), constants(constants)
        {}
    } addrname(wrk, constants);

    struct FName {
        std::string operator()(void *p) {
            char buf[20];
            sprintf(buf, "%p", p);
            return buf;
        }
    } fname;

    std::string result;
    char buf[200];
    const Instruction *ip = &code[0], *ie = ip+code.size();
    if (wrk.size()) {
        printf("wrk = %p - %p\n", &wrk[0], &wrk.back());
    }
    while (ip != ie) {
        sprintf(buf, "%i: ", int(ip-&code[0]));
        result += buf;
        result += opnames[ip->i];
        switch(ip++->i) {
        case FUNC0:
            result += (" " +
                       fname((void *)ip[0].f0) + "() -> " +
                       addrname(ip[1].p) + "\n");
            ip += 2;
            break;
        case FUNC1:
            result += (" " +
                       fname((void *)ip[0].f1) + "(" +
                       addrname(ip[2].p) + ") -> " +
                       addrname(ip[1].p) + "\n");
            ip += 3;
            break;
        case FUNC2:
            result += (" " +
                       fname((void *)ip[0].f1) + "(" +
                       addrname(ip[2].p) + ", " +
                       addrname(ip[3].p) + ") -> " +
                       addrname(ip[1].p) + "\n");
            ip += 4;
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
            result += (std::string("(") +
                       addrname(ip[1].p) + ") --> " +
                       addrname(ip[0].p) + "\n");
            ip += 2;
            break;
        default:
            result += (std::string("(") +
                       addrname(ip[1].p) + ", " +
                       addrname(ip[2].p) + ") --> " +
                       addrname(ip[0].p) + "\n");
            ip += 3;
            break;
        }
    }
    return result;
}

Expr::Expr(const Expr& other)
    : code(other.code),
      wrk(other.wrk),
      constants(other.constants) {

    struct RConv {
        const std::vector<double>& wrk1;
        std::vector<double>& wrk2;
        const std::vector<double>& constants1;
        std::vector<double>& constants2;

        RConv(const std::vector<double>& wrk1,
              std::vector<double>& wrk2,
              const std::vector<double>& constants1,
              std::vector<double>& constants2)
            : wrk1(wrk1), wrk2(wrk2),
              constants1(constants1), constants2(constants2)
        { }

        double *operator()(double *x) {
            if (wrk1.size() &&
                std::less_equal<const double *>()(&wrk1[0], x) &&
                std::less_equal<const double *>()(x, &wrk1.back())) {
                return &wrk2[x - &wrk1[0]];
            } else if (constants1.size() &&
                       std::less_equal<const double *>()(&constants1[0], x) &&
                       std::less_equal<const double *>()(x, &constants1.back())) {
                return &constants2[x - &constants1[0]];
            }
            return x;
        }
    } rconv(other.wrk, wrk, other.constants, constants);

    resreg = rconv(other.resreg);
    Instruction *ip = &code[0], *ep = ip + code.size();
    while(ip != ep) {
        switch(ip++->i) {
        case FUNC0:
            ip[1].p = rconv(ip[1].p);
            ip += 2;
            break;
        case FUNC1:
            ip[1].p = rconv(ip[1].p);
            ip[2].p = rconv(ip[2].p);
            ip += 3;
            break;
        case FUNC2:
            ip[1].p = rconv(ip[1].p);
            ip[2].p = rconv(ip[2].p);
            ip[3].p = rconv(ip[3].p);
            ip+=4;
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
            ip[0].p = rconv(ip[0].p);
            ip[1].p = rconv(ip[1].p);
            ip += 2;
            break;
        default:
            ip[0].p = rconv(ip[0].p);
            ip[1].p = rconv(ip[1].p);
            ip[2].p = rconv(ip[2].p);
            ip += 3;
            break;
        }
    }
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
