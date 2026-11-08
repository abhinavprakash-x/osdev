#include "ulib.h"

int main(void)
{
    uprintf("Simple Calculator v1.0\n");

    char left[16];
    char op[4];
    char right[16];

    uprintf("Enter a Number: ");
    readline(left, sizeof(left));

    uprintf("Enter the Operator (+, -, *, /): ");
    readline(op, sizeof(op));

    uprintf("Enter a Number: ");
    readline(right, sizeof(right));

    int num1 = uatoi(left);
    int num2 = uatoi(right);
    int result;

    if     (op[0] == '+') result = num1 + num2;
    else if(op[0] == '-') result = num1 - num2;
    else if(op[0] == '*') result = num1 * num2;
    else if(op[0] == '/') result = num1 / num2;
    else
    {
        uprintf("Error: Invalid operator\n");
        return 1;
    }

    uprintf("\nResult: %d\n", result);

    return 0;
}