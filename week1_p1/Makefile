Filename = cmplr
lex = $(Filename).l
yacc = $(Filename).y

.PHONY : clean test

# Create lex and yacc files and compile together
$(Filename).out : lex.yy.c y.tab.c
	gcc -o $(Filename).out lex.yy.c y.tab.c    # creates .out file

lex.yy.c : $(lex) y.tab.h
	lex $(lex)				# creates lex.yy.c

y.tab.h y.tab.c : $(yacc)
	yacc -d -v $(yacc)		# creates y.tab.c, y.tab.h, y.output

test : $(Filename).out
	@echo "=== Running test on original file... ===\n"
	./cmplr.out < no_comments_sample.c
	@echo "\n\n=== Running compiler on file with line 2 removed... ===\n"
	./cmplr.out < no_comments_sample2.c

clean :
	rm -f y.tab.c y.tab.h y.output lex.yy.c $(Filename).out 
