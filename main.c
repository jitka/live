#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define LI uint64_t
#define BIG_GRID_SIZE 10 //kolik mrizek 64x64 pocita jedno vlakno
#define GRID_HEAP_SIZE (1<<12) //kolik malich mrizek si celkove pamatuju (1<<19 zabere 256MiB)
#define big_grid(x,y) big_grid_1[(x)*BIG_GRID_SIZE+(y)]
#define big_grid_new(x,y) big_grid_new_1[(x)*BIG_GRID_SIZE+(y)]

LI grid_heap[64*GRID_HEAP_SIZE];
int grid_heap_count=1;
int grid_heap_[GRID_HEAP_SIZE];
/* kde v grid_heap jsou male mrizky mrizka obsahujici 0,0
 * je na pozici BIG_GRID_SIZE/2,BIG_GRID_SIZE/2
 *
 * TODO tehle big_grid by melo byt taky vic a mely by jit
 * pocitat vicevlaknove a nechat je simulovat vic kroku
 *
 */

int big_grid_tmp1[BIG_GRID_SIZE*BIG_GRID_SIZE]; 
int big_grid_tmp2[BIG_GRID_SIZE*BIG_GRID_SIZE]; 
int *big_grid_1; //kvuli tomu ze takhle nejsou dvourozmerne se s nimi pracuje pomoci
int *big_grid_new_1; //maker big_grid a big_grid_new

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

void print_big_grid_to_file(char *name){
	//TODO netestovane
	//vypise pouzity obdelnik z big_grid
	int up=0,down=BIG_GRID_SIZE,left=0,right=BIG_GRID_SIZE;
	for(int tmp=1; tmp && up < BIG_GRID_SIZE; up+=tmp) //to je trosku uchylne ;-) posouvato up az do casti kde neco je
		for (int i = 0; i < BIG_GRID_SIZE; i++)
			if ( big_grid(up,i) > 0 )
				tmp--;
	for(int tmp=1; tmp && down > 0; down-=tmp)
		for (int i = 0; i < BIG_GRID_SIZE; i++)
			if ( big_grid(down-1,i) > 0 )
				tmp--;
	for(int tmp=1; tmp && left < BIG_GRID_SIZE; left+=tmp)
		for (int i = up; i < down; i++)
			if ( big_grid(i,left) > 0 )
				tmp--;
	for(int tmp=1; tmp && right > 0; right-=tmp)
		for (int i = up; i < down; i++)
			if ( big_grid(i,right-1) > 0 )
				tmp--;
	//printf("%d %d %d %d\n",up,down,left,right);
	
	FILE *F = fopen(name,"w");
	for (int i = up; i < down; i++){
		for (int r = 0; r < 64; r++){
			for (int j = left; j < right; j++){
				int start = 64*big_grid(i,j);
				for (int s = 0; s < 64; s++)
					fprintf(F,"%d", !!(grid_heap[start+r]&(1ULL<<s)) );
			}
			fprintf(F,"\n");
		}
	}
	fclose(F);
}

static inline void create(int i, int j){
	if ((i>0) && (i+1<BIG_GRID_SIZE) && (j>0) && (j+1<BIG_GRID_SIZE)) {
		if (grid_heap_count >= GRID_HEAP_SIZE)
			grid_heap_count = 1;
		while (grid_heap_[grid_heap_count]){
			grid_heap_count++;
			if (grid_heap_count >= GRID_HEAP_SIZE)
				grid_heap_count = 1;
		}
		big_grid_new(i,j) = grid_heap_count;
		grid_heap_[grid_heap_count]=1;
		for (int i = 0; i<64; i++)
			grid_heap[grid_heap_count+i]=0;
		grid_heap_count++;

	} else {
		//TODO tady by se mela delat nova velka mrizka
		printf("prelezeny okraje velke mrizky\n");
		exit(1);
	}
}

static inline LI gridf(int i, int j, int row){
	if ((i>0) && (i+1<BIG_GRID_SIZE) && (j>0) && (j+1<BIG_GRID_SIZE)){
		return 0ULL;
	}	
	return grid_heap[64*big_grid(i,j)+row];
}

static inline void count_line(LI line, int where[64],LI mask, int l, int r){
	//kolik je v prvnich trech bitech jednicek
	static int table_of_count[8] = {0,1,1,2,1,2,2,3};

	where[0] += table_of_count[(l+(line<<1)) & mask];
	for (int i = 0; i+2 < 64; i++){
		where[i+1] += table_of_count[(line>>i) & mask];
	}
	where[63] += table_of_count[((r<<2)+(line>>62)) & mask];
}

