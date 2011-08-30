#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#define LI uint64_t
#define WRITE_EVERY_STEP 0
#define GRID_HEAP_SIZE (1<<7) //kolik malich mrizek si celkove pamatuju (1<<19 zabere 256MiB)

//==================================== zasobarna malich mrizek
LI grid_heap[64*GRID_HEAP_SIZE];
int grid_heap_count=1;
int grid_heap_free=GRID_HEAP_SIZE-1;
int grid_heap_[GRID_HEAP_SIZE];


//==================================== struktura kde jsou male mrizky
int big_grid_size=10; //kolik mrizek 64x64 pocita jedno vlakno

int *big_grid_1; 
int *big_grid_new_1;

static inline int big_grid(int x, int y){
       return big_grid_1[x*big_grid_size+y];
}

static inline int big_grid_new(int x, int y){
       return big_grid_new_1[(x)*big_grid_size+(y)];
}

void init_big_grid(){
	big_grid_1 = calloc(big_grid_size*big_grid_size,sizeof(int));
	big_grid_new_1 = calloc(big_grid_size*big_grid_size,sizeof(int));
}

static inline void swap_big_grid(){
	for (int i = 0; i < big_grid_size; i++){
		for (int j = 0; j < big_grid_size; j++){
				grid_heap_[ big_grid(i,j) ] = 0;
				grid_heap_free++;
				big_grid_1[i*big_grid_size+j]=0;
		}
	}
	int *tmp = big_grid_1;
	big_grid_1 = big_grid_new_1;
	big_grid_new_1 = tmp;
}

static inline void create(int i, int j){
	if ((i>=0) && (i<big_grid_size) && (j>=0) && (j<big_grid_size)) {
		if (big_grid_new(i,j) > 0)
			return; //mrizka je uz vytvorena
		if (grid_heap_free <= 0){
			printf("doslo misto\n");
			exit(1);
		}
		while (grid_heap_[grid_heap_count] || grid_heap_count > GRID_HEAP_SIZE){
			//dokud nejsem nekde kde je volno
			grid_heap_count++;
			if (grid_heap_count >= GRID_HEAP_SIZE)
				grid_heap_count = 1;
		}
		big_grid_new_1[i*big_grid_size+j] = grid_heap_count;
		grid_heap_free--;
		grid_heap_[grid_heap_count]=1;
		for (int i = 0; i<64; i++)
			grid_heap[64*grid_heap_count+i]=0ULL;
		grid_heap_count++;

	} else {
		//TODO tady by se mela delat nova velka mrizka
		printf("prelezeny okraje velke mrizky %d %d \n",i,j);
		exit(1);
	}
}


//==================================== ladici smeti
int step_number = 0;

static inline void binary_luint(LI n){
	for (int i = 0; i < 64; i++){
		if (i%5==0)
			printf(" ");
		printf("%d", !!(n&(1llu<<i)) );
	}
	printf("\n");
}

void print_grid(LI grid[64]){
	for (int i = 0; i < 64; i++){
		for (int j = 0; j < 64; j++)
			printf("%d",!!(grid[i]&(1ULL<<j)));
		printf("\n");
	}
}

void print_pom(int pom[64][64]){
	for (int i = 0; i < 64; i++){
		for (int j = 0; j < 64; j++){
			printf("%d",pom[i][j]);
		} printf("\n");
	} 
}


