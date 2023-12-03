#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// 参数设置
// 最大支持的次幂数
const int MAX_POWER_COUNT = 255;
// 二分法求根的二分最小阈值
const long double BINARY_SEARCH_ROOT_THRESHOLD = 1e-6;
// 二分法求根的左右最大最小端点值
const long double BINARY_SEARCH_ROOT_MIN = -1e9;
const long double BINARY_SEARCH_ROOT_MAX = 1e9;
// 是否输出调试信息
int debug = 0;

// long double的绝对值函数
long double lfabs(long double x) {
    if (x < 0) {
        return -x;
    }
    return x;
}


// 下面为元素相关的结构体和函数(元素代表字符串解析后的每个值，可以是常量/变量/操作符/等号)
// 定义元素类型枚举
typedef enum {
    NUMBER,     // 数字
    OPERATOR,   // 操作符
    VARIABLE,   // 未知数x
    EQUALS     // 等号
} ElementType;

// 定义元素结构体(元素代表字符串解析后的每个值，可以是常量/变量/操作符/等号)
typedef struct {
    ElementType type;
    long double value;
} Element;

// 释放元素数组内存
void freeElements(Element *elements) {
    free(elements);
}

// 输出元素用于debug的工具函数
void elementPrint(const Element *element) {
    switch (element->type) {
        case NUMBER:
            printf("%Lf ", element->value);
            break;
        case OPERATOR:
        case VARIABLE:
        case EQUALS:
            printf("%c ", (char) element->value);
            break;
    }
}

// 判断字符是否为数字
int isNumeric(const char ch) {
    return (ch >= '0' && ch <= '9') || ch == '.';
}

// 解析字符串并返回对应的元素数组(忽略空格，处理整数或小数并记录为NUMBER类型元素，变量x记为VARIABLE元素，操作符和等号分别也记录为对应元素)
Element *parseString(const char *input, int *numElements, char *errorElement) {
    int len = (int) strlen(input);
    Element *elements = (Element *) malloc(len * sizeof(Element));
    *numElements = 0;

    int i = 0;
    while (i < len) {
        if (input[i] == ' ') {
            i++; // 忽略空格
            continue;
        }

        // 处理数字
        if (isNumeric(input[i])) {
            int start = i;
            int pointOccurred = 0;
            while (isNumeric(input[i])) {
                i++;
                if (input[i] == '.') {
                    pointOccurred++;
                }
            }
            int length = i - start;
            char number[length + 1];
            strncpy(number, &input[start], length);
            number[length] = '\0';
            if (pointOccurred > 1) {
                strcpy(errorElement, number);
                return NULL;
            }
            elements[*numElements].type = NUMBER;
            elements[*numElements].value = atof(number);
            (*numElements)++;
            continue;
        }

        // 处理操作符和其他特殊字符
        elements[*numElements].value = (long double) input[i];

        switch (input[i]) {
            case '+':
            case '-':
            case '*':
            case '/':
            case '%':
            case '^':
            case '(':
            case ')':
                elements[*numElements].type = OPERATOR;
                break;
            case 'x':
                elements[*numElements].type = VARIABLE;
                break;
            case '=':
                elements[*numElements].type = EQUALS;
                break;
            default:
                strcpy(errorElement, " ");
                errorElement[0] = (char) elements[*numElements].value;
                return NULL;
//                elements[*numElements].type = UNKNOWN;
//                break;
        }

        (*numElements)++;
        i++;
    }

    return elements;
}

