#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <string.h>
#include <ctype.h>

/// Контекст выражения (определённые переменные)
typedef struct Context {
    /// Строка имён переменных
    char *variablesNames;
    /// Массив значений переменных
    int *variablesValues;
} Context;

/// Форма выражения
typedef enum Form {
    NATURAL,
    PREFIX,
    POSTFIX
} Form;

/// Тип выражения
typedef enum ExpressionKind {
    LITERAL,
    VARIABLE,
    PARENTHESIS,
    UNARY,
    BINARY
} ExpressionKind;

/// "Базовый" класс для выражения
typedef struct Expression {
    /// Тип выражения
    ExpressionKind kind;
} Expression;

/// @brief Сделать абстрактное выражение из конкретного
///
/// @details
///		Выделяет блок памяти размером sizeof(Expression) + dataSizeof.
///		Вначале идёт информация о типе выражения,
///     потом, сразу после неё - данные.
///
/// @param kind тип конкретного выражения
/// @param data данные конкретного выражения
/// @param dataSizeof размер класса конкретного выражения (sizeof(...))
///
/// @return абстрактное выражение
Expression *makeExpression(ExpressionKind kind, void *data, size_t dataSizeof) {
    if (data == NULL) { return NULL; }

    /// Выделяем больше места, чем под 1 выражение,
    //? так как нам нужно ещё сохранить данные сразу после.
    Expression *expression = (Expression *) malloc(sizeof(Expression) + dataSizeof);
    if (expression == NULL) { return NULL; }

    expression->kind = kind;
    // Копируем данные за блоком памяти выражения
    memcpy(expression + 1, data, dataSizeof);
    return expression;
}

/// Число в диапазоне [0, 2^31 - 1]
typedef struct Literal {
    /// Значение числа
    int value;
} Literal;

/// DEFINE_EXPRESSION_CAST(Class, Kind)
///		Class - класс, в который преобразуется выражение
///     Kind - тип выражения, который преобразуется в класс Class
///
///		Определяет функцию, преобразующую Expression * в Class *.
///		Преобразование производится в случае, если выражение имеет тип Kind.
///     В противном случае, ошибкаю
///
///   	Пример:
///		DEFINE_EXPRESSION_CAST(Literal, LITERAL)
///			Определяет функцию Literal * asLiteral(Expression *)
/// 		Преобразующую Expression * в Literal *,
///			если выражение имеет тип LITERAL.
#define DEFINE_EXPRESSION_CAST(Class, Kind) \
    Class *as ## Class(Expression *expr) \
    { \
        assert(expr->kind == Kind); \
        return (Class *)((char *)expr + sizeof(Expression)); \
    } \

DEFINE_EXPRESSION_CAST(Literal, LITERAL);

/// Переменная
typedef struct Variable {
    /// Имя переменной
    char name;
} Variable;

DEFINE_EXPRESSION_CAST(Variable, VARIABLE);

/// Выражение в скобках
typedef struct Parenthesis {
    /// Выражение внутри скобок
    Expression *expression;
} Parenthesis;

DEFINE_EXPRESSION_CAST(Parenthesis, PARENTHESIS);

/// Унарное выражение
typedef struct UnaryExpression {
    /// Унарный оператор
    char op;
    /// Операнд
    Expression *operand;
} UnaryExpression;

DEFINE_EXPRESSION_CAST(UnaryExpression, UNARY);

/// Бинарное выражения
typedef struct BinaryExpression {
    /// Левое выражение
    Expression *left;
    /// Бинарный оператор
    char op;
    /// Правое выражение
    Expression *right;
} BinaryExpression;

DEFINE_EXPRESSION_CAST(BinaryExpression, BINARY);

/// Рекурсивно освободить память под выражение
void freeExpression(Expression *expr) {
    if (expr == NULL) { return; }

    switch (expr->kind) {
        case LITERAL:
            break;
        case VARIABLE:
            break;
        case PARENTHESIS:
            freeExpression(asParenthesis(expr)->expression);
            break;
        case UNARY:
            freeExpression(asUnaryExpression(expr)->operand);
            break;
        case BINARY:
            freeExpression(asBinaryExpression(expr)->left);
            freeExpression(asBinaryExpression(expr)->right);
            break;
    }

    free(expr);
}

