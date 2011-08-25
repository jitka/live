#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define LI uint64_t
#define BIG_GRID_SIZE 10 //kolik mrizek 64x64 pocita jedno vlakno
#define GRID_HEAP_SIZE (1<<19) //kolik malich mrizek si celkove pamatuju (1<<19 zabere 256MiB)

LI grid_heap[64*GRID_HEAP_SIZE];
int big_grid[BIG_GRID_SIZE][BIG_GRID_SIZE]; 
			     /* kde v grid_heap jsou male mrizky mrizka obsahujici 0,0
			      * je na pozici BIG_GRID_SIZE/2,BIG_GRID_SIZE/2
			      * TODO tehle big_grid by melo byt taky vic a mely by jit
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

void print_big_grid_to_file(char *name){
	//TODO netestovane
	//vypise pouzity obdelnik z big_grid
	int up=0,down=BIG_GRID_SIZE,left=0,right=BIG_GRID_SIZE;
	for(int tmp=1; tmp; up+=tmp) //to je trosku uchylne ;-) posouvato up az do casti kde neco je
		for (int i = 0; i < BIG_GRID_SIZE; i++)
			if ( big_grid[up][i] > 0 )
				tmp--;
	for(int tmp=1; tmp; down-=tmp)
		for (int i = 0; i < BIG_GRID_SIZE; i++)
			if ( big_grid[down-1][i] > 0 )
				tmp--;
	for(int tmp=1; tmp; left+=tmp)
		for (int i = up; i < down; i++)
			if ( big_grid[i][left] > 0 )
				tmp--;
	for(int tmp=1; tmp; right-=tmp)
		for (int i = up; i < down; i++)
			if ( big_grid[i][right-1] > 0 )
				tmp--;
//	printf("%d %d %d %d\n",up,down,left,right);
	
	FILE *F = fopen(name,"w");
	for (int i = up; i < down; i++){
		for (int r = 0; r < 64; r++){
			for (int j = left; j < right; j++){
				int start = 64*big_grid[i][j];
				for (int s = 0; s < 64; s++)
					fprintf(F,"%d", !!(grid_heap[start+r]&(1ULL<<s)) );
			}
			fprintf(F,"\n");
		}
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

void step_grid(LI grid[64]){
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

void step(){
	step_grid(&grid_heap[64]);
}

void help(){
	printf("pouziti: \n./program-opt jmeno_vstupniho_souboru pocet_interaci jmeno_vystupniho_souboru\n");
	exit(0);	
}

int main(int argc, char *argv[]) {

	if (argc < 4){
		help();
	}

	FILE *F = fopen(argv[1],"r");
	if (F == NULL) {
		printf("neotevru %s\n",argv[1]);
		exit(1);	
	}

	int x, y; 
	fscanf(F,"%d%d",&x,&y);
	if (x>=63 || y>=63){
		printf("TODO vetsi mrizky\n");
		exit(1);	
	}

	fgetc(F);
	int start=1*64;
	for (int i = 0; i < x; i++){
		for (int j = 0; j < y; j++){
			if ( fgetc(F) == '1')
				grid_heap[start+i] |= 1ULL<<j;
		}
		fgetc(F);
	}
	fclose(F);
	big_grid[BIG_GRID_SIZE/2][BIG_GRID_SIZE/2]=1;

	for( int i = 0; i < atoi(argv[2]); i++){
		step();
	}

	print_big_grid_to_file(argv[3]);

	return 0;
}
