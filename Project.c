#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h> // For division handling

#define MAX_TOKENS 100
#define MAX_TOKEN_LEN 20
#define MAX_REGISTERS 10

// Function prototypes
int tokenize(const char *code, char tokens[MAX_TOKENS][MAX_TOKEN_LEN]);
int infixToPostfix(char tokens[MAX_TOKENS][MAX_TOKEN_LEN], int tokenCount, char postfix[MAX_TOKENS][MAX_TOKEN_LEN]);
void generateAssembly(const char *resultVar, char tokens[MAX_TOKENS][MAX_TOKEN_LEN], int tokenCount, int *finalResult, int *isUndefined);
int evaluatePostfix(char tokens[MAX_TOKENS][MAX_TOKEN_LEN], int tokenCount, int *isUndefined);
int precedence(char op);
int isOperator(char c);

int main()
{
    char code[256];
    char tokens[MAX_TOKENS][MAX_TOKEN_LEN];
    char postfix[MAX_TOKENS][MAX_TOKEN_LEN];
    int tokenCount, postfixCount, finalResult;
    int isUndefined = 0; // Flag to check for undefined result (division by zero)

    printf("Enter an arithmetic expression (e.g., result = a + b):\n");
    fgets(code, sizeof(code), stdin);
    code[strcspn(code, "\n")] = '\0'; // Remove trailing newline

    // Tokenize the input
    tokenCount = tokenize(code, tokens);
    if (tokenCount == 0)
    {
        printf("Error: Invalid input.\n");
        return 1;
    }

    // Check for the assignment operator
    if (strcmp(tokens[1], "=") != 0)
    {
        printf("Error: Expected '=' after the result variable.\n");
        return 1;
    }

    // Validate the LHS of the assignment
    if (!isalpha(tokens[0][0]))
    { // Check if the first character is a letter
        printf("Error: Left-hand side of '=' must be a valid variable name.\n");
        return 1;
    }

    // Convert infix to postfix
    postfixCount = infixToPostfix(tokens + 2, tokenCount - 2, postfix);
    if (postfixCount == 0)
    {
        printf("Error: Invalid expression during infix-to-postfix conversion.\n");
        return 1;
    }

    // Generate assembly code and calculate the result
    generateAssembly(tokens[0], postfix, postfixCount, &finalResult, &isUndefined);

    // Display the final result
    if (isUndefined)
    {
        printf("Result: undefined (division by zero)\n");
    }
    else
    {
        printf("Actual Result: %d\n", finalResult);
    }

    return 0;
}

// Tokenize the input code
int tokenize(const char *code, char tokens[MAX_TOKENS][MAX_TOKEN_LEN])
{
    int i = 0, j = 0, tokenCount = 0;
    char token[MAX_TOKEN_LEN];

    while (*code)
    {
        if (isspace(*code))
        {
            code++;
            continue;
        }
        if (isalnum(*code))
        {
            j = 0;
            while (isalnum(*code))
            {
                token[j++] = *code++;
            }
            token[j] = '\0';
            strcpy(tokens[tokenCount++], token);
        }
        else if (strchr("+-*/=()", *code))
        {
            token[0] = *code++;
            token[1] = '\0';
            strcpy(tokens[tokenCount++], token);
        }
        else
        {
            printf("Error: Invalid character '%c'\n", *code);
            return 0;
        }
    }
    return tokenCount;
}