/// Forward-declaration для функции parseExpression
Expression *parseExpression(const char *input, const char **end, Form form);

/// primary-expression:
///		NUMBER | VARIABLE | '(' expression ')'
Expression *parsePrimaryExpression(
        const char *input, const char **end, Form form
) {
    assert(end);

    *end = input;

    // Проверка на пустоту
    if (input == NULL || *input == '\0') {
        return NULL;
    }

    // Переменная
    if (isalpha(*input)) {
        Variable var;
        var.name = *input;
        *end = input + 1;
        return makeExpression(VARIABLE, &var, sizeof(var));;
    }

    // Число
    if (isdigit(*input)) {
        Literal lit;
        lit.value = 0;
        do {
            lit.value = lit.value * 10 + (*input - '0');
            ++input;
        } while (isdigit(*input));
        *end = input;
        return makeExpression(LITERAL, &lit, sizeof(lit));
    }

    /// '(' expression ')'
    if (*input == '(') {
        /* Пропускаем скобку */
        ++input;

        Parenthesis paren;
        paren.expression = parseExpression(input, &input, form);

        if (paren.expression == NULL) { return NULL; }
        if (*input != ')') {
            freeExpression(paren.expression);
            return NULL;
        }

        /* Пропускаем скобку */
        ++input;
        *end = input;

        return makeExpression(PARENTHESIS, &paren, sizeof(paren));;
    }

    return NULL;
}

/// postfix-expression:
/// 	primary postfix-operator?
/// postfix-operator:
///		'!'
Expression *parsePostfixExpression(const char *input, const char **end) {
    Expression *expr = parsePrimaryExpression(input, &input, NATURAL);
    *end = input;
    if (expr == NULL) { return NULL; }

    if (*input == '!') {
        UnaryExpression unary;
        unary.op = *input;
        unary.operand = expr;
        *end = input + 1;
        return makeExpression(UNARY, &unary, sizeof(unary));
    }
    return expr;
}


/// Проверить, что сивол это оператор
bool isBinaryOperator(char symbol) {
    switch (symbol) {
        case '+':
        case '-':
        case '*':
        case '/':
        case '%':
        case '^':
            return true;
        default:
            return false;
    }
}

/// Получить приоритет оператора
int precedence(char op) {
    switch (op) {
        case '^':
            return 3;
        case '*':
        case '/':
        case '%':
            return 2;
        case '+':
        case '-':
            return 1;
        default:
            return -1;
    }
}

/// @brief
///		Парсим правую часть бинарного оператора с учётом приоритета
/// 	binary-expression-rhs:
/// 		(binary-operator postfix-expression)*
///		binary-operator:
///		 	'+' | '-' | '*' | '/' | '%' | '^'
/// @param input текст выражения
/// @param end где закончился парсинг
/// @param lhs левая часть выражения
/// @param prevOperator  предыдущий оператор оператор
Expression *parseBinaryExpressionRHS(
        const char *input,
        const char **end,
        Expression *lhs,
        char prevOperator
) {
    while (true) {
        *end = input;

        // Получаем следующий оператор
        char op = *input;
        // Текущий оператор имеет меньший или равный приоритет
        // (для левоассоциативных) -> возвращаем левую часть
        if (
                !isBinaryOperator(op) ||
                (
                        precedence(op) <= precedence(prevOperator) &&
                        // ^ - правоассоциативный оператор
                        (op != '^' || prevOperator != '^')
                )
                ) {
            return lhs;
        }

        Expression *rhs = parsePostfixExpression(input + 1, &input);
        *end = input;

        if (rhs == NULL) {
            freeExpression(lhs);
            return NULL;
        }

        // Получаем следующий оператор
        // (или не оператор, но это не важно, так как у них меньше приоритет)
        char nextOperator = *input;

        // Если следующий оператор имеет больший приоритет, то парсим его первым
        if (precedence(op) < precedence(nextOperator) ||
            // ^ - правоассоциативный оператор
            (op == '^' && nextOperator == '^')
                ) {
            rhs = parseBinaryExpressionRHS(input, &input, rhs, op);
            *end = input;
            if (rhs == NULL) {
                freeExpression(lhs);
                return NULL;
            }
        }

        BinaryExpression bin;
        bin.left = lhs;
        bin.op = op;
        bin.right = rhs;
        lhs = makeExpression(BINARY, &bin, sizeof(bin));

        *end = input;
    }
}