void print_big_grid_to_file(char *name){
	//vypise pouzity obdelnik z big_grid
	int up=0,down=big_grid_size,left=0,right=big_grid_size;
	for(int tmp=1; tmp && up < big_grid_size; up+=tmp) //to je trosku uchylne ;-) posouvato up az do casti kde neco je
		for (int i = 0; i < big_grid_size; i++)
			if ( big_grid(up,i) > 0 ){
				tmp--;
				break;
			}
	for(int tmp=1; tmp && down > 0; down-=tmp)
		for (int i = 0; i < big_grid_size; i++)
			if ( big_grid(down-1,i) > 0 ){
				tmp--;
				break;
			}
	for(int tmp=1; tmp && left < big_grid_size; left+=tmp)
		for (int i = up; i < down; i++)
			if ( big_grid(i,left) > 0 ){
				tmp--;
				break;
			}
	for(int tmp=1; tmp && right > 0; right-=tmp)
		for (int i = up; i < down; i++)
			if ( big_grid(i,right-1) > 0 ){
				tmp--;
				break;
			}
	//printf("%d %d %d %d\n",up,down,left,right);
	
	char n[200];
	sprintf(n,"%s%03d.pbm",name,step_number);
	FILE *F = fopen(n,"w");
	fprintf(F,"P1\n");
	fprintf(F,"%d %d\n",64*(right-left),64*(down-up));
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

//==================================== a konecne samotne pocitani

static inline LI gridf(int i, int j, int row){
	if ((i>=0) && (i<big_grid_size) && (j>=0) && (j<=big_grid_size)){
		return grid_heap[64*big_grid(i,j)+row];
	}
	return 0ULL;
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

void step_grid(int exist, LI grid[64], int x, int y){
	//nejdriv budu do pom pristitavat kolik je v okoli a pak to 
	//presisu. Nebylo by rychlejsi to rovnou pocitat?
	if (exist == 0)
		return; //tato mrizka je cela nulova a i okoli je nulove

	int pom[64][64];
	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++)
			pom[i][j]=0;

	//scitaji se zive bunky okolo
	int l,r; //bity na levo a na pravo od daneho radku
	l = !!(gridf(x-1,y-1,63) & (1ULL<<63));
	r = !!(gridf(x-1,y+1,63) & (1ULL<<0));
	
	count_line(gridf(x-1,y,63),pom[0],7ULL,l,r);	
	l = !!(gridf(x,y-1,0) & (1ULL<<63));
	r = !!(gridf(x,y+1,0) & (1ULL<<0));
	count_line(grid[0],pom[0],5ULL,l,r);	
	count_line(grid[0],pom[1],7ULL,l,r);
	for (int i = 1; i < 63; i++){ //i je radek
		l = !!(gridf(x,y-1,i) & (1ULL<<63));	
		r = !!(gridf(x,y+1,i) & (1ULL<<0));
		count_line(grid[i],pom[i-1],7ULL,l,r);	
		count_line(grid[i],pom[i],  5ULL,l,r);	
		count_line(grid[i],pom[i+1],7ULL,l,r);
	}
	l = !!(gridf(x,y-1,63) & (1ULL<<63));	
	r = !!(gridf(x,y+1,63) & (1ULL<<0));	
	count_line(grid[63],pom[62],7ULL,l,r);	
	count_line(grid[63],pom[63],5ULL,l,r);
	l = !!(gridf(x+1,y-1,0) & (1ULL<<63));	
	r = !!(gridf(x+1,y+1,0) & (1ULL<<0));	
	count_line(gridf(x+1,y,0),pom[63],7ULL,l,r);	

	//vykresleni mrizky
	create(x,y);
	LI *new = &grid_heap[ 64*big_grid_new(x,y) ];
	int create_left = 0, create_right = 0;

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
		if ( new[i] & (1ULL) )
			create_left++;
		if ( new[i] & (1ULL<<63) )
			create_right++;
	}

	//zalozeni novych mrizek v okoli je-li potreba	
	if (new[0])
		create(x-1,y);
	if (new[63])
		create(x+1,y);
	if (create_left)
		create(x,y-1);
	if (create_right)
		create(x,y+1);

#if 0
	if (step_number == 1){
		printf("%d %d %d\n",step_number, x,y);
		print_grid(new);
		printf("\n");
	}


	if (step_number == 2 && x == 4 && y==5){
		printf("%d %d %d\n",step_number, x,y);
		//binary_luint(gridf(x+1,y,0));
		print_pom(pom);
		printf("\n");
		print_grid(new);
		printf("\n");
	}
#endif
}

void step(char *fuj){
	step_number++;
	//vypocitam vnitrky
	for (int i = 0; i < big_grid_size; i++){
		for (int j = 0; j < big_grid_size; j++)
				step_grid( 	big_grid(i,j),
						&grid_heap[64*big_grid(i,j)],
						i,j);
	}

	//prehodim nove do stareho a stare vycistim
	swap_big_grid();
	if (WRITE_EVERY_STEP)
		print_big_grid_to_file(fuj);
}

void help(){
	printf("pouziti: \n./program-opt jmeno_vstupniho_souboru pocet_interaci jmeno_vystupniho_souboru\n");
	printf("vstupni soubor: pocet_radku pocet_sloupcu \\n tabulka\n");
	printf("pozor na okrajich by standartne meli byt nuly,\nzatim je dovolen vstup pouze do velikosti 64x64\n");
	printf("vystup se da prohlizet pomoci view.sh nebo jako pbm obrazek\n");
	printf("pozor mozna by meli byt kratsi radky pro pbm, ale me to funguje\n");
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
	fscanf(F,"%d%d",&y,&x);
	if (x>64 || y>64){
		printf("TODO vetsi mrizky\n");
		exit(1);	
	}

	create(big_grid_size/2,big_grid_size/2);
	LI *new = &grid_heap[ 64*big_grid_new(big_grid_size/2,big_grid_size/2) ];

	fgetc(F);
	for (int i = 0; i < x; i++){
		for (int j = 0; j < y; j++){
			if ( fgetc(F) == '1')
				new[i] |= 1ULL<<j;
		}
		fgetc(F);
	}
	fclose(F);

//	print_grid(new);

	swap_big_grid();

	for( int i = 0; i < atoi(argv[2]); i++){
//		printf("step%d\n",i);
		step(argv[3]);
	}

	print_big_grid_to_file(argv[3]);

	return 0;
}
