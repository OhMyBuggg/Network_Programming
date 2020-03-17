#include <stdlib.h>
#include <stdio.h>

int main(){
	int a = 12345;
	int temp = a;
	int count = 0;
	while(temp != 0){
		temp = temp/10;
		count++;
	}
	char *p = (char *)malloc(count * sizeof(char));
	sprintf(p, "%d", a);
	temp = atoi(p);
	printf("%d\n", temp-100);
}