/// In natural form:
/// expression:
///		postfix-expression binary-expression-rhs
///
/// In prefix form:
/// expression:
///		primary | '!' '(' expression ')' |
///     binary-operator '(' expression ',' expression ')'
///
/// In postfix form:
/// expression:
///		primary | '(' expression ')' '!' |
///     '(' expression ',' expression ')' binary-operator
Expression *parseExpression(const char *input, const char **end, Form form) {
    switch (form) {
        case NATURAL: {
            Expression *lhs = parsePostfixExpression(input, &input);
            *end = input;
            if (lhs == NULL) { return NULL; }

            return parseBinaryExpressionRHS(input, end, lhs, 0);
        }
        case PREFIX: {
            *end = input;

            if (*input == '!') {
                Expression *expr = parseExpression(input + 2, &input, form);
                *end = input;
                if (expr == NULL) { return NULL; }

                /* Пропускаем ')' */
                ++input;
                *end = input;

                UnaryExpression unary;
                unary.op = '!';
                unary.operand = expr;
                return makeExpression(UNARY, &unary, sizeof(unary));
            } else if (isBinaryOperator(*input)) {
                char op = *input;

                Expression *lhs = parseExpression(
                        input + 2, // пропускаем оператор и скобку
                        &input,
                        form
                );
                *end = input;
                if (lhs == NULL) { return NULL; }

                Expression *rhs = parseExpression(
                        input + 1, // пропускаем запятую
                        &input,
                        form
                );
                *end = input;
                if (rhs == NULL) {
                    freeExpression(lhs);
                    return NULL;
                }

                /* Пропускаем ')' */
                ++input;
                *end = input;

                BinaryExpression bin;
                bin.left = lhs;
                bin.op = op;
                bin.right = rhs;
                return makeExpression(BINARY, &bin, sizeof(bin));
            }
            return parsePrimaryExpression(input, end, form);
        }
        case POSTFIX: {
            *end = input;


            if (*input != '(') {
                return parsePrimaryExpression(input, end, form);
            }

            Expression *expr = parseExpression(
                    input + 1, // пропускаем '('
                    &input,
                    form
            );
            *end = input;
            if (expr == NULL) { return NULL; }

            if (*input == ')') {
                /* Пропускаем ')' */
                ++input;
                *end = input;

                if (*input == '!') {
                    UnaryExpression unary;
                    unary.op = '!';
                    unary.operand = expr;

                    /* Пропускаем '!' */
                    ++input;
                    *end = input;

                    return makeExpression(UNARY, &unary, sizeof(unary));
                }

                if (*input == '\0' || *input == ')' || *input == ',') {
                    Parenthesis paren;
                    paren.expression = expr;
                    return makeExpression(PARENTHESIS, &paren, sizeof(paren));
                }

                freeExpression(expr);
                return NULL;
            }

            if (*input == ',') {
                /* Пропускаем ',' */
                ++input;
                *end = input;

                Expression *rhs = parseExpression(input, &input, form);
                *end = input;
                if (rhs == NULL) {
                    freeExpression(expr);
                    return NULL;
                }

                if (*input == ')') {
                    /* Пропускаем ')' */
                    ++input;
                    *end = input;

                    if (isBinaryOperator(*input)) {
                        BinaryExpression bin;
                        bin.left = expr;
                        bin.op = *input;
                        bin.right = rhs;

                        /* Пропускаем оператор */
                        ++input;
                        *end = input;

                        return makeExpression(BINARY, &bin, sizeof(bin));
                    }

                    freeExpression(rhs);
                    return NULL;
                }

                freeExpression(expr);
                freeExpression(rhs);
                return NULL;
            }

            freeExpression(expr);
            return NULL;
        }
    };
}

