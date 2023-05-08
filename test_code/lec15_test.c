extern void print(int);
extern int read();


int func(int i){
	int a;
	int b;
	int c;
	int d;
	
	c = 10;	
	a = i + 10;
	b = i + c;
	d = 10 + 20;
	a = 10;
	
	while (i < 100){
		i = i + 5;
		c = c + 10;
		d = a + 10;
	}	
	
	if (d > 100)
		return c;
	
	return 100;
}
