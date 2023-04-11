Share with "kvasanta" on git

# please run `make` and then `make test`

I've reached the requirements of the current checkpoint, however I've got an error I haven't debugged, I can't seem to parse the second line of the mini-c code! (the read() function declaration.) 

Below are notes that I've made, I haven't put in much effort into making it very readable.

Rules:
- no comments
- single function, can be named anything other than main, read, or print
- only integers, no other data type
- one variable declared per line
- variable declarations are always at the start of a code block
- all arithmetic operations have 2 operands, and we only do +,-,*,/
- only logical operations are <,>,==,>=,<=
- only while ???
- no comments
- boolean expressions cannot appear on the right hand side of an assignment statement
- boolean expressions cannot be returned, or passed as a parameter to the print function
- all blocks start with declarations, you cannot have it later. But there can be NO declarations too right? 

Our basic building blocks (tokens) will be:
- code_statements is what everything is reduced to. So that's what `%start` relies on.

- keywords
	- known functions
	- boolean values
	- data types
	- loop terms
	- if, else
- variable names
- operators
	- mathematical operators
	- logical operations
- numbers (we only deal with int)

What's Not Done: currently, I'm handling any references to `print` and `read` as known functions, or `KFUNC`'s  as 
KFUNC_CALL : `KFUNC '(' ')' ';' | KFUNC '(' VAR ')' ';'

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


