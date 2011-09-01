#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#define LI uint64_t
#define WRITE_EVERY_STEP 0
#define GRID_HEAP_SIZE (1<<13) //kolik malich mrizek si celkove pamatuju (1<<19 zabere 256MiB)

//==================================== zasobarna malich mrizek
LI grid_heap[64*GRID_HEAP_SIZE];
int grid_heap_count=1; //kde se bude psat dalsi (pokud to jde)
int grid_heap_free=GRID_HEAP_SIZE-1; //kolik je volnych
int grid_heap_[GRID_HEAP_SIZE]; //kde je obsazeno


//==================================== struktura kde jsou male mrizky

//tyhle by se nemeli z venku pouzivat
int *big_grid_1; 
int *big_grid_new_1;
int *big_grid_2; 
int *big_grid_new_2;

int big_grid_left = -5; //kde jeste je ctverec
int big_grid_right = 5; //prvni kde neni
int big_grid_up = -5; //kde jeste je ctverec
int big_grid_down = 5; //prvni kde neni

static inline int big_grid(int x, int y){
       return big_grid_1[ (x-big_grid_up) * (big_grid_right-big_grid_left) + (y-big_grid_left) ];
}

static inline int big_grid_new(int x, int y){
       return big_grid_new_1[ (x-big_grid_up) * (big_grid_right-big_grid_left) + (y-big_grid_left) ];
}

void init_big_grid(){
	big_grid_1 = calloc( (big_grid_down-big_grid_up) * (big_grid_right-big_grid_left), sizeof(int) );
	big_grid_new_1 = calloc( (big_grid_down-big_grid_up) * (big_grid_right-big_grid_left), sizeof(int) );
}

static inline void swap_big_grid(){
	for (int i = big_grid_up; i < big_grid_down; i++){
		for (int j = big_grid_left; j < big_grid_right; j++){
				grid_heap_[ big_grid(i,j) ] = 0;
				grid_heap_free++;
				big_grid_1[ (i-big_grid_up) * (big_grid_right-big_grid_left) + (j-big_grid_left)  ]=0;
		}
	}
	int *tmp = big_grid_1;
	big_grid_1 = big_grid_new_1;
	big_grid_new_1 = tmp;
}

static inline int create(int i, int j){
	if ((i<big_grid_up) || (i>=big_grid_down) || (j<big_grid_left) || (j>=big_grid_right)){
		//nove rozmery
		int up = big_grid_up;
		int down = big_grid_down;
		int left = big_grid_left;
		int right = big_grid_right;
		if (i < big_grid_up) 
			up -= big_grid_down - big_grid_up;
		if (i >= big_grid_down) 
			down += big_grid_down - big_grid_up;
		if (j < big_grid_left) 
			left -= big_grid_right - big_grid_left;
		if (j >= big_grid_right)
			right += big_grid_right - big_grid_left;
		
		//alokoce
		big_grid_2 = calloc( (down-up) * (right-left), sizeof(int) );
		big_grid_new_2 = calloc( (down-up) * (right-left), sizeof(int) );
		
		//prehozeni
		for (int i = big_grid_up; i < big_grid_down; i++)
			for (int j = big_grid_left; j < big_grid_right; j++){
				big_grid_2[ (i-up) * (right-left) + (j-left)  ] = big_grid(i,j);
				big_grid_new_2[ (i-up) * (right-left) + (j-left)  ] = big_grid_new(i,j);
			}
		big_grid_up = up;
		big_grid_down = down;
		big_grid_left = left;
		big_grid_right = right;
		free(big_grid_1);
		free(big_grid_new_1);
		big_grid_1 = big_grid_2;
		big_grid_new_1 = big_grid_new_2;
	}


	if (big_grid_new(i,j) > 0) //mrizka je uz vytvorena
		return big_grid_new(i,j);

	//tvorim novou mrizku
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
	big_grid_new_1[ (i-big_grid_up) * (big_grid_right-big_grid_left) + (j-big_grid_left) ] = grid_heap_count;
	grid_heap_free--;
	grid_heap_[grid_heap_count]=1;
	for (int i = 0; i<64; i++)
		grid_heap[64*grid_heap_count+i]=0ULL;
	return grid_heap_count++;

}

