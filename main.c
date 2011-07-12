#include <stdio.h>
#include <stdint.h>
#define LI uint64_t

LI grid[64];

void print_grid(){
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 20; j++)
			printf("%d ",!!(grid[i]&(1<<j)));
		printf("\n");
	}
}

int main(){
	int x, y; 
	scanf("%d%d",&x,&y);
	getchar();
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 20; j++){
			char tmp,tmp2;
			tmp=getchar();
			tmp2=getchar();
		//	printf("(%c%c)",tmp,tmp2);
			if (tmp != ' ' || tmp2 != ' ')
				grid[i] |= 1<<j;
		}
		getchar();
		//printf("\n");
	}

	print_grid();

}
