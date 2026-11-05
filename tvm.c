
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define MAX_CODE 500
#define MAX_VARS 100
#define MAX_STACK 100
#define MAX_LABELS 100

typedef struct {
    char op[20], a[50], b[50], c[50];
} Instruction;

typedef struct {
    char name[50];
    double value;
} Variable;

typedef struct {
    char name[50];
    int address;
} Label;

Instruction code[MAX_CODE];
Variable vars[MAX_VARS];
Label labels[MAX_LABELS];
double stack[MAX_STACK];

int code_count = 0, var_count = 0;
int label_count = 0, top = -1;

/* Find or create a variable */
int get_var(const char *name) {
    for (int i = 0; i < var_count; i++) {
        if (strcmp(vars[i].name, name) == 0)
            return i;
    }

    if (var_count >= MAX_VARS || !name[0])
        return -1;

    int id = var_count++;
    snprintf(vars[id].name, sizeof(vars[id].name), "%s", name);
    vars[id].value = 0;

    return id;
}

/* Read a number or variable value */
int get_value(const char *text, double *value) {
    char *end;
    double n = strtod(text, &end);

    if (text[0] && *end == '\0') {
        *value = n;
        return 1;
    }

    int id = get_var(text);

    if (id < 0)
        return 0;

    *value = vars[id].value;
    return 1;
}

/* Store a value in a variable */
int set_value(const char *name, double value) {
    int id = get_var(name);

    if (id < 0)
        return 0;

    vars[id].value = value;
    return 1;
}

/* Push value onto stack */
int push(double value) {
    if (top >= MAX_STACK - 1) {
        printf("Error: Stack overflow!\n");
        return 0;
    }

    stack[++top] = value;
    return 1;
}

/* Pop value from stack */
int pop(double *value) {
    if (top < 0) {
        printf("Error: Stack underflow!\n");
        return 0;
    }

    *value = stack[top--];
    return 1;
}

/* Find label address */
int find_label(const char *name) {
    for (int i = 0; i < label_count; i++) {
        if (strcmp(labels[i].name, name) == 0)
            return labels[i].address;
    }

    return -1;
}

/* Load instructions and labels */
int load_code(const char *filename) {
    FILE *fp = fopen(filename, "r");
    char line[256];

    if (!fp) {
        perror("File opening failed");
        return 0;
    }

    while (fgets(line, sizeof(line), fp)) {
        char *comment = strchr(line, '#');

        if (comment)
            *comment = '\0';

        char *tokens[5];
        int count = 0;

        char *token = strtok(line, " \t\r\n");

        while (token && count < 5) {
            tokens[count++] = token;
            token = strtok(NULL, " \t\r\n");
        }

        if (count == 0)
            continue;

        /* Read labels */
        if (count == 1 &&
            tokens[0][strlen(tokens[0]) - 1] == ':') {

            tokens[0][strlen(tokens[0]) - 1] = '\0';

            if (label_count >= MAX_LABELS ||
                !tokens[0][0]) {
                printf("Error: Invalid or too many labels\n");
                fclose(fp);
                return 0;
            }

            if (find_label(tokens[0]) >= 0) {
                printf("Error: Duplicate label: %s\n", tokens[0]);
                fclose(fp);
                return 0;
            }

            snprintf(labels[label_count].name,
                     sizeof(labels[label_count].name),
                     "%s", tokens[0]);

            labels[label_count++].address = code_count;
            continue;
        }

        if (count > 4 || code_count >= MAX_CODE) {
            printf("Error: Invalid or oversized instruction\n");
            fclose(fp);
            return 0;
        }

        Instruction *in = &code[code_count++];

        memset(in, 0, sizeof(*in));

        snprintf(in->op, sizeof(in->op), "%s", tokens[0]);

        if (count > 1)
            snprintf(in->a, sizeof(in->a), "%s", tokens[1]);

        if (count > 2)
            snprintf(in->b, sizeof(in->b), "%s", tokens[2]);

        if (count > 3)
            snprintf(in->c, sizeof(in->c), "%s", tokens[3]);
    }

    fclose(fp);
    return 1;
}