// 判断元素数组的正确性(判断是否括号不匹配，判断是否有连续的操作符，判断是否有连续的数字或变量，首尾不能为操作符，判断是否漏写乘号等)
int checkElements(const Element *elements, const int numElements, char *error) {
    // 括号匹配
    int numUnmatchedLeftBrackets = 0;
    for (int i = 0; i < numElements; i++) {
        if (elements[i].type == OPERATOR && elements[i].value == '(') {
            numUnmatchedLeftBrackets++;
        } else if (elements[i].type == OPERATOR && elements[i].value == ')') {
            numUnmatchedLeftBrackets--;
        }
        if (numUnmatchedLeftBrackets < 0) {
            strcpy(error, "unmatched right bracket");
            return 0;
        }
    }
    if (numUnmatchedLeftBrackets > 0) {
        strcpy(error, "unmatched left bracket");
        return 0;
    }

    // 左右括号不能连续
    for (int i = 0; i < numElements - 1; i++) {
        if (elements[i].type == OPERATOR && elements[i].value == '(' &&
            elements[i + 1].type == OPERATOR && elements[i + 1].value == ')') {
            strcpy(error, "left bracket followed by right bracket");
            return 0;
        }
        if (elements[i].type == OPERATOR && elements[i].value == ')' &&
            elements[i + 1].type == OPERATOR && elements[i + 1].value == '(') {
            strcpy(error, "right bracket followed by left bracket, please use '*'");
            return 0;
        }
    }

    // 连续操作符(左括号可以左连操作符，右括号可以右连)
    for (int i = 0; i < numElements - 1; i++) {
        if (elements[i].type == OPERATOR && elements[i + 1].type == OPERATOR) {
            if (elements[i + 1].value != '(' && elements[i].value != ')') {
                strcpy(error, "continuous operators");
                return 0;
            }
        }
    }

    // 数字或变量不能右边是左括号，不能左边是右括号
    for (int i = 0; i < numElements - 1; i++) {
        if ((elements[i].type == NUMBER || elements[i].type == VARIABLE) &&
            elements[i + 1].type == OPERATOR && elements[i + 1].value == '(') {
            strcpy(error, "number or variable followed by left bracket, please use '*'");
            return 0;
        }
        if (elements[i].type == OPERATOR && elements[i].value == ')' &&
            (elements[i + 1].type == NUMBER || elements[i + 1].type == VARIABLE)) {
            strcpy(error, "right bracket followed by number or variable, please use '*'");
            return 0;
        }
    }

    // 连续两个元素都是数字或变量
    for (int i = 0; i < numElements - 1; i++) {
        if ((elements[i].type == NUMBER || elements[i].type == VARIABLE) &&
            (elements[i + 1].type == NUMBER || elements[i + 1].type == VARIABLE)) {
            strcpy(error, "continuous numbers or variables");
            return 0;
        }
    }

    // 首尾不能为操作符
    if (elements[0].type == OPERATOR && elements[0].value != '(') {
        strcpy(error, "operator at the beginning");
        return 0;
    }
    if (elements[numElements - 1].type == OPERATOR && elements[numElements - 1].value != ')') {
        strcpy(error, "operator at the end");
        return 0;
    }

    return 1;
}

// 找到等号的位置
int findEquals(const Element *elements, const int numElements) {
    for (int i = 0; i < numElements; i++) {
        if (elements[i].type == EQUALS) {
            return i;
        }
    }
    return -1;
}

// 等式转换，将等式转换为表达式的元素列表( E1=E2 -> E1-(E2)=0 )
Element *transformEquals(const Element *elements, const int numElements, int *numElementsTransformed) {
    int equalsIndex = findEquals(elements, numElements);
    if (equalsIndex == -1) {
        return NULL;
    }
    Element *elementsTransformed = (Element *) malloc((numElements + 3 - 1) * sizeof(Element));
    *numElementsTransformed = 0;
    for (int i = 0; i < equalsIndex; i++) {
        elementsTransformed[*numElementsTransformed] = elements[i];
        (*numElementsTransformed)++;
    }
    elementsTransformed[*numElementsTransformed].type = OPERATOR;
    elementsTransformed[*numElementsTransformed].value = '-';
    (*numElementsTransformed)++;
    elementsTransformed[*numElementsTransformed].type = OPERATOR;
    elementsTransformed[*numElementsTransformed].value = '(';
    (*numElementsTransformed)++;
    for (int i = equalsIndex + 1; i < numElements; i++) {
        elementsTransformed[*numElementsTransformed] = elements[i];
        (*numElementsTransformed)++;
    }
    elementsTransformed[*numElementsTransformed].type = OPERATOR;
    elementsTransformed[*numElementsTransformed].value = ')';
    (*numElementsTransformed)++;
    return elementsTransformed;
}


// 下面为表达式相关的结构体和函数(表达式代表每个形如an*x^n+...a1*x+a0的多项式，作为计算时的最小单元)
// 定义表达式结构体(表达式代表每个形如an*x^n+...a1*x+a0的多项式，作为计算时的最小单元)
typedef struct {
    int powerCount; // 表达式最高的阶次，即n
    long double *factorValue; // 各个未知数次幂的系数，次幂数从小到大，即a0,a1,...,an
} Expression;

// 释放表达式内存
void freeExpression(Expression *expression) {
    free(expression->factorValue);
    free(expression);
}

// 根据次幂数新建空表达式(n -> an*x^n+...+a1*n+a0, an~a0未定义)
Expression *expressionNew(const int powerCount) {
    Expression *expression = (Expression *) malloc(sizeof(Expression));
    expression->powerCount = powerCount;
    expression->factorValue = (long double *) malloc((powerCount + 1) * sizeof(long double));
    return expression;
}

// 根据次幂数新建全0的空表达式(n -> an*x^n+...+a1*n+a0, an~a0均为0)
Expression *expressionNew0(const int powerCount) {
    Expression *expression = expressionNew(powerCount);
    for (int i = 0; i <= powerCount; i++) {
        expression->factorValue[i] = 0;
    }
    return expression;
}

// 复制表达式
Expression *expressionCopy(const Expression *expression) {
    Expression *expressionCopy = expressionNew(expression->powerCount);
    for (int i = 0; i <= expression->powerCount; i++) {
        expressionCopy->factorValue[i] = expression->factorValue[i];
    }
    return expressionCopy;
}

