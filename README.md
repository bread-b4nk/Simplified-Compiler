# Simplified Compiler!

# Chavin (Kris) Udomwongsa 

We have a `make clean` and a `make test` to run the compiler on the test files


Below are notes that I've made, I haven't put in much effort into making it very readable.

Rules (of mini-c):
- no comments
- single function, can be named anything other than main, read, or print
- only integers, no other data type
- one variable declared per line
- variable declarations are always at the start of a code block, however they don't need to exist
- all arithmetic operations have 2 operands, and we only do +,-,*,/
- only logical operations are <,>,==,>=,<=
- boolean expressions cannot appear on the right hand side of an assignment statement
- boolean expressions cannot be returned, or passed as a parameter to the print function

Yacc Rules:

code is made up of codeblock's and a lot of the impletention of rules regarding `{` and `}` going around blocks of code will be defined in each individual type of code block.

Variable Declarations
	<data type> <variable name>;

Assignment Statements:
	<variable name> = <arithmetic expression>;

Two types of expressions: arithmetic and boolean

	An arithmetic expression will be
		<variable name/number> <arithmetic operator> <variable name/number>
		

	A boolean expression will be
		<variable name/number> <boolean operator> <variable name/number>

Return Statements
	return;
	or
	return(<arithmetic expression>);
	or 
	return <arithmetic expression>;
	or
	return <variable name>;
	or
	return(<variable name>);

If Else
	if(<boolean expression>) {
		anything
	}
	
	or

	if(<boolean expression>) {
		anything
	}
	else {
		anything
	}

While
	while(<boolean expression>){
		anything
	}