/// Подсчитать факториал числа
int factorial(int x) {
    if (x <= 1) { return 1; }
    int res = 2;
    for (int i = 3; i <= x; i++) {
        res *= i;
    }
    return res;
}

/// @brief Подсчитать значение выражения
/// @param expr выражение
/// @param context контекст выражения (значения переменных)
/// @return
int evaluate(Expression *expr, const Context *context) {
    assert(expr);

    switch (expr->kind) {
        case LITERAL:
            return asLiteral(expr)->value;
        case VARIABLE: {
            const Variable *var = asVariable(expr);
            // Ищем индекс переменной в строке
            char *location = strchr(context->variablesNames, var->name);
            assert(location);

            long long index = location - context->variablesNames;

            return context->variablesValues[index];
        }
        case PARENTHESIS:
            return evaluate(asParenthesis(expr)->expression, context);
        case UNARY: {
            UnaryExpression *unary = asUnaryExpression(expr);
            int operand = evaluate(unary->operand, context);
            switch (unary->op) {
                case '!':
                    return factorial(operand);
                default:
                    assert(false && "unknown operator");
            }
            break;
        }
        case BINARY: {
            BinaryExpression *binary = asBinaryExpression(expr);
            int left = evaluate(binary->left, context);
            int right = evaluate(binary->right, context);
            switch (binary->op) {
                case '+':
                    return left + right;
                case '-':
                    return left - right;
                case '*':
                    return left * right;
                case '/':
                    assert(right != 0);
                    return left / right;
                case '%':
                    return left % right;
                case '^':
                    /// Возведение в степень
                    return (int) pow(left, right);
                default:
                    assert(false && "unknown operator");
            }
        }
    };
}

/// @brief Печать выражения в префиксной или постфиксной форме
void printExpression(FILE *file, Expression *expr, Form form) {
    if (expr == NULL) { return; }

    assert(form != NATURAL);

    switch (expr->kind) {
        case LITERAL:
            fprintf(file, "%d", asLiteral(expr)->value);
            break;
        case VARIABLE:
            fprintf(file, "%c", asVariable(expr)->name);
            break;
        case PARENTHESIS:
            fprintf(file, "(");
            printExpression(file, asParenthesis(expr)->expression, form);
            fprintf(file, ")");
            break;
        case UNARY: {
            UnaryExpression *unary = asUnaryExpression(expr);
            fprintf(file, form == PREFIX ? "%c(" : "(", unary->op);
            printExpression(file, unary->operand, form);
            fprintf(file, form == POSTFIX ? ")%c" : ")", unary->op);
            break;
        }
        case BINARY: {
            BinaryExpression *binary = asBinaryExpression(expr);
            fprintf(file, form == PREFIX ? "%c(" : "(", binary->op);
            printExpression(file, binary->left, form);
            fprintf(file, ",");
            printExpression(file, binary->right, form);
            fprintf(file, form == POSTFIX ? ")%c" : ")", binary->op);
            break;
        }
    };
}


/// @brief Доступные комманды
typedef enum Command {
    INVALID, // Неверная команда

    PARSE, // Разбор выражения, принимает аргументом выражение
    LOAD_PRF, // Загрузка выражение в префиксной форме
    LOAD_PST, // Загрузка выражение в постфиксной форме
    SAVE_PRF, // Сохранение выражения в префиксной форме
    SAVE_PST, // Сохранение выражения в постфиксной форме
    EVALUATE  // Вычисление выражения
} Command;