// 输出表达式用于debug的工具函数
void expressionPrint(const Expression *expression) {
    int printed = 0;
    for (int i = expression->powerCount; i >= 0; i--) {
        if (expression->factorValue[i] != 0) {
            if (printed) {
                printf(" + ");
            }
            if (i == 0) {
                printf("%.2Lf", expression->factorValue[i]);
            } else if (i == 1) {
                printf("%.2Lfx", expression->factorValue[i]);
            } else {
                printf("%.2Lfx^%d", expression->factorValue[i], i);
            }
            printed = 1;
        }
    }
    if (!printed) {
        printf("0");
    }
}

// 简化表达式(除去高次幂的0项)(自动释放原表达式内存)
Expression *expressionSimplify(Expression *expression) {
    // 求出最大非零项的阶次
    int maxPower = 0;
    for (int i = expression->powerCount; i >= 0; i--) {
        if (expression->factorValue[i] != 0) {
            maxPower = i;
            break;
        }
    }
    Expression *simplifiedExpression = expressionNew(maxPower);
    for (int i = 0; i <= maxPower; i++) {
        simplifiedExpression->factorValue[i] = expression->factorValue[i];
    }
    freeExpression(expression);
    return simplifiedExpression;
}

// 元素转表达式(需确定元素为值类型而非符号)(不自动释放元素内存)
Expression *elementToExpression(const Element *element) {
    Expression *expression;
    if (element->type == NUMBER) {
        expression = expressionNew(0);
        expression->factorValue[0] = element->value;
    } else if (element->type == VARIABLE) {
        expression = expressionNew(1);
        expression->factorValue[0] = 0;
        expression->factorValue[1] = 1;
    } else {
        return NULL;
    }
    return expression;
}

// 表达式加法(结果取次幂数最大的为新的次幂数，然后将对应次幂的系数相加即可)
Expression *expressionAdd(const Expression *expression1, const Expression *expression2, char *error) {
    // 加法
    // 取次幂数最高值为结果次幂，然后逐项循环求值
    int powerCount =
            expression1->powerCount > expression2->powerCount ?
            expression1->powerCount : expression2->powerCount;
    Expression *expression = expressionNew0(powerCount);
    for (int i = 0; i <= expression1->powerCount; i++) {
        expression->factorValue[i] += expression1->factorValue[i];
    }
    for (int i = 0; i <= expression2->powerCount; i++) {
        expression->factorValue[i] += expression2->factorValue[i];
    }
    // 简化结果
    expression = expressionSimplify(expression);
    return expression;
}

// 表达式减法(结果取次幂数最大的为新的次幂数，然后将对应次幂的系数相减即可)
Expression *expressionSubtract(const Expression *expression1, const Expression *expression2, char *error) {
    // 减法
    // 取次幂数最高值为结果次幂，然后逐项循环求值
    int powerCount =
            expression1->powerCount > expression2->powerCount ?
            expression1->powerCount : expression2->powerCount;
    Expression *expression = expressionNew0(powerCount);
    for (int i = 0; i <= expression1->powerCount; i++) {
        expression->factorValue[i] += expression1->factorValue[i];
    }
    for (int i = 0; i <= expression2->powerCount; i++) {
        expression->factorValue[i] -= expression2->factorValue[i];
    }
    // 简化结果
    expression = expressionSimplify(expression);
    return expression;
}

// 表达式乘法(结果取次幂数之和的为新的次幂数，然后循环并累积各个次幂数的值)(基于整式乘法)
Expression *expressionMultiply(const Expression *expression1, const Expression *expression2, char *error) {
    // 整式乘法
    // 取次幂数之和为结果次幂，然后逐项循环求值
    int powerCount = expression1->powerCount + expression2->powerCount;
    Expression *expression = expressionNew0(powerCount);
    for (int i = 0; i <= expression1->powerCount; i++) {
        for (int j = 0; j <= expression2->powerCount; j++) {
            expression->factorValue[i + j] += expression1->factorValue[i] * expression2->factorValue[j];
        }
    }
    // 简化结果
    expression = expressionSimplify(expression);
    return expression;
}

// 表达式除法(仅支持除以一个常数，除以0报错，结果取被除数的次幂数，循环将每个系数均除以除数即可)
Expression *expressionDivide(const Expression *expression1, const Expression *expression2, char *error) {
    // 除数必须为常数(次幂数为0)
    if (expression2->powerCount != 0) {
        strcpy(error, "divisor must be a constant");
        return NULL;
    }
    // 除数不能为0
    if (expression2->factorValue[0] == 0) {
        strcpy(error, "divide by zero");
        return NULL;
    }
    // 除法
    // 新建表达式，直接循环求值
    int powerCount = expression1->powerCount;
    Expression *expression = expressionNew(powerCount);
    for (int i = 0; i <= expression1->powerCount; i++) {
        expression->factorValue[i] = expression1->factorValue[i] / expression2->factorValue[0];
    }
    // 简化结果
    expression = expressionSimplify(expression);
    return expression;
}

