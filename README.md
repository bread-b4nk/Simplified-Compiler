# Simplified Compiler!

## Chavin (Kris) Udomwongsa 

**For grading: please use `make final`** as it'll run through all the fianl test files provided.


In terms of testing we have `make first_test`, `make test_sem` and `make test_opt`, and `make final`


**The rest of this readme is not done, I want to do a nice writeup of everything for my future self**.

The compiler is broken up into the following stages

1. lexical analysis
2. semantic analysis (ast creation and parsing)
3. llvm IR generation 
4. optimization

I will go into detail on how these sections work, which files belong to each section, and how they work together.

## Lexical Analysis

## Semantic Analysis

## LLVM IR Generation

## Optimization

This occurs in `cmplr.c` and not in its own optimization function, primarily because of the nightmare of importing functions from something generated by clang into something generated using g++.



For class we used a dumbed-down (simplified) version of C, mini-c.

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


