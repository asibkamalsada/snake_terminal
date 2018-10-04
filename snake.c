#include <Windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <stdbool.h>
#include <conio.h>

#define BORDER 4
#define FOOD 3
#define BODY 2
#define HEAD 1
#define EMPTY 0

#define INITIAL_LENGTH 4
#define MOVE_DURATION 500

int WIDTH = 10;

// saves whether the snake has eaten something in the last move, so to say, is there food in its stomach
typedef enum { food, no_food } Foodstatus;
// more convenient than coding the moves in plain characters. easier to read
typedef enum { MOVE_UP = 'w', MOVE_RIGHT = 'd', MOVE_DOWN = 's', MOVE_LEFT = 'a', MOVE_DEFAULT} Move;
// self_explanatory, but is used in various functions
typedef enum { GAME_WON, SUCCESS, FAILURE } Outcome;

// what makes a field... SQUARES! many of them
typedef struct{
	// tail has position 1 and head of the snake has position (length of snake), all other fields have 0
	int position;
	// what kind of field is it currently, body, head, border or empty
	int status;
}Square;

//dont mind these, these are only for the program to work correctly
Outcome spawn_food( Square*, int );
Outcome move(Square*, Move, Move*, Foodstatus*);
void print_field(Square*);
void init_field(Square*);
void end_of_game(void);

int main(void)
{
	//game loop
	while(true){
		printf("\nWanna start? how many rows should there be?:\n");
		char input[10];
		if(fgets(input, sizeof input, stdin)){
			// width will be set zero when user types zero, or a non number
			if((WIDTH = (int)strtol(input, NULL, 10)) == 0){
				// break out of the game (loop)
				break;
			}
		}		
		//sets a random seed for the food
		srand(time(NULL));
		
		// on this field, the magic happens
		Square *field = malloc(WIDTH * WIDTH * sizeof *field);

		
		init_field( field );
		

		Move *last_move = malloc(sizeof *last_move);
		*last_move = MOVE_UP;
		Foodstatus *foodstatus = malloc(sizeof *foodstatus);
		*foodstatus = no_food;
		
		// extends or lessens the time span the user can react -> speed
		double factor = 0;

		// moving the snake loop
		while( true ){
			factor = 1;
			// so the user has time to react, slows the program down
			Sleep((int)(MOVE_DURATION * factor));
			char input = MOVE_DEFAULT;
			// only does something when KeyBoardisHIT 
			if(kbhit()){
				// takes first character
				input = getch();
				// deletes rest
				while( kbhit() ) {
					(void)getch();
				}
			}
			if ( input == ' ' ) {
				factor = 0.25;
				input = MOVE_DEFAULT;
			} else if ( 
			// this if branch gives the default move if something different than needed was hit
				!(input == MOVE_DOWN 
					|| input == MOVE_UP 
					|| input == MOVE_LEFT 
					|| input == MOVE_RIGHT 
					|| input == MOVE_DEFAULT) ){
				input = MOVE_DEFAULT;
			}
			// what happened in the last move? outcome tells you
			Outcome outcome = move(field, input, last_move, foodstatus);
			if(outcome == FAILURE || outcome == GAME_WON){
				if(outcome == FAILURE){
					printf("YOU DIED\n");
				} else {
				printf("YOU WON\n");
				}
				// erases all user input from buffer, thats pretty important
				while(kbhit()){
					(void)getch();
				}
				// go out of snake move loop back into game loop
				break;
			}
		}
		// memory management
		free( foodstatus );
		free( last_move );
		free( field );
	}
	return EXIT_SUCCESS;
}

// creates the field, sets the head and spawns food
void init_field(Square *field)
{
	for (int i = 0; i < WIDTH; i++){
		for (int j = 0; j < WIDTH; j++){
			// first there is nothing 
			(field+(i*WIDTH)+j)->position = 0;
			if ( i == 0 || j == 0 || i == WIDTH-1 || j == WIDTH-1){
				(field+(i*WIDTH)+j)->status = BORDER;
			} else {
				(field+(i*WIDTH)+j)->status = EMPTY;
			}
		}
	}
	// but then there comes a place for the snake
	int x = 1 + (rand() % (WIDTH - 2));
	int y = 1 + (rand() % (WIDTH - 2));

	(field+(x*WIDTH)+y)->status = HEAD;
	(field+(x*WIDTH)+y)->position = INITIAL_LENGTH;
	// and some food is added
	spawn_food(field, (WIDTH-2) * (WIDTH-2) - 1);
	// look, user, how awesome
	print_field(field);
	
}