// 表达式求余(仅支持常数求模一个常数，求模0报错，均为常数故直接计算即可)
Expression *expressionMod(const Expression *expression1, const Expression *expression2, char *error) {
    //  模数必须为常数(次幂数为0)
    if (expression2->powerCount != 0) {
        strcpy(error, "modulus must be a constant");
        return NULL;
    }
    // 模数不能为0
    if (expression2->factorValue[0] == 0) {
        strcpy(error, "modulo by zero");
        return NULL;
    }
    // 被除数必须为常数(次幂数为0)
    if (expression1->powerCount != 0) {
        strcpy(error, "dividend must be a constant");
        return NULL;
    }
    // 求余
    // 新建表达式，直接循环求值
    int powerCount = expression1->powerCount;
    Expression *expression = expressionNew(powerCount);
    for (int i = 0; i <= expression->powerCount; i++) {
        expression->factorValue[i] =
                expression1->factorValue[i] -
                (int) (expression1->factorValue[i] / expression2->factorValue[0]) * expression2->factorValue[0];
    }
    // 简化结果
    expression = expressionSimplify(expression);
    return expression;
}

// 表达式求幂的底层函数，基于表达式乘法实现递归二分的快速幂(需保证幂次为整数)
Expression *expressionPowerQuick(const Expression *expression1, const int powerCount, char *error) {
    // 快速幂
    // 如果幂次为0，返回1
    if (powerCount == 0) {
        Expression *expression = expressionNew(0);
        expression->factorValue[0] = 1;
        return expression;
    }
    // 如果幂次为1，返回原表达式
    if (powerCount == 1) {
        return expressionCopy(expression1);
    }
    // 新建表达式，二分递归求值
    // 求出一半的幂次
    Expression *expressionHalf = expressionPowerQuick(expression1, powerCount / 2, error);
    // 求出模2的幂次
    Expression *expressionMod2 = expressionPowerQuick(expression1, powerCount % 2, error);
    // 两者相乘
    Expression *expression = expressionMultiply(expressionHalf, expressionHalf, error);
    expression = expressionMultiply(expression, expressionMod2, error);
    // 简化结果
    expression = expressionSimplify(expression);
    // 释放内存
    freeExpression(expressionHalf);
    freeExpression(expressionMod2);

    return expression;
}

// 表达式求幂(结果幂次为两个参数的幂次之积，内部使用上面的快速幂函数计算)
Expression *expressionPower(const Expression *expression1, const Expression *expression2, char *error) {
    // 次幂数必须为常数(次幂数为0)
    if (expression2->powerCount != 0) {
        strcpy(error, "exponent must be a constant");
        return NULL;
    }
    // 次幂数不能为负数
    if (expression2->factorValue[0] < 0) {
        strcpy(error, "exponent must be a positive number");
        return NULL;
    }
    // 次幂数必须为整数
    if (expression2->factorValue[0] != (int) expression2->factorValue[0]) {
        strcpy(error, "exponent must be an integer");
        return NULL;
    }
    // 求幂
    // 新建表达式，使用快速幂函数计算
    int powerNum = (int) expression2->factorValue[0];
    Expression *expression = expressionPowerQuick(expression1, powerNum, error);
    // 简化结果
    expression = expressionSimplify(expression);
    return expression;
}


// 下面为计算表达式的相关函数
// 操作符结构体
typedef struct {
    char symbol; // 符号
    int priority; // 优先级(实际优先级，遇到左括号会增加优先级，遇到右括号会减少优先级)
} Operator;

// 输出操作符和优先级用于debug的工具函数
void operatorPrint(const Operator *operator) {
    printf("OPERATOR(%c, %d)", operator->symbol, operator->priority);
}

// 输出操作符优先级，+-为1、*/%为2、^为3，每遇到一个左括号则接下来的运算符优先级全部加4，每一个右括号则取消加4
int operatorPriority(const char symbol) {
    switch (symbol) {
        case '+':
        case '-':
            return 1;
        case '*':
        case '/':
        case '%':
            return 2;
        case '^':
            return 3;
        default:
            return 0 / 0;
    }
}