static inline LI grid_row(int i, int j, int row){
	if ((i>=big_grid_up) && (i<big_grid_down) && (j>=big_grid_left) && (j<=big_grid_right)){
		return grid_heap[64*big_grid(i,j)+row];
	}
	return 0ULL;
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
	int up = big_grid_up; //nejhorneji neprazdny radek
	for(int tmp=1; tmp; up+=tmp) //to je trosku uchylne ;-) posouvato up az do casti kde neco je
		for (int i = big_grid_left; i < big_grid_right; i++)
			if ( big_grid(up,i) > 0 ){
				tmp--;
				break;
			}
	int down = big_grid_down; //pod nejspodnejsim neprazdnym radkem
	for(int tmp=1; tmp; down-=tmp)
		for (int i = big_grid_left; i < big_grid_right; i++)
			if ( big_grid(down-1,i) > 0 ){
				tmp--;
				break;
			}
	int left = big_grid_left;
	for(int tmp=1; tmp; left+=tmp)
		for (int i = up; i < down; i++)
			if ( big_grid(i,left) > 0 ){
				tmp--;
				break;
			}
	int right = big_grid_right;
	for(int tmp=1; tmp; right-=tmp)
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
				LI row = grid_row(i,j,r);
				for (int s = 0; s < 64; s++)
					fprintf(F,"%d", !!(row&(1ULL<<s)) );
			}
			fprintf(F,"\n");
		}
	}
	fclose(F);
}

//==================================== a konecne samotne pocitani

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
	l = !!(grid_row(x-1,y-1,63) & (1ULL<<63));
	r = !!(grid_row(x-1,y+1,63) & (1ULL<<0));
	
	count_line(grid_row(x-1,y,63),pom[0],7ULL,l,r);	
	l = !!(grid_row(x,y-1,0) & (1ULL<<63));
	r = !!(grid_row(x,y+1,0) & (1ULL<<0));
	count_line(grid[0],pom[0],5ULL,l,r);	
	count_line(grid[0],pom[1],7ULL,l,r);
	for (int i = 1; i < 63; i++){ //i je radek
		l = !!(grid_row(x,y-1,i) & (1ULL<<63));	
		r = !!(grid_row(x,y+1,i) & (1ULL<<0));
		count_line(grid[i],pom[i-1],7ULL,l,r);	
		count_line(grid[i],pom[i],  5ULL,l,r);	
		count_line(grid[i],pom[i+1],7ULL,l,r);
	}
	l = !!(grid_row(x,y-1,63) & (1ULL<<63));	
	r = !!(grid_row(x,y+1,63) & (1ULL<<0));	
	count_line(grid[63],pom[62],7ULL,l,r);	
	count_line(grid[63],pom[63],5ULL,l,r);
	l = !!(grid_row(x+1,y-1,0) & (1ULL<<63));	
	r = !!(grid_row(x+1,y+1,0) & (1ULL<<0));	
	count_line(grid_row(x+1,y,0),pom[63],7ULL,l,r);	

	//vykresleni mrizky
	LI *new = &grid_heap[ 64*create(x,y) ];
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
		//binary_luint(grid_row(x+1,y,0));
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
	for (int i = big_grid_up; i < big_grid_down; i++){
		for (int j = big_grid_left; j < big_grid_right; j++)
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
	printf("pozor na okrajich by standartne meli byt nuly,\nzatim je dovolen vstup pouze do velikosti 64x64\n");
	printf("vystup se da prohlizet pomoci view.sh nebo jako pbm obrazek\n");
	printf("pozor mozna by meli byt kratsi radky pro pbm, ale me to funguje\n");
	exit(0);	
}

void put_pixel(int i,int j){
	int x,y;
	if (i<0) {
		x = -( (-i) / 64 + 1 );
		i = (64 - ((-i) % 64)) % 64;
	} else {
		x = i / 64;
		i = i % 64;
	}

	if (j<0) {
		y = -( (-j) / 64 + 1 );
		j = (64 - ((-j) % 64)) % 64;
	} else {
		y = j / 64;
		j = j % 64;
	}
	LI *new = &grid_heap[ 64*create(x,y) ];
	new[i] |= (1ULL<<j); //zustava
}

typedef int_fast64_t timestamp_t;

static timestamp_t get_timer(void) {
	struct timeval t;
	gettimeofday(&t, NULL);
	return 1000000*t.tv_sec + t.tv_usec;
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

	if ( 0 < x && x < 64 && 0 < y && y < 64) {
		//maly vstup
		LI *new = &grid_heap[ 64*create(0,0) ];
		fgetc(F);
		for (int i = 0; i < x; i++){
			for (int j = 0; j < y; j++){
				if ( fgetc(F) == '1')
					new[i] |= 1ULL<<j;
			}
			fgetc(F);
		}

	} else if (x==0 && y==0) {
		//velky vstup
		int i,j;
		while (EOF != fscanf(F,"%d%d",&i,&j)){
			put_pixel(-i,-j);
		}
	} else {
		printf("maly vstupni soubor: pocet_sloupcu(<64) pocet_radku(<64) \\n tabulka\n");
		printf("velky vstupni soubor: 0 0 \\n co radek to x-ova a y-ova souradnice\n");
		fclose(F);
		exit(1);	
	}

	fclose(F);
	swap_big_grid();

	timestamp_t t0 = get_timer();

	for( int i = 0; i < atoi(argv[2]); i++)
		step(argv[3]);

	t0 = get_timer() - t0;
	printf("%d\n",(int) lround(1000*(t0/1e6)));

	print_big_grid_to_file(argv[3]);

	return EXIT_SUCCESS;
}
