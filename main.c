#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <math.h>
#include <sys/time.h>
#include <pthread.h>
#define LI uint64_t
#define uint unsigned int
//#define WRITE_EVERY_STEP
//#define DELETE_EMPTY_GRID 
//#define SIMPLE_THREAD 
#define COMPLICATED_THREAD
#define THREAD_NUMBER 2
#define OLD_STEP_GRID
#define GRID_HEAP_SIZE (1<<13) //kolik malich mrizek si celkove pamatuju (1<<19 zabere 256MiB)

#if ( defined SIMPLE_THREAD || defined COMPLICATED_THREAD)
pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;
#endif

//==================================== zasobarna malich mrizek
LI grid_heap[64*GRID_HEAP_SIZE];
uint grid_heap_count=1; //kde se bude psat dalsi (pokud to jde)
int grid_heap_free=GRID_HEAP_SIZE-1; //kolik je volnych
int grid_heap_[GRID_HEAP_SIZE]; //kde je obsazeno



//==================================== struktura kde jsou male mrizky

//tyhle by se nemeli z venku pouzivat
uint *big_grid_1; 
uint *big_grid_new_1;
uint *big_grid_new_2;

int big_grid_left = -1; //kde jeste je ctverec
int big_grid_right = 1; //prvni kde neni
int big_grid_up = -1; //kde jeste je ctverec
int big_grid_down = 1; //prvni kde neni

int big_grid_left_new = -1; //kde jeste je ctverec
int big_grid_right_new = 1; //prvni kde neni
int big_grid_up_new = -1; //kde jeste je ctverec
int big_grid_down_new = 1; //prvni kde neni

static inline uint big_grid(int x, int y){
       return big_grid_1[ (x-big_grid_up) * (big_grid_right-big_grid_left) + (y-big_grid_left) ];
}

static inline uint big_grid_new(int x, int y){
       return big_grid_new_1[ (x-big_grid_up_new) * (big_grid_right_new-big_grid_left_new) + (y-big_grid_left_new) ];
}

void init_big_grid(){
	big_grid_1 = calloc( (big_grid_down-big_grid_up) * (big_grid_right-big_grid_left), sizeof(uint) );
	big_grid_new_1 = calloc( (big_grid_down-big_grid_up) * (big_grid_right-big_grid_left), sizeof(uint) );
}

static inline void swap_big_grid(){
	//stare vymazu
	for (int i = big_grid_up; i < big_grid_down; i++){
		for (int j = big_grid_left; j < big_grid_right; j++){
			if (big_grid(i,j)) {
				grid_heap_[ big_grid(i,j) ] = 0;
				grid_heap_free++;
				big_grid_1[ (i-big_grid_up) * (big_grid_right-big_grid_left) + (j-big_grid_left)  ]=0;
			}
		}
	}
	//a prehodim
	uint *tmp = big_grid_1;
	big_grid_1 = big_grid_new_1;
	big_grid_new_1 = tmp;
	{int p=big_grid_left; big_grid_left=big_grid_left_new; big_grid_left_new=p;}
	{int p=big_grid_right; big_grid_right=big_grid_right_new; big_grid_right_new=p;}
	{int p=big_grid_up; big_grid_up=big_grid_up_new; big_grid_up_new=p;}
	{int p=big_grid_down; big_grid_down=big_grid_down_new; big_grid_down_new=p;}
}

static inline void delete(int x, int y){
	#if ( defined SIMPLE_THREAD || defined COMPLICATED_THREAD)
	//zamykame kvuli pouziti big_grid_*_new ktere se muze mnenit v create
	if (pthread_mutex_lock(&mutex))
		perror("problem_mutex1");
	#endif
	grid_heap_[ big_grid_new(x,y)] = 0; //reknu ze na to misto se muze psat
	grid_heap_free++; //ze je o misto vic
	big_grid_new_1[ (x-big_grid_up_new) * (big_grid_right-big_grid_left_new) + (y-big_grid_left_new) ] = 0; //a ze mrizka x y neexistuje
	#if ( defined SIMPLE_THREAD || defined COMPLICATED_THREAD)
	if (pthread_mutex_unlock(&mutex))
		perror("problem_mutex2");
	#endif
}