// 表达式计算(括号优先级改变，操作符和变量常量入栈计算)
/*
 循环遍历元素列表，将变量和常量转为表达式记入表达式栈，操作符和优先级记入操作符栈(单调栈)
 括号会改变接下来操作符的优先级，操作符入栈时弹出优先级更大的操作符再入栈
 每次弹出操作符时同时弹出两个表达式一起计算，结果表达式再存入表达式栈
 反复运算直到结束
*/
Expression *expressionCalculate(const Element *elements, const int numElements, char *error) {
    // 操作符优先级：^:3, */%:2, +-:1, ()全体加4
    // 操作符栈：存储操作符，遇到左括号则接下来的优先级加4，遇到右括号则接下来的优先级减4，遇到操作符则弹出两个表达式进行计算，然后将计算结果入栈
    int numOperators = 0;
    Operator *operatorStack = (Operator *) malloc(numElements * sizeof(Operator));
    // 操作数栈：存储表达式，遇到数字或未知数则转换为表达式入栈，遇到操作符则弹出两个表达式进行计算，然后将计算结果入栈
    int numExpressions = 0;
    Expression *expressionStack = (Expression *) malloc(numElements * sizeof(Expression));
    // 从左到右遍历元素数组，遇到数字或未知数则转换为表达式，遇到操作符则根据操作符计算
    int currentPlusPriority = 0; // 当前的额外优先级
    for (int i = 0; i < numElements; i++) {
        switch (elements[i].type) {
            case NUMBER:
            case VARIABLE:
                // 转换为表达式入栈
                expressionStack[numExpressions] = *elementToExpression(&elements[i]);
                numExpressions++;
                // log：输出表达式
                if (debug == 1) {
                    printf("Stack an expression:");
                    expressionPrint(&expressionStack[numExpressions - 1]);
                    printf("\n");
                }
                break;
            case OPERATOR:
                // 如果是左括号，接下来的算符的优先级加4
                if (elements[i].value == '(') {
                    currentPlusPriority += 4;
                    break;
                }
                // 如果是右括号，接下来的算符的优先级减4
                if (elements[i].value == ')') {
                    currentPlusPriority -= 4;
                    break;
                }
                // 如果是其他操作符，与操作符栈中的优先级比较，如果比栈顶的优先级高，则入栈，否则一直弹出栈顶运算符并计算直到栈顶优先级低于当前运算符
                // 如果栈顶优先级低于当前运算符，则入栈
                int currentPriority = operatorPriority((char) elements[i].value);
                if (numOperators == 0 ||
                    operatorStack[numOperators - 1].priority < currentPriority + currentPlusPriority) {
                    operatorStack[numOperators].symbol = (char) elements[i].value;
                    operatorStack[numOperators].priority = currentPriority + currentPlusPriority;
                    numOperators++;
                    // log：输出操作符
                    if (debug == 1) {
                        printf("Stack an operator:");
                        operatorPrint(&operatorStack[numOperators - 1]);
                        printf("\n");
                    }
                    break;
                }
                // 如果栈顶优先级高于当前运算符，则弹出栈顶运算符并计算直到栈顶优先级低于当前运算符
                while (numOperators > 0 &&
                       operatorStack[numOperators - 1].priority >= currentPriority + currentPlusPriority) {
                    // 弹出栈顶运算符
                    Operator operator = operatorStack[numOperators - 1];
                    numOperators--;
                    // log：输出操作符
                    if (debug == 1) {
                        printf("Pop an operator:");
                        operatorPrint(&operator);
                        printf("\n");
                    }
                    // 弹出两个表达式
                    if (numExpressions < 2) {
                        strcpy(error, "expression stack is empty");
                        return NULL;
                    }
                    Expression expression2 = expressionStack[numExpressions - 1];
                    numExpressions--;
                    Expression expression1 = expressionStack[numExpressions - 1];
                    numExpressions--;
                    // log：输出表达式
                    if (debug == 1) {
                        printf("Pop two expressions:");
                        expressionPrint(&expression1);
                        printf("\t");
                        expressionPrint(&expression2);
                        printf("\n");
                    }
                    // 根据运算符计算
                    Expression *expressionResult = NULL;
                    switch (operator.symbol) {
                        case '+':
                            expressionResult = expressionAdd(&expression1, &expression2, error);
                            break;
                        case '-':
                            expressionResult = expressionSubtract(&expression1, &expression2, error);
                            break;
                        case '*':
                            expressionResult = expressionMultiply(&expression1, &expression2, error);
                            break;
                        case '/':
                            expressionResult = expressionDivide(&expression1, &expression2, error);
                            break;
                        case '%':
                            expressionResult = expressionMod(&expression1, &expression2, error);
                            break;
                        case '^':
                            expressionResult = expressionPower(&expression1, &expression2, error);
                            break;
                        default:
                            strcpy(error, "unknown operator");
                            break;
                    }
                    free(expression1.factorValue);
                    free(expression2.factorValue);
                    // 如果计算出错，则返回NULL
                    if (expressionResult == NULL) {
                        return NULL;
                    }
                    // 计算结果入栈
                    expressionStack[numExpressions] = *expressionCopy(expressionResult);
                    numExpressions++;
                    // log：输出表达式
                    if (debug == 1) {
                        printf("Stack an expression:");
                        expressionPrint(&expressionStack[numExpressions - 1]);
                        printf("\n");
                    }
                    freeExpression(expressionResult); // FIXME
                }
                // 然后操作符入栈
                operatorStack[numOperators].symbol = (char) elements[i].value;
                operatorStack[numOperators].priority = currentPriority + currentPlusPriority;
                numOperators++;
                // log：输出操作符
                if (debug == 1) {
                    printf("Stack an operator:");
                    operatorPrint(&operatorStack[numOperators - 1]);
                    printf("\n");
                }
                break;
            default:
                // 不可能到达此分支，报错
                strcpy(error, "unknown element type");
                return NULL;
        }
    }
    // 遍历完元素数组后，如果操作符栈不为空，则弹出栈顶运算符并计算直到栈为空
    while (numOperators > 0) {
        // 弹出栈顶运算符
        Operator operator = operatorStack[numOperators - 1];
        numOperators--;
        // log：输出操作符
        if (debug == 1) {
            printf("Pop an operator:");
            operatorPrint(&operator);
            printf("\n");
        }
        // 弹出两个表达式
        if (numExpressions < 2) {
            strcpy(error, "expression stack is empty");
            return NULL;
        }
        Expression expression2 = expressionStack[numExpressions - 1];
        numExpressions--;
        Expression expression1 = expressionStack[numExpressions - 1];
        numExpressions--;
        // log：输出表达式
        if (debug == 1) {
            printf("Pop two expressions:");
            expressionPrint(&expression1);
            printf("\t");
            expressionPrint(&expression2);
            printf("\n");
        }
        // 根据运算符计算
        Expression *expressionResult = NULL;
        switch (operator.symbol) {
            case '+':
                expressionResult = expressionAdd(&expression1, &expression2, error);
                break;
            case '-':
                expressionResult = expressionSubtract(&expression1, &expression2, error);
                break;
            case '*':
                expressionResult = expressionMultiply(&expression1, &expression2, error);
                break;
            case '/':
                expressionResult = expressionDivide(&expression1, &expression2, error);
                break;
            case '%':
                expressionResult = expressionMod(&expression1, &expression2, error);
                break;
            case '^':
                expressionResult = expressionPower(&expression1, &expression2, error);
                break;
            default:
                strcpy(error, "unknown operator");
                break;
        }
        free(expression1.factorValue);
        free(expression2.factorValue);
        // 如果计算出错，则返回NULL
        if (expressionResult == NULL) {
            return NULL;
        }
        // 计算结果入栈
        expressionStack[numExpressions] = *expressionCopy(expressionResult);
        numExpressions++;
        // log：输出表达式
        if (debug == 1) {
            printf("Stack an expression:");
            expressionPrint(&expressionStack[numExpressions - 1]);
            printf("\n");
        }
        freeExpression(expressionResult); // FIXME
    }
    // 如果操作数栈不为空，则返回栈顶表达式
    if (numExpressions == 1) {
        Expression *expressionResult = expressionCopy(&expressionStack[numExpressions - 1]);
        // 简化结果
        expressionResult = expressionSimplify(expressionResult);
//        // 循环释放内存
//        for (int i = 0; i < numExpressions; i++) {
//            freeExpression(&expressionStack[i]);
//        }
        free(expressionStack);
        free(operatorStack);
        return expressionResult;
    }
    // 如果操作数栈为空，则返回NULL
    strcpy(error, "expression stack count is wrong");
    return NULL;
}

