#include <stdio.h>
#include <stdint.h>
#define LI uint64_t
#define BIG_GRID_SIZE 10 //kolik mrizek 64x64 pocita jedno vlakno
#define GRID_HEAP_SIZE (1<<19) //kolik malich mrizek si celkove pamatuju (1<<19 zabere 256MiB)

LI grid_heap[64*GRID_HEAP_SIZE];
int big_grid[2][BIG_GRID_SIZE][BIG_GRID_SIZE]; 
			     /* kde v grid_heap jsou male mrizky mrizka obsahujici 0,0
			      * je na pozici BIG_GRID_SIZE/2,BIG_GRID_SIZE/2
			      * TODO tehle big_grit by melo byt taky vic a mely by jit
			      * pocitat vicevlaknove a nechat je simulovat vic kroku
			      *
			      */


static inline void binary_luint(LI n){
	for (int i = 0; i < 64; i++){
		if (i%5==0)
			printf(" ");
		printf("%d", !!(n&(1llu<<i)) );
	}
	printf("\n");
}

void print_grid(LI grid[64]){
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 20; j++)
			printf("%d ",!!(grid[i]&(1<<j)));
		printf("\n");
	}
}

void print_grid_to_file(LI grid[64]){
	FILE *F = fopen("grid.out","w");
	for (int i = 0; i < 64; i++){
		for (int j = 0; j < 64; j++)
			fprintf(F,"%d",!!(grid[i]&(1<<j)));
		fprintf(F,"\n");
	}
	fclose(F);
}


static inline void count_line(LI line, int where[64],LI mask){
	//kolik je v prvnich trech bitech jednicek
	static int table_of_count[8] = {0,1,1,2,1,2,2,3};

	for (int i = 0; i+3 < 64; i++){
		where[i+1] += table_of_count[(line>>i) & mask];
	}
}

void step(LI grid[64]){
	//nejdriv budu do pom pristitavat kolik je v okoli a pak to 
	//presisu. Nebylo by rychlejsi to rovnou pocitat?
	int pom[64][64];
	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++)
			pom[i][j]=0;

	count_line(grid[0],pom[0],5ULL);	
	count_line(grid[1],pom[0],7ULL);
	for (int i = 1; i < 63; i++){
		count_line(grid[i-1],pom[i],7ULL);	
		count_line(grid[i],  pom[i],5ULL);	
		count_line(grid[i+1],pom[i],7ULL);
	}
	count_line(grid[62],pom[63],7ULL);	
	count_line(grid[63],pom[63],5ULL);

/*	printf("\n");
	for (int j = 0; j < 20; j++){
		for (int i = 0; i < 20; i++){
			printf("%d ",pom[j][i]);
		} printf("\n");
	} printf("\n");
	printf("\n");
*/	
	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++){
			if ( grid[i] & (1ULL<<j) ){ //ziva
				if (! (pom[i][j]==2 || pom[i][j]==3) ){
					grid[i] &= ~(1ULL<<j); //zemre
				}
			} else { //mrtva
				if ( pom[i][j]==3){
					grid[i] |= (1ULL<<j); //narodi se
				} 
			}

		}

}

int main(){
	int x, y; 
	LI grid[64];
	for (int i = 0; i < 64; i++)
		grid[i]=0ULL;

	scanf("%d%d",&x,&y);
	getchar();
	int start=1*64;
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 20; j++){
			char tmp,tmp2;
			tmp=getchar();
			tmp2=getchar();
		//	printf("(%c%c)",tmp,tmp2);
			if (tmp != ' ' || tmp2 != ' ')
				grid_heap[start+i] |= 1<<j;
		}
		getchar();
		//printf("\n");
	}
	print_grid_to_file(&grid_heap[64]);
/*
	print_grid(grid);
	printf("0\n");
	step(grid);
	print_grid(grid);
	printf("1\n");
	step(grid);
	print_grid(grid);
	printf("2\n");
	step(grid);
	print_grid(grid);
	printf("3\n");
	step(grid);
	print_grid(grid);
	printf("4\n");
	step(grid);
	print_grid(grid);
	printf("5\n");
*/
}
