#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>
#include <math.h>
#include <windows.h>

#define INPUT_BUFFER 500
char ch[INPUT_BUFFER];
const char *pos;
bool shouldClose = false;
double lastResult = NAN;

// ANSI codes
#define reset "\033[0m"
#define bold "\033[1m"
#define inverse "\033[7m"
#define BLACK "\033[30m"
#define RED "\033[31m"
#define GREEN "\033[32m"
#define YELLOW "\033[33m"
#define BLUE "\033[34m"
#define PURPLE "\033[35m"
#define CYAN "\033[36m"

void skipSpaces() { while (isspace(*pos)) pos++; }
double parseFactor();
double parseTerm();
double parseExpr();
double parseCompare();
double calculate(const char *input);
void readKey(char *input);
void printHelp();
void boot();

typedef enum {
    CMD_UNKNOWN = -1,
    CMD_EXIT,
    CMD_HELP,
} Command;
Command parseCommand(const char *input);

typedef struct {
    const char *name;
    Command cmd;
} CommandEntry;

CommandEntry commands[] = {
    {"exit", CMD_EXIT},
    {"help", CMD_HELP},
};
int commandCount = sizeof(commands) / sizeof(commands[0]);

typedef struct {
    const char *name;
    double (*func1)(double);
    double (*func2)(double, double);
} FuncEntry;

#define PI   3.14159265358979323846
#define TAU  (2 * PI)
#define E    2.71828182845904523536
#define PHI  1.61803398874989484820
#define G    9.80665
#define C    299792458.0
double torad(double deg) { return deg * (PI / 180.0); }
double todeg(double rad) { return rad * (180.0 / PI); }
double sind(double x)  { return sin(torad(x)); }
double cosd(double x)  { return cos(torad(x)); }
double tand(double x)  { return tan(torad(x)); }
double asind(double x) { return todeg(asin(x)); }
double acosd(double x) { return todeg(acos(x)); }
double atand(double x) { return todeg(atan(x)); }
double sign(double x)  { return (x > 0) - (x < 0); }
double fact(double x)   { 
    if (x < 0) return NAN;
    int n = (int)x;
    double r = 1;
    for (int i = 2; i <= n; i++) r *= i;
    return r;
}
double logn(double x, double y) { return log(x)/log(y);}
double dmin(double x, double y) { return x < y ? x : y; }
double dmax(double x, double y) { return x > y ? x : y; }
FuncEntry funcs[] = {
    { "torad", torad, NULL  },
    { "todeg", todeg, NULL  },
    { "sin",   sin,   NULL  },
    { "cos",   cos,   NULL  },
    { "tan",   tan,   NULL  },
    { "sind",  sind,  NULL  },
    { "cosd",  cosd,  NULL  },
    { "tand",  tand,  NULL  },
    { "asin",  asin,  NULL  },
    { "acos",  acos,  NULL  },
    { "atan",  atan,  NULL  },
    { "asind", asind, NULL  },
    { "acosd", acosd, NULL  },
    { "atand", atand, NULL  },
    { "sinh",  sinh,  NULL  },
    { "cosh",  cosh,  NULL  },
    { "tanh",  tanh,  NULL  },
    { "sqrt",  sqrt,  NULL  },
    { "exp",   exp,   NULL  },
    { "log",   log,   NULL  },
    { "log2",  log2,  NULL  },
    { "log10", log10, NULL  },
    { "logn",  NULL,  logn  },
    { "abs",   fabs,  NULL  },
    { "round", round, NULL  },
    { "trunc", trunc, NULL  },
    { "ceil",  ceil,  NULL  },
    { "floor", floor, NULL  },
    { "pow",   NULL,  pow   },
    { "hypot", NULL,  hypot },
    { "fmod",  NULL,  fmod  },
    { "sign",  sign,  NULL  },
    { "fact",  fact,  NULL  },
    { "min",   NULL,  dmin  },
    { "max",   NULL,  dmax  },
};
int funcCount = sizeof(funcs) / sizeof(funcs[0]);