// 表达式求值，表达式未知数代入后的值(令x=x_0)
long double expressionEvaluate(const Expression *expression, const long double variableValue) {
    long double result = 0;
    long double variableValuePower = 1; // 记录未知数的幂次值
    for (int i = 0; i <= expression->powerCount; i++) {
        result += expression->factorValue[i] * variableValuePower;
        variableValuePower *= variableValue;
    }
    return result;
}

// 表达式求导，a_(n-1)'=a_n*n
Expression *expressionDerivative(const Expression *expression) {
    // 求导
    // 新建表达式，逐项求导
    Expression *expressionDerivative = expressionNew(expression->powerCount - 1);
    for (int i = 0; i <= expressionDerivative->powerCount; i++) {
        expressionDerivative->factorValue[i] = expression->factorValue[i + 1] * (i + 1);
    }
    // 简化结果
    expressionDerivative = expressionSimplify(expressionDerivative);
    return expressionDerivative;
}

// 二分求根：根据左右端点和表达式进行二分求根，需确保左右端点代入表达式后的值异号
/*
数值 二分求根(表达式, 左端点, 右端点){
    if(左端点和右端点的差值小于阈值){
        return (左端点+右端点)/2
    }
    中点 = (左端点+右端点)/2
    if(表达式在中点的值为0){
        return 中点
    }
    if(表达式在中点的值和左端点的值异号){
        return 二分求根函数(表达式, 左端点, 中点)
    }
    if(表达式在中点的值和右端点的值异号){
        return 二分求根函数(表达式, 中点, 右端点)
    }
    return 错误
}
*/
long double expressionBinarySearchRoot(const Expression *expression, const long double left, const long double right) {
    // 二分求根
    // 如果左右端点的表达式值相等，则返回0/0
    if (expressionEvaluate(expression, left) == expressionEvaluate(expression, right)) {
        return 0 / 0;
    }
    // 如果左右端点的表达式值同号，则返回0/0
    if (expressionEvaluate(expression, left) * expressionEvaluate(expression, right) > 0) {
        return 0 / 0;
    }
    // 如果左右端点的差值小于阈值，则返回中点
    if (lfabs(right - left) < BINARY_SEARCH_ROOT_THRESHOLD) {
        return (left + right) / 2;
    }
    // 计算中点
    long double middle = (left + right) / 2;
    // 如果表达式在中点的值为0，则返回中点
    if (lfabs(expressionEvaluate(expression, middle)) < BINARY_SEARCH_ROOT_THRESHOLD) {
        return middle;
    }
    // 如果表达式在中点的值和左端点的值异号，则返回二分求根函数(表达式, 左端点, 中点)
    if (expressionEvaluate(expression, middle) * expressionEvaluate(expression, left) < 0) {
        return expressionBinarySearchRoot(expression, left, middle);
    }
    // 如果表达式在中点的值和右端点的值异号，则返回二分求根函数(表达式, 中点, 右端点)
    if (expressionEvaluate(expression, middle) * expressionEvaluate(expression, right) < 0) {
        return expressionBinarySearchRoot(expression, middle, right);
    }
    // 不可能到达此分支，返回0/0
    return 0 / 0;
}

