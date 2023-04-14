/* This file gives you an example program in miniC language.
 * We focus on compiling the program in a single file. The program contains 
 * a single function defintion. You can name this function anything other 
 * than main, print and read. It has two extern function declarations
 * for print and read that we will use to print and read values without
 * explicitly calling printf and scanf. I have included with individual lines of code
 * to help you understand the structure of code in more detail.
*/

extern void print(int); // function to print a value
extern int read(); // function to read a value


/* All local variables and parameters are integers. Return value of the function is also an integer. */
int func(int a){ //Can have maximum of 1 parameter
        // All local variables are declared at the beginnig of a 
        // code block and each variable is declared
        // separately in a new line.
        int loc1;
        int loc2;
        int i;

        loc1 = a + 10; // All arithmetic operations will have only 2 operands
        loc2 = a * 10; // Arithmetic operations that are allowed are: +, -, *, / 

        // Conditions in if and while statements will use only  >, <, ==, >=, <= operators. We do not use logical operators (&&, ||, !)
        if (loc1 > loc2) {
                print(loc1); // Only functions you can call are print and read. We do not include system calls.
        }
        else
                print(loc2);


        i = 0;

        while (i < loc1){
                int j;
                i = i + 1;
                j = loc2 + i;
                loc2 = j;
        }

        return(loc1 + loc2);
}

/* Comments are only to help you understand the structure of a program in miniC. miniC language does not allow any comments */
