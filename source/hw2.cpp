#include <curses.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <time.h>
#include <unistd.h>

#define ROW 10
#define COLUMN 50
#define logInput 9
#define maxLength 16
#define minLength 9
#define DELAY 50000

struct Node {
  int x, y;
  Node(int _x, int _y) : x(_x), y(_y){};
  Node(){};
} frog;

char map[ROW + 10][COLUMN];

// Custom Variable
pthread_mutex_t mutex;
bool onLogs = 0;
bool directionLog[logInput] = {0};
int sizeLog[logInput];
int positionLog[logInput];
int status = 0;

void startLogs();

void startGame() {
  srand(static_cast<unsigned int>(time(NULL))); 
  pthread_mutex_init(&mutex, NULL);
  frog = Node(ROW, (COLUMN - 1) / 2);
  startLogs();
  status = 0;
}

void startLogs() {
  size_t i = 0;
  while ( i < logInput) {
    if (i % 2) directionLog[i] = true;
    positionLog[i] = rand() % (COLUMN - 1);
    sizeLog[i] =
        minLength +
        (rand() % ((maxLength + 1) - minLength));
    i++;
  }
}

void updateMap() {
  printf("\033[H\033[2J");
  size_t i, j;

  for (i = 1; i < ROW; ++i) {
    for (j = 0; j < COLUMN - 1; ++j) {
      if (j != positionLog[i - 1]) {
        map[i][j] = ' ';
        continue;
      }
      for (size_t k = 0; k < sizeLog[i - 1]; k++)
        map[i][(j + k) % (COLUMN - 1)] = '=';

      j = j + sizeLog[i - 1];
    }
  }

  for (j = 0; j < COLUMN - 1; ++j) 
    map[ROW][j] = map[0][j] = '|';
  
  for (j = 0; j < COLUMN - 1; ++j) 
    map[0][j] = map[0][j] = '|';
  
  map[frog.x][frog.y] = '0';
  for (i = 0; i <= ROW; ++i) puts(map[i]);
}

// Determine a keyboard is hit or not. If yes, return 1. If not, return 0.
int kbhit(void) {
  struct termios oldt, newt;
  int ch;
  int oldf;

  tcgetattr(STDIN_FILENO, &oldt);

  newt = oldt;
  newt.c_lflag &= ~(ICANON | ECHO);

  tcsetattr(STDIN_FILENO, TCSANOW, &newt);
  oldf = fcntl(STDIN_FILENO, F_GETFL, 0);

  fcntl(STDIN_FILENO, F_SETFL, oldf | O_NONBLOCK);

  ch = getchar();

  tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
  fcntl(STDIN_FILENO, F_SETFL, oldf);

  if (ch != EOF) {
    ungetc(ch, stdin);
    return 1;
  }
  return 0;
}

void *frog_move(void *p) {
	while(status == 0){
		if(kbhit()){
			char direction = getchar();
			if(direction == 'W' || direction == 'w') frog.x -= 1;
			if(direction == 'A' || direction == 'a') frog.y -= 1;
			if(direction == 'S' || direction == 's') {
        frog.x += 1;
        if(frog.x == 11) frog.x -= 1;
      }
			if(direction == 'D' || direction == 'd') frog.y += 1;
			if(direction == 'Q' || direction == 'q') {
				status = 3;
			}
			if(map[frog.x][frog.y] == ' '){
				status = 2;
			}
		}

		if(status !=0 ) break;
    if (map[frog.x][frog.y] == ' '){
      status = 2;
    } else {
      status = 0;
    }

    if (map[frog.x][frog.y] == '=') onLogs = 1;
    if (map[frog.x][frog.y] == '|') onLogs = 0;
    map[frog.x][frog.y] = '0';
	}
	pthread_exit(NULL);
}

void *logs_move(void *t) {
  /*  Move the logs  */
  while (status == 0) {
    pthread_mutex_lock(&mutex);
    for (size_t i = 0; i < logInput; i++) {
      if (directionLog[i]) {
        positionLog[i] = (positionLog[i] + 1) % (COLUMN - 1);
      } else {
        if (positionLog[i] - 1 < 0) {
          positionLog[i] = ((positionLog[i] - 1) + (COLUMN - 1));
        } else {
          positionLog[i] = ((positionLog[i] - 1) % (COLUMN - 1));
        }                 
      }  
    }
    /*  Check game's status  */
    if (onLogs) {
      if (directionLog[frog.x - 1])
        frog.y += 1;
      else
        frog.y -= 1;
    }

    if (frog.x == 0) status = 1;
    if (frog.y == -1 || frog.y == 49) status = 2;
    /*  Print the map on the screen  */
    usleep(DELAY);
    updateMap();
    pthread_mutex_unlock(&mutex);
  }
  pthread_exit(NULL);
}

int main(int argc, char *argv[]) {

  startGame();
  memset( map , 0, sizeof( map ) ) ;
	int i , j ; 
	for( i = 1; i < ROW; ++i ){	
		for( j = 0; j < COLUMN - 1; ++j )	
			map[i][j] = ' ' ;  
	}	

	for( j = 0; j < COLUMN - 1; ++j )	
		map[ROW][j] = map[0][j] = '|' ;

	for( j = 0; j < COLUMN - 1; ++j )	
		map[0][j] = map[0][j] = '|' ;

	frog = Node( ROW, (COLUMN-1) / 2 ) ; 
	map[frog.x][frog.y] = '0' ; 

	//Print the map into screen
	for( i = 0; i <= ROW; ++i)	
		puts( map[i] );

  /*  Create pthreads for wood move and frog control.  */
  pthread_t movement, log;

  pthread_create(&log, NULL, &logs_move, NULL);
  pthread_create(&movement, NULL, &frog_move, NULL);

  pthread_join(log, NULL);
  pthread_join(movement, NULL);

  if(status != 0){
		if(status == 1){
			printf("\033[H\033[2J");
			printf("You won the game!!\n");
		}
		if(status == 2){
			printf("\033[H\033[2J");
			printf("You lose the game!!\n");
		}
		if(status == 3){
			printf("\033[H\033[2J");
			printf("You exit the game.\n");
		}
	}
  pthread_exit(NULL);
  return 0;
}
