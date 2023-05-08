extern void print(int);
extern int read();


int func(int i){
	int a;
	int b;
	
	a =  i + 10;
	b = i + 20;
	
	if (a < 10) {
		b = b * b;
		a = b * a;
	}
	else { 
		int c;
		c = a * b;
	}

	return 0;
}
