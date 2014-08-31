expr
====

![test image](https://raw.github.com/6502/expr/master/images/test.jpg)

Tiny library for run-time evaluation of expressions for C++.

The code is just one include file and one cpp file (about 500 lines
in total, no dependencies besides standard C++ library) providing
only one class `Expr`.

Usage
-----
You can instantiate an `Expr` object by parsing an expression from a
`const char *`:

    Expr e = Expr::parse("1/(1 + x)", vars);

or also
    
    Expr e("1/(1 + x)", vars);

where `vars` is an `std::map<std::string, double>` instance containing
variables.  Referencing undefined variables is a parse time error.

To evaluate the expression with current variable values just call `e.eval()`.

Warning
-------
For efficiency reasons the variables are stored in parsed expressions
as `double *`. This is fine for `std::map<std::string, double>`
because the address of an item inside a map is guaranteed to never
change when other items are added or if item value is changed. The map
variable used in the expression however must not be removed when an
`Expr` instance depending on them is stil alive because calling `eval`
in that case would be undefined behavior.

`Expr` can be copied but note that the copy refers to the same
variables in memory.

An `Expr` instance can be initialized with a single `double` value
and in that case is a constant expression returning that value.

Default constructor returns a constant expr evaluating to 0.0 and
an `Expr` instance can also be implicitly converted to a double
(it calls `eval`).

Partial parsing
---------------
It's also possible to parse an expression without giving an error if
extra characters are present. This is done using `Expr::parsePartial`
that requires a `const char *&` instead of a `const char *`. The
character pointer will be on the first character not used for parsing
the expression and not being on the terminating `NUL` is not
considered an error.

Functions
---------
Using `Expr::addFunction(name, f)` it's possible to add external
functions of 0, 1 or 2 parameters. Predefined functions from
`<math.h>` are: **floor**, **fabs**, **sqrt**, **sin**, **cos**,
**tan**, **atan**, **atan2**, **pow** and there is also **random**
taking no parameters and returning a number between 0 and 1
implemented as `double(rand()) / RAND_MAX`.

Syntax
------
C syntax is used; implemented operators (in order of precedence) are:

    - (unary)          (sign change)
    * /                (multiplication/division)
    + -                (addition/subtraction)

    << >>              (left and right shift)
    &                  (bitwise and)
    | ^                (bitwise or / xor)

    < <= > >= == !=    (comparison, result is 0 or 1)
    &&                 (logical and)
    ||                 (logical or)

**NOTE**: precedence is not the same as in C because C precendence for
bitwise operations is just wrong.

variables and function names are parsed with `[a-zA-Z_][a-zA-Z0-9_]*`.

Comments can be included: characters from `;` to the end of a line
are ignored during parsing.

Errors
------
Parsing errors throw an instance of `Expr::Error` (that derives from
`std::runtime_error`).

Speed
-----
`Expr` compiles the expression into bytecode for a register based
virtual machine. In the expression used in test_expr image generation:

    ((128 + sin(((x-320)*(x-320) + (y-240)*(y-240))*k)*127) ^
     (255 * ((floor(x/128)+floor(y/96)) & 1))) + random()*32-16

the evaluation is about 3 times slower than optimized C++ equivalent for
the same. Currently there are no optimizations of any kind implemented.