// okay this one is heavy, it basically checks for every Square how it changes after a successful move
Outcome move(
	// self_explanatory
	Square* field, 
	// to be moved
	Move direction, 
	// current move will be written into this, but at first it holds the last move
	Move *last_move, 
	// first holds old status, then new
	Foodstatus *foodstatus)
{
	// has the tail moved?
	bool tail_moved = false;
	// has the head moved?
	bool head_moved = false;
	// yummy?
	bool ate_this_move = false;
	
	if(direction == MOVE_DEFAULT 
		// these are non-legit moves and the snake will go its normal path then
		|| (*last_move == MOVE_DOWN && direction == MOVE_UP)
		|| (*last_move == MOVE_UP && direction == MOVE_DOWN)
		|| (*last_move == MOVE_LEFT && direction == MOVE_RIGHT)
		|| (*last_move == MOVE_RIGHT && direction == MOVE_LEFT))
	{
		direction = *last_move;
	} else {
		*last_move = direction;
	}
	
	// this one's kinda crazy, it translates the move into a calculation
	int delta_x = 0;
	int delta_y = 0;
	switch(direction){
		case MOVE_UP: delta_y = -1; break;
		case MOVE_RIGHT: delta_x = 1; break;
		case MOVE_DOWN: delta_y = 1; break;
		case MOVE_LEFT: delta_x = -1; break;
		default: break;
	}
	
	// will be needed to spawn food
	int number_of_free_squares = 0;
	
	// oof
	for (int i = 0; i < WIDTH; i++){
		for (int j = 0; j < WIDTH; j++){
			// less typing, so I saved those into variables
			int old_coordinate_delta = (i*WIDTH)+j;
			int new_coordinate_delta = ((i+delta_y)*WIDTH)+(j+delta_x);
			
			// what kind of square are we currently on
			switch((field + old_coordinate_delta)->status){
				case HEAD : 
					// this won't let the head move more than once per Move
					if (!head_moved){
						
						// this needs to be done first, so that the new square on which the head appears can be handled correctly
						if( ( field + new_coordinate_delta )->status == BORDER ){
							// this leads the head to pop on the other side
							if ( i + delta_y == 0 ) {
								new_coordinate_delta = ( WIDTH-2 )*WIDTH + (j+delta_x);
							} else if ( i + delta_y == WIDTH-1 ) {
								new_coordinate_delta = WIDTH + (j+delta_x);
							} else if ( j + delta_x == 0 ) {
								new_coordinate_delta = ((i+delta_y)*WIDTH) + WIDTH-2 ;
							} else if ( j + delta_x == WIDTH-1 ) {
								new_coordinate_delta = ((i+delta_y)*WIDTH) + 1;
							}
						}
						// the offset translates the food info into calculateable info
						int offset = 0;
						if(*foodstatus == food){
							offset = 1;
						}
						// what will happen on the new square
						switch( ( field + new_coordinate_delta )->status){
							// if there is food, it will handle food + empty
							case FOOD : 
								ate_this_move = true;
							case EMPTY : 
								( field + new_coordinate_delta )->status = HEAD; 
								break;
								
							case BODY :
								// if there is no food eaten last move and the new square is the tail and the tail didnt move yet
								if ( *foodstatus == no_food 
									&& ( field + new_coordinate_delta )->position == 1 
									&& !tail_moved )
								{
									// the head can go to this square
									( field + new_coordinate_delta )->status = HEAD;
								} else {
									// but if not, its the end of the current game
									return FAILURE;
								}
								break;
							default : break;
						}
						// sets new square position to the old one of the head + 1 if the snake ate, so it grows
						(field + new_coordinate_delta)->position = (field + old_coordinate_delta)->position + offset;
						// sets the old coordinate one less, but only if the snake didnt eat
						(field + old_coordinate_delta)->position -= (1-offset);
					
						// self_explanatory
						(field + old_coordinate_delta )->status = BODY;
						
						head_moved = true;
					}
					break;
				// how to update the body		
				case BODY : 
					if ( *foodstatus == no_food ){
						( ( field + old_coordinate_delta )->position )--;
						// this is the tail
						if( ( field + old_coordinate_delta )->position == 0){
							( field + old_coordinate_delta )->status = EMPTY;
							tail_moved = true;
						} else {
							( field + old_coordinate_delta )->status = BODY;
						}
					}
					break;
				
				case EMPTY :
					// counts free squares
					number_of_free_squares++;
					// normalizes, maybe something went wrong before?
					( field + old_coordinate_delta )-> position = 0;
					break;
				// nothing to do here			
				case BORDER : break;
				case FOOD : break;
				default : break;
			}
		}
	}
	//new foodstatus is food, if the snake ate this move
	*foodstatus = ate_this_move ? food : no_food;

	print_field(field);
	
	if ( ate_this_move ) {
		// cant spawn food? you won
		if ( spawn_food( field, number_of_free_squares ) == FAILURE ){
			return GAME_WON;
		}
		print_field(field);
	}
	
	return SUCCESS;
	
}

Outcome spawn_food( Square* field, int number_of_free_squares ) 
{
	if ( number_of_free_squares <= 1 ){
		// exits a bit earlier than normal snake I guess, but its okay
		return FAILURE;
	} 
	// the number of the EMPTY square to get food
	int nth_free_square = rand() % (number_of_free_squares - 1);
	
	for (int i = 0; i < WIDTH; i++){
		for (int j = 0; j < WIDTH; j++){
			// is current square empty?
			if ( ( field + (i*WIDTH)+j )->status == EMPTY ) {
				// is it the correct square?
				if ( nth_free_square-- == 0 ) {
					// yay
					( field + (i*WIDTH)+j )->status = FOOD;
					return SUCCESS;
				}				
			}
		}
	}
	// hm, dont seem to be able to write it in
	return FAILURE;
}

void print_field(Square *field)
{
	system("CLS");
	//printf("   00 01 02 03 04 05 06 07 08 09 10 11 12 13 14 15 16 17 18 19\n");
	printf("\n\n\n");
	for (int i = 0; i < WIDTH; i++){
		//printf("%02d ",i);
		for (int j = 0; j < WIDTH; j++){
			if ( j == 0 ){
				printf("   ");
			}
			switch((field+(i*WIDTH)+j)->status){
				case HEAD : printf("H "); break;
				case BORDER : printf("x "); break;
				case FOOD : printf("T "); break;
				case BODY : 
					if ((field+(i*WIDTH)+j)->position == 1){
						printf("o ");
					} else {
						printf("* "); 
					}
					break;
				default : printf("  "); break;
			} 
		}
		switch(i){
			case 0: 
				printf("   Control with W A S D\n");
				break;
			case 1:
				printf("   Hold SPACE for faster travel\n");
				break;
			case 2:
				printf("      ENJOY!\n");
				break;
			default:
				printf("\n");
		}
	}
	
}