/* Execute complete virtual machine */
void execute(void) {
    int pc = 0, steps = 0;
    double a, b, result;

    while (pc < code_count) {

        if (++steps > 100000) {
            printf("Error: Execution limit exceeded\n");
            return;
        }

        Instruction *in = &code[pc];

        printf("[PC=%d] %s\n", pc, in->op);

        /* SET */
        if (strcmp(in->op, "SET") == 0) {
            if (!in->a[0] || !in->b[0] ||
                in->c[0] ||
                !get_value(in->b, &a) ||
                !set_value(in->a, a))
                goto error;
        }

        /* Stack-based arithmetic */
        else if (
            (strcmp(in->op, "ADD") == 0 ||
             strcmp(in->op, "SUB") == 0 ||
             strcmp(in->op, "MUL") == 0 ||
             strcmp(in->op, "DIV") == 0) &&
            !in->b[0] && !in->c[0]
        ) {
            if (in->a[0])
                goto error;

            if (!pop(&b) || !pop(&a))
                return;

            if (strcmp(in->op, "ADD") == 0)
                result = a + b;

            else if (strcmp(in->op, "SUB") == 0)
                result = a - b;

            else if (strcmp(in->op, "MUL") == 0)
                result = a * b;

            else {
                if (b == 0) {
                    printf("Error: Division by zero\n");
                    return;
                }

                result = a / b;
            }

            if (!push(result))
                return;
        }

        /* Variable-based arithmetic and comparisons */
        else if (
            strcmp(in->op, "ADD") == 0 ||
            strcmp(in->op, "SUB") == 0 ||
            strcmp(in->op, "MUL") == 0 ||
            strcmp(in->op, "DIV") == 0 ||
            strcmp(in->op, "LT") == 0 ||
            strcmp(in->op, "GT") == 0 ||
            strcmp(in->op, "EQ") == 0 ||
            strcmp(in->op, "LE") == 0 ||
            strcmp(in->op, "GE") == 0
        ) {
            if (!in->a[0] || !in->b[0] || !in->c[0] ||
                !get_value(in->b, &a) ||
                !get_value(in->c, &b))
                goto error;

            if (strcmp(in->op, "ADD") == 0)
                result = a + b;

            else if (strcmp(in->op, "SUB") == 0)
                result = a - b;

            else if (strcmp(in->op, "MUL") == 0)
                result = a * b;

            else if (strcmp(in->op, "DIV") == 0) {
                if (b == 0) {
                    printf("Error: Division by zero\n");
                    return;
                }

                result = a / b;
            }

            else if (strcmp(in->op, "LT") == 0)
                result = a < b;

            else if (strcmp(in->op, "GT") == 0)
                result = a > b;

            else if (strcmp(in->op, "EQ") == 0)
                result = a == b;

            else if (strcmp(in->op, "LE") == 0)
                result = a <= b;

            else
                result = a >= b;

            if (!set_value(in->a, result))
                goto error;
        }

        /* Unconditional jump */
        else if (strcmp(in->op, "JMP") == 0) {
            if (!in->a[0] || in->b[0])
                goto error;

            int target = find_label(in->a);

            if (target < 0)
                goto error;

            pc = target;
            continue;
        }

        /* Conditional jumps */
        else if (
            strcmp(in->op, "JZ") == 0 ||
            strcmp(in->op, "JNZ") == 0
        ) {
            if (!in->a[0] || !in->b[0] || in->c[0])
                goto error;

            if (!get_value(in->a, &a))
                goto error;

            int jump = strcmp(in->op, "JZ") == 0
                       ? a == 0 : a != 0;

            if (jump) {
                int target = find_label(in->b);

                if (target < 0)
                    goto error;

                pc = target;
                continue;
            }
        }

        /* PUSH */
        else if (strcmp(in->op, "PUSH") == 0) {
            if (!in->a[0] || in->b[0] ||
                !get_value(in->a, &a))
                goto error;

            if (!push(a))
                return;
        }

        /* POP */
        else if (strcmp(in->op, "POP") == 0) {
            if (in->b[0] || in->c[0])
                goto error;

            if (!pop(&a))
                return;

            if (in->a[0]) {
                if (!set_value(in->a, a))
                    goto error;
            } else {
                printf("Popped value: %.2f\n", a);
            }
        }

        /* PRINT */
        else if (strcmp(in->op, "PRINT") == 0) {
            if (in->b[0] || in->c[0])
                goto error;

            if (in->a[0]) {
                if (!get_value(in->a, &a))
                    goto error;

                printf("Output: %.2f\n", a);
            } else {
                if (top < 0) {
                    printf("Error: Stack is empty\n");
                    return;
                }

                printf("Output: %.2f\n", stack[top]);
            }
        }

        /* HALT */
        else if (strcmp(in->op, "HALT") == 0) {
            if (in->a[0] || in->b[0] || in->c[0])
                goto error;

            printf("Program halted.\n");
            break;
        }

        else {
            printf("Error: Unknown instruction: %s\n", in->op);
            return;
        }

        pc++;
    }

    /* Display final variables */
    printf("\n--- Final Variables ---\n");

    for (int i = 0; i < var_count; i++) {
        printf("%s = %.2f\n",
               vars[i].name, vars[i].value);
    }

    /* Display final stack */
    printf("\n--- Final Stack ---\n");

    if (top < 0) {
        printf("Stack is empty.\n");
    } else {
        for (int i = top; i >= 0; i--) {
            printf("%.2f\n", stack[i]);
        }
    }

    return;

error:
    printf("Error: Invalid instruction, value, variable, or label.\n");
}

/* Main function */
int main(int argc, char *argv[]) {

    const char *filename =
        argc > 1 ? argv[1] : "program.tac";

    printf("=== Tiny Virtual Machine ===\n");
    printf("Phase 4: Integrated VM Execution\n\n");

    if (!load_code(filename)) {
        printf("Failed to load program.\n");
        return EXIT_FAILURE;
    }

    printf("Loaded %d instructions.\n\n", code_count);

    execute();

    return EXIT_SUCCESS;
}