// 表达式求根：先求导数，然后求极值点，方程的根一定在 x最小值∪极值点∪x最大值 这个列表的相邻点之间，逐段二分即可得到所有根
/*
数值列表 表达式求根(表达式){
    if(表达式.阶数==1){
        return 新列表(-表达式.常数项/表达式.一次项)
    }
    导数表达式 = 表达式求导(表达式)
    导数零点列表 = 表达式求根(导数表达式)
    新建 结果列表(导数表达式的根的列表的长度)
    for i in (x最小值∪导数零点列表∪x最大值).index {
        if(表达式求值(表达式, 导数零点列表[i]) == 0){
            结果列表.push(导数零点列表[i])
            continue
        }
        if(i-1>=0 && 表达式求值(导数表达式, 导数零点列表[i])*表达式求值(导数表达式, 导数零点列表[i-1])<0){
            结果列表.push(二分求根函数(表达式, 导数零点列表[i], 导数零点列表[i-1]))
        }
    }
    return 结果列表
}
*/
long double *expressionFindRoot(const Expression *expression, int *numRoots) {
    // 求根
    // 如果表达式阶数为0，则返回空列表
    if (expression->powerCount == 0) {
        *numRoots = 0;
        return NULL;
    }
    // 如果表达式阶数为1，则返回列表(-表达式.常数项/表达式.一次项)
    if (expression->powerCount == 1) {
        *numRoots = 1;
        long double *roots = (long double *) malloc(sizeof(long double));
        roots[0] = -expression->factorValue[0] / expression->factorValue[1];
        return roots;
    }
    // 求导数表达式
    Expression *derivativeExpression = expressionDerivative(expression);
    // 求导数表达式的根
    long double *derivativeRoots = expressionFindRoot(derivativeExpression, numRoots);
    // 新建结果列表
    long double *roots = (long double *) malloc((*numRoots + 1) * sizeof(long double));
    int numRootsNew = 0;
    // 二分最值的左端点和右端点也应该判断
    // 二分最值的左端点和第一个导数根判断
    if (expressionEvaluate(expression, BINARY_SEARCH_ROOT_MIN) * expressionEvaluate(expression, derivativeRoots[0]) <
        0) {
        roots[numRootsNew] = expressionBinarySearchRoot(expression, BINARY_SEARCH_ROOT_MIN, derivativeRoots[0]);
        numRootsNew++;
    }
    // 遍历导数表达式的根
    for (int i = 0; i < *numRoots; i++) {
        // 如果表达式在导数表达式的根的值为0，则将导数表达式的根加入结果列表
        if (lfabs(expressionEvaluate(expression, derivativeRoots[i])) < BINARY_SEARCH_ROOT_THRESHOLD) {
            roots[numRootsNew] = derivativeRoots[i];
            numRootsNew++;
            continue;
        }
        // 如果表达式在导数表达式的根和导数表达式的根的前一个值异号，则将二分求根函数(表达式, 导数表达式的根, 导数表达式的根的前一个值)加入结果列表
        if (i - 1 >= 0 &&
            expressionEvaluate(expression, derivativeRoots[i]) *
            expressionEvaluate(expression, derivativeRoots[i - 1]) < BINARY_SEARCH_ROOT_THRESHOLD) {
            roots[numRootsNew] = expressionBinarySearchRoot(expression, derivativeRoots[i], derivativeRoots[i - 1]);
            numRootsNew++;
        }
    }
    // 二分最值的右端点和最后一个导数根判断
    if (expressionEvaluate(expression, BINARY_SEARCH_ROOT_MAX) *
        expressionEvaluate(expression, derivativeRoots[*numRoots - 1]) < 0) {
        roots[numRootsNew] = expressionBinarySearchRoot(expression, derivativeRoots[*numRoots - 1],
                                                        BINARY_SEARCH_ROOT_MAX);
        numRootsNew++;
    }
    // 合并相同的根(差值小于阈值的根合并)
    int deletedRootNum = 0; // 记录删除的根的个数，也是下次判断的跨度
    for (int i = 0; i < numRootsNew - deletedRootNum; i++) {
        roots[i] = roots[i + deletedRootNum];
        while (i + 1 + deletedRootNum < numRootsNew &&
               lfabs(roots[i] - roots[i + 1 + deletedRootNum]) < BINARY_SEARCH_ROOT_THRESHOLD) {
            deletedRootNum++;
        }
    }
    numRootsNew -= deletedRootNum;
    for (int i = 0; i < numRootsNew; i++) {
        if (lfabs(roots[i]) < BINARY_SEARCH_ROOT_THRESHOLD) roots[i] = 0;
    }
    // log：输出根
    if (debug == 1) {
        printf("Roots:");
        for (int i = 0; i < numRootsNew; i++) {
            printf("%Lf  ", roots[i]);
        }
    }
    // 释放导数表达式的根内存
    free(derivativeRoots);
    // 返回结果列表
    *numRoots = numRootsNew;
    return roots;
}