// Convert infix to postfix using Shunting-Yard Algorithm
int infixToPostfix(char tokens[MAX_TOKENS][MAX_TOKEN_LEN], int tokenCount, char postfix[MAX_TOKENS][MAX_TOKEN_LEN])
{
    char stack[MAX_TOKENS][MAX_TOKEN_LEN];
    int stackTop = -1, postfixCount = 0;

    for (int i = 0; i < tokenCount; i++)
    {
        if (isalnum(tokens[i][0]))
        { // Operand
            strcpy(postfix[postfixCount++], tokens[i]);
        }
        else if (tokens[i][0] == '(')
        { // Left parenthesis
            strcpy(stack[++stackTop], tokens[i]);
        }
        else if (tokens[i][0] == ')')
        { // Right parenthesis
            while (stackTop >= 0 && stack[stackTop][0] != '(')
            {
                strcpy(postfix[postfixCount++], stack[stackTop--]);
            }
            if (stackTop < 0 || stack[stackTop][0] != '(')
            {
                return 0; // Mismatched parentheses
            }
            stackTop--; // Pop '('
        }
        else if (isOperator(tokens[i][0]))
        { // Operator
            while (stackTop >= 0 && precedence(stack[stackTop][0]) >= precedence(tokens[i][0]))
            {
                strcpy(postfix[postfixCount++], stack[stackTop--]);
            }
            strcpy(stack[++stackTop], tokens[i]);
        }
        else
        {
            return 0; // Invalid token
        }
    }

    while (stackTop >= 0)
    { // Pop remaining operators
        if (stack[stackTop][0] == '(')
        {
            return 0; // Mismatched parentheses
        }
        strcpy(postfix[postfixCount++], stack[stackTop--]);
    }

    return postfixCount;
}

// Generate assembly code and evaluate the expression
void generateAssembly(const char *resultVar, char tokens[MAX_TOKENS][MAX_TOKEN_LEN], int tokenCount, int *finalResult, int *isUndefined)
{
    char stack[MAX_REGISTERS][MAX_TOKEN_LEN];
    int stackTop = -1;
    int regCount = 1;

    for (int i = 0; i < tokenCount; i++)
    {
        if (isalnum(tokens[i][0]))
        { // Variable or number
            printf("MOV R%d, %s\n", regCount, tokens[i]);
            sprintf(stack[++stackTop], "R%d", regCount++);
        }
        else if (isOperator(tokens[i][0]))
        { // Operator
            if (stackTop < 1)
            {
                printf("Error: Invalid expression. Missing operands for operator '%s'.\n", tokens[i]);
                return;
            }
            char right[MAX_TOKEN_LEN], left[MAX_TOKEN_LEN];
            strcpy(right, stack[stackTop--]);
            strcpy(left, stack[stackTop--]);
            printf("%s R%d, %s, %s\n",
                   tokens[i][0] == '+' ? "ADD" : tokens[i][0] == '-' ? "SUB"
                                             : tokens[i][0] == '*'   ? "MUL"
                                                                     : "DIV",
                   regCount, left, right);
            sprintf(stack[++stackTop], "R%d", regCount++);
        }
    }

    if (stackTop != 0)
    {
        printf("Error: Invalid expression. Too many or too few operands.\n");
        return;
    }

    printf("MOV %s, %s\n", resultVar, stack[stackTop]);

    // Evaluate the postfix expression
    *finalResult = evaluatePostfix(tokens, tokenCount, isUndefined);
}

// Evaluate a postfix expression
int evaluatePostfix(char tokens[MAX_TOKENS][MAX_TOKEN_LEN], int tokenCount, int *isUndefined)
{
    int stack[MAX_TOKENS];
    int stackTop = -1;

    for (int i = 0; i < tokenCount; i++)
    {
        if (isalnum(tokens[i][0]))
        {                                        // Operand
            stack[++stackTop] = atoi(tokens[i]); // Convert operand to integer
        }
        else if (isOperator(tokens[i][0]))
        { // Operator
            if (stackTop < 1)
            {
                printf("Error: Invalid expression during evaluation.\n");
                return 0;
            }
            int right = stack[stackTop--];
            int left = stack[stackTop--];
            switch (tokens[i][0])
            {
            case '+':
                stack[++stackTop] = left + right;
                break;
            case '-':
                stack[++stackTop] = left - right;
                break;
            case '*':
                stack[++stackTop] = left * right;
                break;
            case '/':
                if (right == 0)
                {
                    *isUndefined = 1; // Set flag for division by zero
                    return 0;
                }
                stack[++stackTop] = left / right;
                break;
            }
        }
    }

    return stackTop == 0 ? stack[stackTop] : 0; // Return final result
}

// Helper: Get operator precedence
int precedence(char op)
{
    if (op == '+' || op == '-')
        return 1;
    if (op == '*' || op == '/')
        return 2;
    return 0;
}

// Helper: Check if character is operator
int isOperator(char c)
{
    return c == '+' || c == '-' || c == '*' || c == '/';
}