int main()
{
    SetConsoleOutputCP(CP_UTF8); // for UTF_8 support
    boot();
    while (!shouldClose)
    {
        printf(inverse bold BLUE " > " reset " ");
        fflush(stdout);

        readKey(ch);
        if (shouldClose)
            break;

        switch (parseCommand(ch))
        {
            case CMD_EXIT: shouldClose = true; break;
            case CMD_HELP: printHelp(); break;
            case CMD_UNKNOWN:
                double result = calculate(ch);
                if (!isnan(result))
                {
                    lastResult = result;
                    printf(bold YELLOW "┌──┘" "\n");
                    printf("│ = %.18g\n", result);
                    printf("└──┐" "\n" reset);
                }
        }
    }

    printf(inverse bold GREEN " ! " reset " Calculator closed\n");
    return 0;
}

double parseFactor()
{
    skipSpaces();

    if (*pos == '\0')
    {
        fprintf(stderr, inverse RED " ! " reset " unexpected end of input\n");
        return NAN;
    }

    if (*pos == '-') { pos++; return -parseFactor(); }

    if (isalpha(*pos))
    {
        char name[32];
        int i = 0;
        while (isalpha(*pos) || isdigit(*pos))
            name[i++] = *pos++;
        name[i] = '\0';

        skipSpaces();
        if (strcmp(name, "pi")  == 0) return PI;
        if (strcmp(name, "tau") == 0) return TAU;
        if (strcmp(name, "e")   == 0) return E;
        if (strcmp(name, "inf") == 0) return INFINITY;
        if (strcmp(name, "phi") == 0) return PHI;
        if (strcmp(name, "g")   == 0) return G;
        if (strcmp(name, "c")   == 0) return C;
        if (strcmp(name, "ans") == 0) return lastResult;

        if (*pos == '(')
        {
            pos++;
            double arg1 = parseExpr();
            double arg2 = NAN;
            if (*pos == ',') {
                pos++;
                arg2 = parseExpr();
            }
            skipSpaces();
            if (*pos == ')') pos++;

            for (int i = 0; i < funcCount; i++)
            {
                if (strcmp(name, funcs[i].name) == 0)
                {
                    bool hasTwo = !isnan(arg2);
                    if (hasTwo && funcs[i].func2 != NULL)
                        return funcs[i].func2(arg1, arg2);
                    else if (!hasTwo && funcs[i].func1 != NULL)
                        return funcs[i].func1(arg1);
                    else if (hasTwo && funcs[i].func2 == NULL) {
                        fprintf(stderr, inverse RED " ! " reset " '%s' takes 1 argument, got 2\n", name);
                        return NAN;
                    } else {
                        fprintf(stderr, inverse RED " ! " reset " '%s' takes 2 arguments, got 1\n", name);
                        return NAN;
                    }
                }
            }

            fprintf(stderr, inverse RED " ! " reset " unknown function: '%s'\n", name);
            return NAN;
        }

        fprintf(stderr, inverse RED " ! " reset " unknown identifier: '%s'\n", name);
        return NAN;
    }

    double base;

    if (*pos == '(')
    {
        pos++;
        base = parseExpr();
        skipSpaces();
        if (*pos == ')') pos++;
    }
    else
    {
        char *end;
        base = strtod(pos, &end);
        if (end == pos)
        {
            fprintf(stderr, inverse RED " ! " reset " unexpected character: '%c'\n", *pos);
            pos++;
            return NAN;
        }
        pos = end;
    }

    // handle ^ here so unary minus wraps the whole thing
    skipSpaces();
    if (*pos == '^') {
        pos++;
        return pow(base, parseFactor()); // right-associative
    }

    return base;
}

double parseTerm()
{
    double left = parseFactor();

    while (1)
    {
        skipSpaces();
        
        if      (*pos == '*') { pos++; left *= parseFactor(); }
        else if (*pos == '/')
        {
            pos++;
            double right = parseFactor();
            if (right == 0.0)
            {
                fprintf(stderr, inverse RED " ! " reset " division by zero\n");
                return NAN;
            }
            left /= right;
        }
        else if (*pos == '%')
        {
            pos++;
            double right = parseFactor();
            if (right == 0.0) {
                fprintf(stderr, inverse RED " ! " reset " modulus by zero\n");
                return NAN;
            }
            left = fmod(left, right);
        }
        else break;
    }

    return left;
}