void step_grid(int exist, LI grid[64], int y, int x){
	//nejdriv budu do pom pristitavat kolik je v okoli a pak to 
	//presisu. Nebylo by rychlejsi to rovnou pocitat?
	if (exist == 0)
		return; //tato mrizka je cela nulova a i okoli je nulove
	int pom[64][64];
	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++)
			pom[i][j]=0;

	int l,r; //bity na levo a na pravo od daneho radku

	l = !!(gridf(x,y-1,0) & (1ULL<<63));
	r = !!(gridf(x,y+1,0) & (1ULL<<0));
	count_line(gridf(x-1,y,63),pom[0],7ULL,l,r);	
	count_line(grid[0],pom[0],5ULL,l,r);	
	count_line(grid[1],pom[0],7ULL,l,r);
	for (int i = 1; i < 63; i++){ //i je radek
		l = !!(gridf(x,y-1,i) & (1ULL<<63));	
		r = !!(gridf(x,y+1,i) & (1ULL<<0));
		count_line(grid[i-1],pom[i],7ULL,l,r);	
		count_line(grid[i],  pom[i],5ULL,l,r);	
		count_line(grid[i+1],pom[i],7ULL,l,r);
	}
	l = !!(gridf(x,y-1,63) & (1ULL<<63));	
	r = !!(gridf(x,y+1,63) & (1ULL<<0));	
	count_line(grid[62],pom[63],7ULL,l,r);	
	count_line(grid[63],pom[63],5ULL,l,r);
	count_line(gridf(x+1,y,0),pom[63],7ULL,l,r);	

/*	
	printf("\n");
	for (int i = 0; i < 20; i++){
		for (int j = 0; j < 64; j++){
			printf("%d",pom[i][j]);
		} printf("\n");
	} printf("\n");
	printf("\n");
*/

	//vytvor novy
	create(x,y);
	LI *new = &grid_heap[ 64*big_grid_new(x,y) ];
	//TODO vytvaret nove mrizky okolo
	for (int i = 0; i < 64; i++){
		for (int j = 0; j < 64; j++){
			if ( grid[i] & (1ULL<<j) ){ //ziva
				if ( pom[i][j]==2 || pom[i][j]==3 ){
					new[i] |= (1ULL<<j); //zustava
				}
			} else { //mrtva
				if ( pom[i][j]==3){
					new[i] |= (1ULL<<j); //narodi se
				} 
			}
		}
	}

}

void init_big_grid(){
	big_grid_1 = big_grid_tmp1;
	big_grid_new_1 = big_grid_tmp2;
}

void swap_big_grid(){
	int *tmp = big_grid_1;
	big_grid_1 = big_grid_new_1;
	big_grid_new_1 = tmp;
}

void step(){
	//nedriv nove vypocitam vnitrky,
	for (int i = 0; i < BIG_GRID_SIZE; i++){
		for (int j = 0; j < BIG_GRID_SIZE; j++)
				step_grid( 	big_grid(i,j),
						&grid_heap[64*big_grid(i,j)],
						i,j);
	}
	//pak ovolnim stare
/*	for (int i = 0; i < BIG_GRID_SIZE; i++){
		for (int j = 0; j < BIG_GRID_SIZE; j++){
				grid_heap_[ 64*big_grid(i,j) ] = 0;
				big_grid(i,j) = 0;
		}
	}
*/	//prehodim nove do stareho
	swap_big_grid();
}

void help(){
	printf("pouziti: \n./program-opt jmeno_vstupniho_souboru pocet_interaci jmeno_vystupniho_souboru\n");
	exit(0);	
}

int main(int argc, char *argv[]) {

	init_big_grid();

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
	if (x>64 || y>64){
		printf("TODO vetsi mrizky\n");
		exit(1);	
	}

	create(BIG_GRID_SIZE/2,BIG_GRID_SIZE/2);
	LI *new = &grid_heap[ 64*big_grid_new(BIG_GRID_SIZE/2,BIG_GRID_SIZE/2) ];

	fgetc(F);
	for (int i = 0; i < x; i++){
		for (int j = 0; j < y; j++){
			if ( fgetc(F) == '1')
				new[i] |= 1ULL<<j;
		}
		fgetc(F);
	}
	fclose(F);

	swap_big_grid();

	for( int i = 0; i < atoi(argv[2]); i++){
		step();
	}

	print_big_grid_to_file(argv[3]);

	return 0;
}