/// Разбор команды
Command parseCommand(const char *command) {
    if (strcmp(command, "parse") == 0) {
        return PARSE;
    }
    if (strcmp(command, "load_prf") == 0) {
        return LOAD_PRF;
    }
    if (strcmp(command, "load_pst") == 0) {
        return LOAD_PST;
    }
    if (strcmp(command, "save_prf") == 0) {
        return SAVE_PRF;
    }
    if (strcmp(command, "save_pst") == 0) {
        return SAVE_PST;
    }
    if (strcmp(command, "evaluate") == 0) {
        return EVALUATE;
    }

    return INVALID;
}

/// @brief Загрузка выражения в указанной форме и вывод результата в файл
void load_expression(Expression **expr, Form form, FILE *out) {
    assert(expr);

    // Освободить память от предыдущего выражения
    if (*expr) {
        freeExpression(*expr);
    }

    // Разбор выражения
    char *expression = strtok(
            NULL, // NULL - продолжить разбор предыдущей строки
            " "
    );
    const char *end = NULL;
    *expr = parseExpression(expression, &end, form);
    if (expr == NULL) {
        fprintf(out, "incorrect\n");
    } else {
        fprintf(out, "success\n");
    }
}

/// Обработать строку с коммандок
void processLine(char *line, Expression **expr, FILE *out) {
    assert(expr);

    // Разбить строку по пробелам
    char *command = strtok(line, " ");
    Command cmd = parseCommand(command);
    switch (cmd) {
        case PARSE: {
            load_expression(expr, NATURAL, out);
            return;
        }
        case LOAD_PRF: {
            load_expression(expr, PREFIX, out);
            return;
        }
        case LOAD_PST: {
            load_expression(expr, POSTFIX, out);
            return;
        }
        case SAVE_PRF: {
            if (expr == NULL) {
                fprintf(out, "not_loaded\n");
                return;
            }

            printExpression(out, *expr, PREFIX);
            fprintf(out, "\n");
            return;
        }
        case SAVE_PST: {
            if (expr == NULL) {
                fprintf(out, "not_loaded\n");
                return;
            }

            printExpression(out, *expr, POSTFIX);
            fprintf(out, "\n");
            return;
        }
        case EVALUATE: {
            if (expr == NULL) {
                fprintf(out, "not_loaded\n");
                return;
            }

            Context context;
            context.variablesNames = (char *) malloc(1);
            context.variablesNames[0] = '\0';
            context.variablesValues = (int *) malloc(0);

            char *var = strtok(NULL, " ,");
            while (var) {
                char name = var[0];
                int value = atoi(var + 2);

                size_t varsCount = strlen(context.variablesNames);
                char *newNames = (char *) realloc(
                        context.variablesNames,
                        varsCount + 2
                );
                assert(newNames != NULL);
                context.variablesNames = newNames;
                context.variablesNames[varsCount] = name;
                context.variablesNames[varsCount + 1] = '\0';

                int *newValues = (int *) realloc(
                        context.variablesValues,
                        (varsCount + 1) * sizeof(int)
                );
                assert(newValues != NULL);
                context.variablesValues = newValues;
                context.variablesValues[varsCount] = value;

                var = strtok(NULL, " ,");
            }

            fprintf(out, "%d\n", evaluate(*expr, &context));

            free(context.variablesNames);
            free(context.variablesValues);
            return;
        }
        case INVALID: {
            fprintf(out, "incorrect\n");
            return;
        }
    };
}

int main(int argc, const char *argv[]) {
    Expression *expr = NULL;
    if (argc == 1) {
        FILE* in = fopen("input.txt", "r");
        if (in == NULL) {
            perror("can't open file input.txt");
            return 1;
        }

        FILE* out = fopen("output.txt", "w");
        if (out == NULL) {
            perror("can't open file output.txt");
            return 1;
        }
        char line[256] = {0};
        while (fgets(line, sizeof(line), in)) {
            size_t len = strlen(line);
            // Удалить символ новой строки
            if (len > 0 && line[len - 1] == '\n') {
                line[len - 1] = '\0';
            }

            processLine(line, &expr, out);
        };
    } else {
        for (int i = 1; i < argc; ++i) {
            char line[256] = {0};
            strcpy(line, argv[i]);
            processLine(line, &expr, stdout);
        }
    }
    return 0;
}