double parseExpr()
{
    double left = parseTerm();

    while (1)
    {
        skipSpaces();
        if (*pos == '+')
        {
            pos++;
            left += parseTerm();
        }
        else if (*pos == '-')
        {
            pos++;
            left -= parseTerm();
        }
        else break;
    }

    return left;
}

double parseCompare() {
    double left = parseExpr();

    skipSpaces();
    if (*pos == '!' && *(pos+1) != '=') { pos++;       return left == parseExpr() ? 0 : 1; }
    if (*pos == '=' && *(pos+1) != '=') { pos++;       return left == parseExpr() ? 1 : 0; }
    if (*pos == '=' && *(pos+1) == '=') { pos += 2;    return left == parseExpr() ? 1 : 0; }
    if (*pos == '<' && *(pos+1) == '=') { pos += 2;    return left <= parseExpr() ? 1 : 0; }
    if (*pos == '>' && *(pos+1) == '=') { pos += 2;    return left >= parseExpr() ? 1 : 0; }
    if (*pos == '<')                    { pos++;       return left <  parseExpr() ? 1 : 0; }
    if (*pos == '>')                    { pos++;       return left >  parseExpr() ? 1 : 0; }

    return left;
}

double calculate(const char *input)
{
    pos = input; 
    return parseCompare();
}

Command parseCommand(const char *input)
{
    for (int i = 0; i < commandCount; i++)
    {
        if (strcasecmp(input, commands[i].name) == 0)
            return commands[i].cmd;
    }
    return CMD_UNKNOWN;
}

void readKey(char *input)
{
    if (fgets(ch, INPUT_BUFFER, stdin) == NULL)
    {
        // EOF (e.g. Ctrl+D)
        shouldClose = true;
        return;
    }
    input[strcspn(input, "\n")] = '\0'; // strip trailing newline
}

#define PLINE(...) do { printf(__VA_ARGS__); fflush(stdout); Sleep(10); } while(0)