static inline int create(int i, int j){

	#if ( defined SIMPLE_THREAD || defined COMPLICATED_THREAD)
	if (pthread_mutex_lock(&mutex))
		perror("problem_mutex3");
	#endif

	//zvetsim rozmery velke mrizky
	while ((i<big_grid_up_new) || (i>=big_grid_down_new) || (j<big_grid_left_new) || (j>=big_grid_right_new)){
		//nove rozmery
		int up = big_grid_up_new;
		int down = big_grid_down_new;
		int left = big_grid_left_new;
		int right = big_grid_right_new;
		if (i < big_grid_up_new) 
			up -= big_grid_down_new - big_grid_up_new;
		if (i >= big_grid_down_new) 
			down += big_grid_down_new - big_grid_up_new;
		if (j < big_grid_left_new) 
			left -= big_grid_right_new - big_grid_left_new;
		if (j >= big_grid_right_new)
			right += big_grid_right_new - big_grid_left_new;
		
		//alokoce
		big_grid_new_2 = calloc( (down-up) * (right-left), sizeof(uint) );
		
		//prehozeni
		for (int i = big_grid_up_new; i < big_grid_down_new; i++)
			for (int j = big_grid_left_new; j < big_grid_right_new; j++){
				big_grid_new_2[ (i-up) * (right-left) + (j-left)  ] = big_grid_new(i,j);
			}
		big_grid_up_new = up;
		big_grid_down_new = down;
		big_grid_left_new = left;
		big_grid_right_new = right;
		free(big_grid_new_1);
		big_grid_new_1 = big_grid_new_2;
				
	}


	if (big_grid_new(i,j) > 0){ //mrizka je uz vytvorena
		#if ( defined SIMPLE_THREAD || defined COMPLICATED_THREAD)
		if (pthread_mutex_unlock(&mutex))
			perror("problem_mutex4");
		#endif
		return big_grid_new(i,j);
	}

	//tvorim novou mrizku
	if (grid_heap_free <= 0){
		perror("doslo misto");
		exit(1);
	}

	while (grid_heap_[grid_heap_count] || grid_heap_count >= GRID_HEAP_SIZE){
		//dokud nejsem nekde kde je volno
		grid_heap_count++;
		if (grid_heap_count >= GRID_HEAP_SIZE)
			grid_heap_count = 1;
	}
	big_grid_new_1[ (i-big_grid_up_new) * (big_grid_right_new-big_grid_left_new) + (j-big_grid_left_new) ] = grid_heap_count;
	grid_heap_free--;
	grid_heap_[grid_heap_count]=1;
	for (int i = 0; i<64; i++)
		grid_heap[64*grid_heap_count+i]=0ULL;
	int result = grid_heap_count;
	grid_heap_count++;

	#if ( defined SIMPLE_THREAD || defined COMPLICATED_THREAD)
	if (pthread_mutex_unlock(&mutex))
		perror("problem_mutex5");
	#endif
	return result;

}

static inline LI get_grid_row(int i, int j, int row){
	if ((i>=big_grid_up) && (i<big_grid_down) && (j>=big_grid_left) && (j<=big_grid_right)){
		//TODO
		int p=big_grid(i,j);
		return grid_heap[64*p+row];
	}
	return 0ULL;
}

//==================================== ladici smeti
int step_number = 0;

static inline void binary_luint(LI n){
	for (int i = 0; i < 64; i++){
		if (i%5==0)
			fprintf(stderr," ");
		fprintf(stderr,"%d", !!(n&(1llu<<i)) );
	}
	fprintf(stderr,"\n");
}

void print_grid(LI grid[64]){
	for (int i = 0; i < 64; i++){
		for (int j = 0; j < 64; j++)
			fprintf(stderr,"%d",!!(grid[i]&(1ULL<<j)));
		fprintf(stderr,"\n");
	}
}

