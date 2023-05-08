Filename = cmplr
lex = $(Filename).l
yacc = $(Filename).y
opt = optimizer
INC = ./ast/ast.c ./ast/smtic.c ./$(opt).c 

.PHONY : clean test test_sem test_opt1

$(Filename).out : $(Filename).c lex.yy.c y.tab.c $(INC) # ./$(opt).o
	g++ -g -o $(Filename).out $(Filename).c lex.yy.c y.tab.c $(INC) `llvm-config-15 --cxxflags --ldflags --libs core` -I ./$(opt).o

$(opt).o : $(opt).c
	clang -g `llvm-config-15 --cflags` -I /usr/include/llvm-c-15/ -c $(opt).c

	
lex.yy.c : $(lex) y.tab.h
	lex $(lex)				# creates lex.yy.c

y.tab.h y.tab.c : $(yacc)
	yacc -d -v $(yacc)		# creates y.tab.c, y.tab.h, y.output

test : $(Filename).out
	@echo "=== Running tests on files... ===\n"
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
	@echo "\n=== sample_mini_c without comments ===\n"
	./cmplr.out test_code/no_comments_sample.c

test_sem : $(Filename).out
	@echo "=== Running Semantic Tests... ===\n"
	@echo "\n\n=== p1.c  ===\n"
	./cmplr.out semantic_tests/p1.c
	@echo "\n\n=== p2.c ===\n"
	./cmplr.out semantic_tests/p2.c
	@echo "\n\n=== p3.c ===\n"
	./cmplr.out semantic_tests/p3.c
	@echo "\n\n=== p4.c ===\n"
	./cmplr.out semantic_tests/p4.c

test_opt1 : $(Filename).out
	@echo "=== Running Optimization Tests... ===\n"
	@echo "\n\n=== opt_test1.c ===\n"
	@echo "Common Subexp"
	./cmplr.out opt_tests/opt_test1.c
	@echo "\n\n=== opt_test2.c ===\n"
	@echo "Safety Check: Common Subexp"
	./cmplr.out opt_tests/opt_test2.c
	@echo "\n\n=== opt_test3.c ===\n"
	@echo "Common Subexp"
	./cmplr.out opt_tests/opt_test3.c


clean :
	rm -f *.o y.tab.c y.tab.h y.output lex.yy.c $(Filename).out
	rm -f llvm-ir.s before after