void printHelp()
{
    PLINE(inverse bold YELLOW "\n List of commands:             \n" reset);

    PLINE(inverse bold YELLOW " Basic operations:             \n" reset);
    PLINE(bold YELLOW " +           " "│" reset " Addition       " bold YELLOW "│" reset " Calculate the sum.\n");
    PLINE(bold YELLOW " -           " "│" reset " Subtraction    " bold YELLOW "│" reset " Calculate the difference.\n");
    PLINE(bold YELLOW " *           " "│" reset " Multiplication " bold YELLOW "│" reset " Calculate the product.\n");
    PLINE(bold YELLOW " /           " "│" reset " Division       " bold YELLOW "│" reset " Calculate the quotient.\n");
    PLINE(bold YELLOW " ^           " "│" reset " Exponentiation " bold YELLOW "│" reset " x raised to the power of n.\n");
    PLINE(bold YELLOW " %%           " "│" reset " Modulus        " bold YELLOW "│" reset " Remainder after dividing two numbers.\n");
    PLINE(bold YELLOW " ( )         " "│" reset " Grouping       " bold YELLOW "│" reset " Control order of operations.\n");

    PLINE(inverse bold YELLOW " Comparison:                   \n" reset);
    PLINE(bold YELLOW " =           " "│" reset " Equal          " bold YELLOW "│" reset " Check if two values are equal.\n");
    PLINE(bold YELLOW " !=          " "│" reset " Not Equal      " bold YELLOW "│" reset " Check if two values are not equal.\n");
    PLINE(bold YELLOW " <           " "│" reset " Less Than      " bold YELLOW "│" reset " Check if left is less than right.\n");
    PLINE(bold YELLOW " >           " "│" reset " Greater Than   " bold YELLOW "│" reset " Check if left is greater than right.\n");
    PLINE(bold YELLOW " <=          " "│" reset " Less or Equal  " bold YELLOW "│" reset " Check if left is less or equal.\n");
    PLINE(bold YELLOW " >=          " "│" reset " Greater/Equal  " bold YELLOW "│" reset " Check if left is greater or equal.\n");

    PLINE(inverse bold PURPLE " Trig (radians):               \n" reset);
    PLINE(bold PURPLE " sin(x)      " "│" reset " Sine           " bold PURPLE "│" reset " Ratio of opposite to hypotenuse.\n");
    PLINE(bold PURPLE " cos(x)      " "│" reset " Cosine         " bold PURPLE "│" reset " Ratio of adjacent to hypotenuse.\n");
    PLINE(bold PURPLE " tan(x)      " "│" reset " Tangent        " bold PURPLE "│" reset " Ratio of opposite to adjacent.\n");
    PLINE(bold PURPLE " asin(x)     " "│" reset " Arc Sine       " bold PURPLE "│" reset " Inverse of sin, returns angle in radians.\n");
    PLINE(bold PURPLE " acos(x)     " "│" reset " Arc Cosine     " bold PURPLE "│" reset " Inverse of cos, returns angle in radians.\n");
    PLINE(bold PURPLE " atan(x)     " "│" reset " Arc Tangent    " bold PURPLE "│" reset " Inverse of tan, returns angle in radians.\n");

    PLINE(inverse bold CYAN " Trig (degrees):               \n" reset);
    PLINE(bold CYAN " sind(x)     " "│" reset " Sine           " bold CYAN "│" reset " sin with input in degrees.\n");
    PLINE(bold CYAN " cosd(x)     " "│" reset " Cosine         " bold CYAN "│" reset " cos with input in degrees.\n");
    PLINE(bold CYAN " tand(x)     " "│" reset " Tangent        " bold CYAN "│" reset " tan with input in degrees.\n");
    PLINE(bold CYAN " asind(x)    " "│" reset " Arc Sine       " bold CYAN "│" reset " Inverse of sin, returns angle in degrees.\n");
    PLINE(bold CYAN " acosd(x)    " "│" reset " Arc Cosine     " bold CYAN "│" reset " Inverse of cos, returns angle in degrees.\n");
    PLINE(bold CYAN " atand(x)    " "│" reset " Arc Tangent    " bold CYAN "│" reset " Inverse of tan, returns angle in degrees.\n");
    PLINE(bold CYAN " torad(x)    " "│" reset " To Radians     " bold CYAN "│" reset " Convert degrees to radians.\n");
    PLINE(bold CYAN " todeg(x)    " "│" reset " To Degrees     " bold CYAN "│" reset " Convert radians to degrees.\n");

    PLINE(inverse bold GREEN " Hyperbolic:                   \n" reset);
    PLINE(bold GREEN " sinh(x)     " "│" reset " Hyp. Sine      " bold GREEN "│" reset " Hyperbolic sine of x.\n");
    PLINE(bold GREEN " cosh(x)     " "│" reset " Hyp. Cosine    " bold GREEN "│" reset " Hyperbolic cosine of x.\n");
    PLINE(bold GREEN " tanh(x)     " "│" reset " Hyp. Tangent   " bold GREEN "│" reset " Hyperbolic tangent of x.\n");

    PLINE(inverse bold PURPLE " Other functions:              \n" reset);
    PLINE(bold PURPLE " sqrt(x)     " "│" reset " Square Root    " bold PURPLE "│" reset " Value multiplied by itself to get x.\n");
    PLINE(bold PURPLE " exp(x)      " "│" reset " Exponential    " bold PURPLE "│" reset " e raised to the power of x.\n");
    PLINE(bold PURPLE " log(x)      " "│" reset " Logarithm      " bold PURPLE "│" reset " Power to which e must be raised to get x.\n");
    PLINE(bold PURPLE " log2(x)     " "│" reset " Log Base 2     " bold PURPLE "│" reset " Power to which 2 must be raised to get x.\n");
    PLINE(bold PURPLE " log10(x)    " "│" reset " Log Base 10    " bold PURPLE "│" reset " Power to which 10 must be raised to get x.\n");
    PLINE(bold PURPLE " logn(x,y)   " "│" reset " Log Base N     " bold PURPLE "│" reset " Log of x in any base y.\n");
    PLINE(bold PURPLE " abs(x)      " "│" reset " Absolute Value " bold PURPLE "│" reset " Distance from zero, always positive.\n");
    PLINE(bold PURPLE " sign(x)     " "│" reset " Sign           " bold PURPLE "│" reset " Returns -1, 0, or 1 based on sign of x.\n");
    PLINE(bold PURPLE " fact(x)     " "│" reset " Factorial      " bold PURPLE "│" reset " Product of all integers from 1 to x.\n");
    PLINE(bold PURPLE " round(x)    " "│" reset " Round          " bold PURPLE "│" reset " Round to nearest integer.\n");
    PLINE(bold PURPLE " trunc(x)    " "│" reset " Truncate       " bold PURPLE "│" reset " Drop the decimal part toward zero.\n");
    PLINE(bold PURPLE " ceil(x)     " "│" reset " Ceiling        " bold PURPLE "│" reset " Round up to nearest integer.\n");
    PLINE(bold PURPLE " floor(x)    " "│" reset " Floor          " bold PURPLE "│" reset " Round down to nearest integer.\n");
    PLINE(bold PURPLE " pow(x,y)    " "│" reset " Power          " bold PURPLE "│" reset " x raised to the power of y.\n");
    PLINE(bold PURPLE " hypot(x,y)  " "│" reset " Hypotenuse     " bold PURPLE "│" reset " sqrt(x² + y²).\n");
    PLINE(bold PURPLE " fmod(x,y)   " "│" reset " Float Modulus  " bold PURPLE "│" reset " Remainder of x/y for decimals.\n");
    PLINE(bold PURPLE " min(x,y)    " "│" reset " Minimum        " bold PURPLE "│" reset " Returns the smaller of two values.\n");
    PLINE(bold PURPLE " max(x,y)    " "│" reset " Maximum        " bold PURPLE "│" reset " Returns the larger of two values.\n");

    PLINE(inverse bold RED " Constants:                    \n" reset);
    PLINE(bold RED " pi          " "│" reset " Pi             " bold RED "│" reset " 3.14159265358979323846...\n");
    PLINE(bold RED " tau         " "│" reset " Tau            " bold RED "│" reset " 6.28318530717958647692... (2π)\n");
    PLINE(bold RED " e           " "│" reset " Euler's number " bold RED "│" reset " 2.71828182845904523536...\n");
    PLINE(bold RED " phi         " "│" reset " Golden Ratio   " bold RED "│" reset " 1.61803398874989484820...\n");
    PLINE(bold RED " inf         " "│" reset " Infinity       " bold RED "│" reset " A value larger than any number.\n");
    PLINE(bold RED " g           " "│" reset " Gravity        " bold RED "│" reset " 9.80665 m/s²\n");
    PLINE(bold RED " c           " "│" reset " Speed of Light " bold RED "│" reset " 299792458 m/s\n");
    PLINE(bold RED " ans         " "│" reset " Last Answer    " bold RED "│" reset " Result of the previous calculation.\n");

    PLINE("\n");
}

void boot()
{
    const char *title1 = " Calculator.c ";
    const char *title2 = " Calculator.c      version 1.0";
    int len1 = strlen(title1);
    int len2 = strlen(title2);

    printf("\n");
    for (int i = 0; i < len1; i++) {
        printf(bold "%c" reset, title1[i]);
        fflush(stdout);
        Sleep(20);
    }

    printf("\r");
    fflush(stdout);

    for (int i = 0; i < len2; i++) {
        if (i < 14)
            printf(inverse bold YELLOW "%c" reset, title2[i]);
        else 
            printf(PURPLE "%c" reset, title2[i]);
        fflush(stdout);
        Sleep(20);
    }

    Sleep(70);

    printf("\n");
    printf("Type " YELLOW "exit" reset " to close the program\n");
    Sleep(20);
    printf("Type " YELLOW "help" reset " to see a list of commands\n\n");
    Sleep(20);
}