// 最终的计算表达式的函数(字符串->元素列表->最终表达式->求根/求值)
int calculate_expression(const char *expression, char *result_msg) {
    // 解析字符串->元素(符号/字母/数字)
    char *error = (char *) malloc(strlen(expression) * sizeof(char));
    int numElements;
    Element *elements = parseString(expression, &numElements, error);
    // 判断元素数组的正确性
    if (elements == NULL) {
        sprintf(result_msg, "Error:\tsymbol %s was not define", error);
        return 0;
    }
    if (numElements == 0) {
        sprintf(result_msg, "Error:\tmissing expression");
        return 0;
    }
    // 判断元素数组的正确性
    if (!checkElements(elements, numElements, error)) {
        sprintf(result_msg, "Error: \t%s\n", error);
        return 0;
    }

    // 判断元素列表为计算式还是未知数方程并处理
    int isEquation = (findEquals(elements, numElements) != -1);
    if (isEquation) {
        // 为等式，进行转换，将等式转换为表达式的元素列表
        int numElementsTransformed;
        Element *elementsTransformed = transformEquals(elements, numElements, &numElementsTransformed);
        freeElements(elements);
        elements = elementsTransformed;
        numElements = numElementsTransformed;
    } else {
        // 为计算式，检查所有元素中不能出现未知数
        for (int i = 0; i < numElements; i++) {
            if (elements[i].type == VARIABLE) { // 报错，只有等式中才能出现未知数，计算式不行
                sprintf(result_msg, "Error:\tOnly variables can appear in equations, not in calculations");
                return 0;
            }
        }
    }
    if (debug) {
        // 输出看看结果
        printf("Parsed Elements:\t");
        for (int i = 0; i < numElements; i++) {
            elementPrint(&elements[i]);
        }
        printf("\n");
    }

    // 正式开始计算表达式(使用expressionCalculate函数，将元素列表转为整个多项式)
    Expression *expressionResult = expressionCalculate(elements, numElements, error);
    if (expressionResult == NULL) {
        sprintf(result_msg, "Error: \t%s\n", error);
        return 0;
    }
    if (debug) {
        // 输出结果
        printf("Final expression:\t");
        expressionPrint(expressionResult);
        printf("\n");
    }
    // 特判：如果超过255次幂，报错
    if (expressionResult->powerCount >= MAX_POWER_COUNT) {
        sprintf(result_msg, "Error:\tpower count is too large(>=%d)\n", MAX_POWER_COUNT);
        return 0;
    }

    // 求值/求根，然后输出
    if (!isEquation) {
        // 求值，已经求完，直接输出结果
        sprintf(result_msg, "Result:\t%Lf\n", expressionResult->factorValue[0]);
    } else {
        // 求根，递归的值二分法进行计算
        int numRoots;
        long double *roots = expressionFindRoot(expressionResult, &numRoots);
        // 输出结果
        if (numRoots == 0) { // 无解
            sprintf(result_msg, "Roots:\tNo roots\n");
        } else { // 循环输出解
            sprintf(result_msg, "Roots:\t");
            int result_len = (int) strlen(result_msg);
            char result_tmp[32];
            for (int i = 0; i < numRoots; i++) {
                sprintf(result_tmp, "%Lf ", roots[i]);
                strcpy(result_msg + result_len, result_tmp);
                result_len += (int) strlen(result_tmp);
            }
            sprintf(result_msg + result_len, "\n");
        }
    }
    // 释放内存
    freeExpression(expressionResult);
    freeElements(elements);

    return 1;
}
