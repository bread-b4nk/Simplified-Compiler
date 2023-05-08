extern void print(int);
extern int read();

int func(int i){
 	int a;
	int b;
	
	a = i*10;
	i = 20;
	b = i*10;	
	
	return(a+b); 
}
