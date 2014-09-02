#include <stdio.h>
#include <time.h>
#include <math.h>
#include "expr.h"

double myrandom() {
    return double(rand()) / RAND_MAX;
}

double sqr(double x) {
    return x*x;
}

double len2(double a, double b) {
    return sqrt(a*a + b*b);
}

int main() {
    Expr::addFunction("sqr", sqr);
    Expr::addFunction("len2", len2);

    struct Test { const char *expr; int err; double result; } tests[] = {
        {"1", -1, 1.0},
        {"1+1", -1, 2.0},
        {"3 * 4", -1, 12.0},
        {"((1))", -1, 1.0},
        {"3*--3", -1, 9.0},
        {"1+2*(3+4)", -1, 15.0},
        {"1+2*(3+4*(5+6.5))", -1, 99.0},
        {"x1", -1, 100.0},
        {"x1+y1*2", -1, 500},
        {"1--1", -1, 2},
        {"1/(8--8) ; this comment is ignored", -1, 0.0625},
        {"1/;internal comment\n(8--8)", -1, 0.0625},
        {"1<2", -1, 1.0},
        {"1<=2", -1, 1.0},
        {"1>2", -1, 0.0},
        {"1>=2", -1, 0.0},
        {"1==2", -1, 0.0},
        {"1!=2", -1, 1.0},
        {"1!=2", -1, 1.0},
        {"1<2 && 3<4", -1, 1.0},
        {"1<2 && 3>4", -1, 0.0},
        {"1<2 || 3<4", -1, 1.0},
        {"1<2 || 3>4", -1, 1.0},
        {"(0.1+0.1+0.1+0.1+0.1+0.1+0.1+0.1+0.1+0.1)!=1.0", -1, 1.0}, // Approximation!

        {"1 | 2 == 3", -1, 1.0},
        {"3 ^ 2 == 1", -1, 1.0},
        {"3 ^ 1 == 2", -1, 1.0},
        {"-1 & 7 == 7", -1, 1.0},
        {"-1 >> 1 == -1", -1, 1.0},
        {"-1 << 1 == -2", -1, 1.0},
        {"(1 << 16)-1 >> 8 == 255", -1, 1.0},

        {"abs(-1)", -1, 1.0},
        {"cos(1-1)", -1, 1.0},
        {"abs(cos(3.141592654 * 0.5)) < 1E-6", -1, 1.0},
        {"cos()", 4, -1},
        {"atan2(12)", 8, -1},
        {"atan2(0, 1)", -1, 0},
        {"abs(atan(1)-3.141592654/4) < 1E-6", -1, 1.0},
        {"random() != 1.2", -1, 1.0},
        {"abs(log(1024)/log(2) - 10) < 1E-6", -1, 1.0},
        {"floor(pow(2, 8) + 0.5) == 256", -1, 1.0},
        {"abs(sqrt(2)*sqrt(2) - 2) < 1E-6", -1, 1.0},
        {"abs(tan(3.141592654/4)-1) < 1E-6", -1, 1.0},
        {"abs(log(exp(13)) - 13) < 1E-6", -1, 1.0},
        {"abs(len2(3,4) - 5) < 1E-6", -1, 1.0},
        {"sqr(3) == 9", -1, 1.0},

        {"1+z2*4", 4, -1},
        {"1+2*", 4, -1},
        {"1+(2*3", 6, -1},
        {"1+2()", 3, -1},
        {"+1", 0, -1},
    };

    std::map<std::string, double> vars;
    vars["x0"] = 3.14;
    vars["y0"] = 2.718;
    vars["x1"] = 100;
    vars["y1"] = 200;

    int errors = 0;
    int ntests = sizeof(tests)/sizeof(tests[0]);
    for (int i=0; i<ntests; i++) {
        double res = -1;
        try {
            Expr expr = Expr::parse(tests[i].expr, vars);
            if (tests[i].err != -1) throw Expr::Error("Parsing should have failed");
            res = expr.eval();
            if (tests[i].result != res) throw Expr::Error("Unexpected result");
        } catch (Expr::Error& err) {
            if (tests[i].err == err.position) {
                // Ok; parsing error position is correct
            } else {
                errors++;
                printf("TEST FAILED: \"%s\" (err=%i, result=%0.3f) --> res=%0.3f\n%s (position=%i)\n\n",
                       tests[i].expr, tests[i].err, tests[i].result,
                       res, err.what(), err.position);
            }
        }
    }
    printf("%i errors on %i tests\n", errors, ntests);

    int w=640, h=480;
    vars["k"] = 10*3.141592654 / ((w*w+h*h)/4);
    double& y = vars["y"];
    double& x = vars["x"];
    std::vector<unsigned char> img(w*h);
    Expr e1("((128 + sin(((x-320)*(x-320) + (y-240)*(y-240))*k)*127) ^"
            " (255 * ((floor(x/128)+floor(y/96)) & 1))) + random()*32-16", vars);
    Expr e;
    e = e1;

    printf("Expression compiled code:\n%s\n", e.disassemble().c_str());
    clock_t start = clock();
    for (int rep=0; rep<10; rep++) {
        int i = 0;
        for (y=0; y<h; y++) {
            for (x=0; x<w; x++) {
                int ie = int(e);
                if (ie < 0) ie = 0; if (ie > 255) ie = 255;
                img[i++] = ie;
            }
        }
    }
    clock_t stop = clock();
    printf("Test image generated in %0.3fms (%.0f pixels/sec)\n",
           (stop - start)*100.0/CLOCKS_PER_SEC,
           double(w*h*10)*CLOCKS_PER_SEC/(stop-start+1));
    FILE *f = fopen("test.pgm", "wb");
    if (f) {
        fprintf(f, "P5\n%i %i 255\n", w, h);
        fwrite(&img[0], 1, w*h, f);
        fclose(f);
    } else {
        fprintf(stderr, "Error generating test.pgm\n");
    }

    clock_t start2 = clock();
    double k = vars["k"];
    for (int rep=0; rep<10; rep++) {
        int i = 0;
        for (y=0; y<h; y++) {
            for (x=0; x<w; x++) {
                int ie = int(
                    (int(128 + sin(((x-320)*(x-320) + (y-240)*(y-240))*k)*127) ^
                     int(255 * (int(floor(x/128)+floor(y/96)) & 1))) + myrandom()*32-16);
                if (ie < 0) ie = 0; if (ie > 255) ie = 255;
                img[i++] = ie;
            }
        }
    }
    clock_t stop2 = clock();
    printf("Test image generated natively in %0.3fms (%.0f pixels/sec)\n",
           (stop2 - start2)*100.0/CLOCKS_PER_SEC,
           double(w*h*10)*CLOCKS_PER_SEC/(stop2-start2+1));
    f = fopen("test_native.pgm", "wb");
    if (f) {
        fprintf(f, "P5\n%i %i 255\n", w, h);
        fwrite(&img[0], 1, w*h, f);
        fclose(f);
    } else {
        fprintf(stderr, "Error generating test_native.pgm\n");
    }

    return errors != 0;
}
