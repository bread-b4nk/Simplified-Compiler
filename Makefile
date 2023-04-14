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
	@echo "=== Running tests on files... ===\n"
	@echo "\n=== sample_mini_c without comments ===\n"
	./cmplr.out test_code/no_comments_sample.c
	@echo "\n\n=== p1.c  ===\n"
	./cmplr.out test_code/p1.c
	@echo "\n\n=== p2.c ===\n"
	./cmplr.out test_code/p2.c
	@echo "\n\n=== p3.c ===\n"
	./cmplr.out test_code/p3.c
	@echo "\n\n=== p4.c ===\n"
	./cmplr.out test_code/p4.c
	@echo "\n\n=== p5.c ===\n"
	./cmplr.out test_code/p5.c

clean :
	rm -f y.tab.c y.tab.h y.output lex.yy.c $(Filename).out 
