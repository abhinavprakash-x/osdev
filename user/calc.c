#include "ulib.h"

// How to take input from user in C?
// Right now the keyboard buffer is read by the shell, and the shell is responsible
// for parsing the input and executing commands. The shell reads characters from the
// keyboard buffer one by one, and when the user presses Enter, it processes the command.
// we need SYS_READ to read from the keyboard buffer directly, so that we can implement interactive user input in our programs.

int main(void)
{
    return 0;
}