void print_pom(int pom[64][64]){
	for (int i = 0; i < 64; i++){
		for (int j = 0; j < 64; j++){
			fprintf(stderr,"%d",pom[i][j]);
		} fprintf(stderr,"\n");
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
	//fprintf(stderr,"%d %d %d %d\n",up,down,left,right);
	
	char n[200];
	sprintf(n,"%s%03d.pbm",name,step_number);
	FILE *F = fopen(n,"w");
	fprintf(F,"P1\n");
	fprintf(F,"%d %d\n",64*(right-left),64*(down-up));
	for (int i = up; i < down; i++){
		for (int r = 0; r < 64; r++){
			for (int j = left; j < right; j++){
				LI row = get_grid_row(i,j,r);
				for (int s = 0; s < 64; s++)
					fprintf(F,"%d", !!(row&(1ULL<<s)) );
			}
			fprintf(F,"\n");
		}
	}
	fclose(F);
}

//==================================== samotne pocitani

#ifdef OLD_STEP_GRID
static inline void count_line(LI line, int where[64],LI mask, int l, int r){
	
	//kolik je v prvnich trech bitech jednicek
	static int table_of_count[8] = {0,1,1,2,1,2,2,3};

	//tohle se samo nerozbali je potreba pouzit -funroll-loops!!!
	//ale valgrind se tvari ze to v te smice porad travi cas 5%
	//a pritom po pouziti tohohle se zkratil cas o pul sekundy
	where[0] += table_of_count[(l+(line<<1)) & mask];
	for (int i = 0; i+2 < 64; i++){
		where[i+1] += table_of_count[(line>>i) & mask];
	}
	where[63] += table_of_count[((r<<2)+(line>>62)) & mask];
}

void step_grid(uint exist, LI grid[64], int x, int y){
	//nejdriv budu do pom pristitavat kolik je v okoli a pak to 
	//presisu.
	if (exist == 0)
		return; //tato mrizka je cela nulova a i okoli je nulove

	int pom[64][64];
	for (int i = 0; i < 64; i++)
		for (int j = 0; j < 64; j++)
			pom[i][j]=0;

	//scitaji se zive bunky okolo
	int l,r; //bity na levo a na pravo od daneho radku
	l = !!(get_grid_row(x-1,y-1,63) & (1ULL<<63));
	r = !!(get_grid_row(x-1,y+1,63) & (1ULL<<0));
	
	count_line(get_grid_row(x-1,y,63),pom[0],7ULL,l,r);	
	l = !!(get_grid_row(x,y-1,0) & (1ULL<<63));
	r = !!(get_grid_row(x,y+1,0) & (1ULL<<0));
	count_line(grid[0],pom[0],5ULL,l,r);	
	count_line(grid[0],pom[1],7ULL,l,r);
	for (int i = 1; i < 63; i++){ //i je radek
		l = !!(get_grid_row(x,y-1,i) & (1ULL<<63));	
		r = !!(get_grid_row(x,y+1,i) & (1ULL<<0));
		count_line(grid[i],pom[i-1],7ULL,l,r);	
		count_line(grid[i],pom[i],  5ULL,l,r);	
		count_line(grid[i],pom[i+1],7ULL,l,r);
	}
	l = !!(get_grid_row(x,y-1,63) & (1ULL<<63));	
	r = !!(get_grid_row(x,y+1,63) & (1ULL<<0));	
	count_line(grid[63],pom[62],7ULL,l,r);	
	count_line(grid[63],pom[63],5ULL,l,r);
	l = !!(get_grid_row(x+1,y-1,0) & (1ULL<<63));	
	r = !!(get_grid_row(x+1,y+1,0) & (1ULL<<0));	
	count_line(get_grid_row(x+1,y,0),pom[63],7ULL,l,r);	

	//vykresleni mrizky
	LI *new = &grid_heap[ 64*create(x,y) ];
	int create_left = 0, create_right = 0;

	for (int i = 0; i < 64; i++){
		//pouziti tmp pomohlo na jednovlaknovem o 50ms!!!
		//nasledujici instrukce bere 16% casu. Proboha proc?? vzdit je pouzita i nahore
		LI tmp = grid[i];
		for (int j = 0; j < 64; j++){
// stara verze	je o 120ms rychlejsi 	
/*			if ( tmp & (1ULL<<j) ){ //ziva
				if ( pom[i][j]==2 || pom[i][j]==3 ){
					new[i] |= (1ULL<<j); //zustava
				}
			} else { //mrtva
				if ( pom[i][j]==3){
					new[i] |= (1ULL<<j); //narodi se
				} 
			}
*//* 			//to prvni 19 to druhe 9
			new[i] |= ((LI) ((tmp&1ULL<<j)&&(pom[i][j]==2||pom[i][j]==3)) )<<j;
			new[i] |= ((LI) ((!(tmp&1ULL<<j))&&(pom[i][j]==3)) )<<j;
*/			//nasledujci hverze vyuziva toho ze to ta prvni je rychlejsi nez druha
			//zlepsila o dalsi pulsekundu!!!!
			if ( (pom[i][j]==3) || ((pom[i][j]==2) && (tmp & (1ULL<<j))) )
					new[i] |= (1ULL<<j);
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
		fprintf(stderr,"%d %d %d\n",step_number, x,y);
		print_grid(new);
		fprintf(stderr,"\n");
	}


	if (step_number == 2 && x == 4 && y==5){
		fprintf(stderr,"%d %d %d\n",step_number, x,y);
		//binary_luint(get_grid_row(x+1,y,0));
		print_pom(pom);
		fprintf(stderr,"\n");
		print_grid(new);
		fprintf(stderr,"\n");
	}
#endif
}

#else //OLD_STEP_GRID

static LI step_cell[1<<9];
	//vrati 1 jestli ctverecek zije
	//prvni bit je samotny ctverecek 
	//a dalsich 8 je jeho okoli

static inline void step_cell_init(){
	for (LI v = 0; v < (1<<9); v++){
		int neighbors=0;
		for (int i=1; i<9; i++)
			if (v&(1LLU<<i))
				neighbors++;
		if (v&1LLU) {
			//zivy
			if ( neighbors==2 || neighbors==3)
				step_cell[v] = 1LLU;
			else
				step_cell[v] = 0LLU;
		} else {	
			//mrtvy
			if ( neighbors==3)
				step_cell[v] = 1LLU;
			else
				step_cell[v] = 0LLU;
		}		
	}	
/*
	for (LI v = 0; v < (1<<9); v++){
		binary_luint(v);
		fprintf(stderr,"%llu\n",step_cell[v]);
	}
*/
}

static inline LI step_row(LI ulc, LI up, LI urc, LI l, LI row, LI r, LI dlc, LI down, LI drc){
	LI new = 0LLU;
	new |= step_cell[(row&3ULL) | (l<<2) | (ulc<<3) | ((up&3ULL)<<4) | (dlc<<6) | ((down&3ULL)<<7)]<<0;
	for (int i = 1; i+1 < 64; i++){
		new |= step_cell[ 
			((row>>i)&1ULL) |  (((row>>(i-1))&1ULL)<<1) | (((row>>(i+1))&1ULL)<<2) | ((up>>(i-1)&7ULL)<<3) | ((down>>(i-1)&7ULL)<<6) ]<<i;
	}
	new |= step_cell[((row>>63)&1ULL) | (((row>>62)&1ULL)<<1) | (r<<2) | (urc<<3) | (drc<<4) | (((up>>62)&3ULL)<<5) | (((down>>62)&3)<<7ULL)]<<63;
	return new;
}

void step_grid(uint exist, LI grid[64], int x, int y){
	//pro kazdy bod rovnou pocita okoli a vyhodnoti

	//nebudu pocitat nulove
	if (exist == 0)
		return; 

	LI *new = &grid_heap[ 64*create(x,y) ];
	int create_left = 0, create_right = 0;

	new[0] = step_row(
			!!(get_grid_row(x-1,y-1,63) & (1ULL<<63)),
			get_grid_row(x-1,y,63),
			!!(get_grid_row(x-1,y+1,63) & (1ULL)),
			!!(get_grid_row(x,y-1,0) & (1ULL<<63)),
			grid[0],
			!!(get_grid_row(x,y+1,0) & (1ULL)),
			!!(get_grid_row(x,y-1,1) & (1ULL<<63)),
			grid[1],
			!!(get_grid_row(x,y+1,1) & (1ULL<<0))
			);
	for (int i = 1; i+1 < 64; i++){
		new[i] = step_row(
				!!(get_grid_row(x,y-1,i-1) & (1ULL<<63)),
				grid[i-1],
				!!(get_grid_row(x,y+1,i-1) & (1ULL)),
				!!(get_grid_row(x,y-1,i) & (1ULL<<63)),
				grid[i],
				!!(get_grid_row(x,y+1,i) & (1ULL)),
				!!(get_grid_row(x,y-1,i+1) & (1ULL<<63)),
				grid[i+1],
				!!(get_grid_row(x,y+1,i+1) & (1ULL<<0))
				);
		if ( new[i] & (1ULL) )
			create_left++;
		if ( new[i] & (1ULL<<63) )
			create_right++;
	}
	new[63] = step_row(
			!!(get_grid_row(x,y-1,62) & (1ULL<<63)),
			grid[62],
			!!(get_grid_row(x,y+1,62) & (1ULL)),
			!!(get_grid_row(x,y-1,63) & (1ULL<<63)),
			grid[63],
			!!(get_grid_row(x,y+1,63) & (1ULL)),
			!!(get_grid_row(x+1,y-1,0) & (1ULL<<63)),
			get_grid_row(x+1,y,0),
			!!(get_grid_row(x+1,y+1,0) & (1ULL<<0))
			);
#ifdef DELETE_EMPTY_GRID 
	int nulova = 0;
	for (int i = 0; i < 64; i++)
		if (new[i]!=0ULL){
			nulova++;
			break;
		}
	if (nulova==0){
		delete(x,y);
	}
#endif 

	//zalozeni novych mrizek v okoli je-li potreba	
	if (new[0])
		create(x-1,y);
	if (new[63])
		create(x+1,y);
	if (create_left || (new[0]&1ULL) || (new[63]&1ULL))
		create(x,y-1);
	if (create_right || (new[0]&(1ULL<<63)) || (new[63]&(1ULL<<63)))
		create(x,y+1);

#if 0
	if (step_number == 1){
		fprintf(stderr,"%d %d %d\n",step_number, x,y);
		print_grid(new);
		fprintf(stderr,"\n");
	}


	if (step_number == 2 && x == 4 && y==5){
		fprintf(stderr,"%d %d %d\n",step_number, x,y);
		//binary_luint(get_grid_row(x+1,y,0));
		print_pom(pom);
		fprintf(stderr,"\n");
		print_grid(new);
		fprintf(stderr,"\n");
	}

	fprintf(stderr,"%d %d %d\n",step_number, x,y);
	print_grid(grid);
	fprintf(stderr,"\n");
	print_grid(new);
#endif
}

#endif //OLD_STEP_GRID

#ifdef SIMPLE_THREAD
struct i_j{
	int i;
	int j;
};

static void * simple_thread(void *arg){
	struct i_j *ij = (struct i_j *) arg;

	step_grid( 	big_grid(ij->i,ij->j),
			&grid_heap[64*big_grid(ij->i,ij->j)],
			ij->i,ij->j);
	return NULL;
}
#endif

#ifdef COMPLICATED_THREAD
int grid_row, grid_column;
static void * complicate_thread(void *arg){
	while (1) {
		//zamknu
		if (pthread_mutex_lock(&mutex))
			perror("problem_mutex6");
		do {
			if ( grid_column < big_grid_right) {
				//posunu se po radku
				grid_column++;
			} else {
				//jdu na novy radek
				if ( grid_row+1 < big_grid_down) {
					grid_column = big_grid_left;
					grid_row++;
				} else { 
					//uz neni novy radek
					if (pthread_mutex_unlock(&mutex))
						perror("problem_mutex7");
					return NULL;
				}
			}
		} 
		while (big_grid(grid_row,grid_column)==0);

		int i = grid_row;
		int j = grid_column;
	//	if (i < 0){
	//		fprintf(stderr,"%d %d\n",i,j);
	//		fprintf(stderr,"nove %d %d %d %d \n",big_grid_up, big_grid_down, big_grid_left, big_grid_right);
	//	}
		if (pthread_mutex_unlock(&mutex))
			perror("problem_mutex8");
		step_grid( 	big_grid(i,j),
				&grid_heap[64*big_grid(i,j)],
				i,j);

	}
	return NULL;
}
#endif


void step(char *fuj){
	step_number++;
//	fprintf(stderr,"%d\n",step_number);
//	fprintf(stderr,"%d %d %d %d \n",big_grid_up, big_grid_down, big_grid_left, big_grid_right);
	
	//vypocitam vnitrky
#ifdef SIMPLE_THREAD
	int tmp = 0;
	struct i_j tmp_ij[THREAD_NUMBER];
	pthread_t thread_id[THREAD_NUMBER];
	for (int i = big_grid_up; i < big_grid_down; i++) {
		for (int j = big_grid_left; j < big_grid_right; j++) {
			if (big_grid(i,j)) {
				tmp_ij[tmp].i=i;
				tmp_ij[tmp++].j=j;
//			fprintf(stderr,"%d\n",tmp);
			} 
			if (tmp == THREAD_NUMBER) {
				for (int k = 0; k < THREAD_NUMBER; k++)
					create(tmp_ij[k].i,tmp_ij[k].j);
				for (int k = 0; k < THREAD_NUMBER; k++)
					if (pthread_create(&thread_id[k],NULL,&simple_thread,&tmp_ij[k]))
						perror("problem");
				for (int k = 0; k < THREAD_NUMBER; k++)
					if (pthread_join(thread_id[k], NULL))
						perror("problem");
				tmp = 0;
			}
		}
	}
	for (int k = 0; k < tmp; k++)
		if (pthread_create(&thread_id[k],NULL,&simple_thread,&tmp_ij[k]))
			perror("problem");
	for (int k = 0; k < tmp; k++)
		if (pthread_join(thread_id[k], NULL))
			perror("problem");

#else	
#ifdef COMPLICATED_THREAD
	pthread_t thread_id[THREAD_NUMBER];
	grid_row = big_grid_up;
	grid_column = big_grid_left-1; //protoze nejdriv se pricte a pak teprve kresli
	for (int i = 0; i < THREAD_NUMBER; i++)
		if (pthread_create(&thread_id[i],NULL,&complicate_thread,NULL))
			perror("problem");
	for (int i = 0; i < THREAD_NUMBER; i++)
		if (pthread_join(thread_id[i], NULL))
			perror("problem");

#else //bez vlaken
	for (int i = big_grid_up; i < big_grid_down; i++){
		for (int j = big_grid_left; j < big_grid_right; j++)
				step_grid( 	big_grid(i,j),
						&grid_heap[64*big_grid(i,j)],
						i,j);
	}
#endif
#endif

	//prehodim nove do stareho a stare vycistim
	swap_big_grid();



#ifdef WRITE_EVERY_STEP
	print_big_grid_to_file(fuj);
#endif
}

//==================================== nacitani

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
#ifdef OLD_STEP_GRID
#else
	step_cell_init();
#endif

	if (argc < 4){
		help();
	}

	FILE *F = fopen(argv[1],"r");
	if (F == NULL) {
		fprintf(stderr,"neotevru %s\n",argv[1]);
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
		fprintf(stderr,"maly vstupni soubor: pocet_sloupcu(<64) pocet_radku(<64) \\n tabulka\n");
		fprintf(stderr,"velky vstupni soubor: 0 0 \\n co radek to x-ova a y-ova souradnice\n");
		fclose(F);
		exit(1);	
	}

	fclose(F);
	swap_big_grid();

	timestamp_t t0 = get_timer();

	for( int i = 0; i < atoi(argv[2]); i++)
		step(argv[3]);

#if ( defined SIMPLE_THREAD || defined COMPLICATED_THREAD)
	if (pthread_mutex_destroy(&mutex))
		perror("problem");
#endif
	t0 = get_timer() - t0;
	printf("%d\n",(int) lround(1000*(t0/1e6)));

	print_big_grid_to_file(argv[3]);

	return EXIT_SUCCESS;
}
