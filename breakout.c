/*
   Kevin Witmer 2017,

   See README file for compilation instructions

   Ideas taken from:
   https://www.viget.com/articles/game-programming-in-c-with-the-ncurses-library	
*/

#include <ncurses.h>	// For drawing shapes on the screen
#include <unistd.h>		// For usleep

#define DELAY 30000	// Passed to usleep() within main loop to control game speed
#define BALL_DELAY 4 // So ball doesn't move too fast compared to paddle (incremented once per loop iteration)
#define MAX_X 67	// Board width
#define MAX_Y 24	// Board height
#define PAD_Y 23	// Paddle distance from top 
#define BLOCK_W 17	// Block width
#define BLOCK_H 4	// Block height
#define BLOCK_SP 1	// Block spacing
#define BLOCK_ROWS 2 // Num of rows of blocks
#define BLOCK_COLS 4 // Num of columns of blocks

// Block sprites that keep track of where they are on an X/Y coordinate system
struct BlockMap {
	char sprite;		// The character that gets put in the sprite grid before getting printed to the screen
	char *grid_member;	// Pointer to a member of the blocks 2-D array. Keeps track of which block this sprite belongs to
};

// Blocks 2-dimensional array. 0 = not broken, 1 = broken
// These get expanded to the full blocks and placed in a grid of sprites (chars)
// according to the specified width & height of a block
char blocks[BLOCK_ROWS][BLOCK_COLS] = { 0 };

// Grid for keeping track of where to draw block sprites (ascii chars) and create boundaries
struct BlockMap block_sprites[BLOCK_ROWS * BLOCK_H][BLOCK_COLS * BLOCK_W];

// Draw the walls, ceiling
void draw_board (void) {
	int i;	// Iterator

	// Ceiling
	mvprintw(0, 0, ".");			// Left corner
	for (i = 1; i < MAX_X; ++i) {
		mvprintw(0, i, "-");		// Top edge
	}
	mvprintw(0, MAX_X, ".");		// Right corner

	// Left wall
	for (i = 1; i <= MAX_Y; ++i) {
		mvprintw(i, 0, "|");
	}
	// Right wall
	for (i = 1; i <= MAX_Y; ++i) {
		mvprintw(i, MAX_X, "|");
	}
}

// Takes y and x coordinate of top left corner of block and adds it to a sprite grid char by char
void make_block (int y, int x) {
	for (int i = 0; i < BLOCK_H; ++i) {
		for (int j = 0; j < BLOCK_W; ++j) {
			// Top row of block
			if (i == 0) {
				if (j == 0 || j == BLOCK_W - 1) {
					block_sprites[y + i][x + j].sprite = '.'; // Top corner
				}
				else {
					block_sprites[y + i][x + j].sprite = '-'; // Top edge
				}
			}
			// Bottom row of block
			else if (i == BLOCK_H - 1) {
				if (j == 0 || j == BLOCK_W - 1) {
					block_sprites[y + i][x + j].sprite = '\''; // Bottom corner
				}
				else {
					block_sprites[y + i][x + j].sprite = '-'; // Bottom edge
				}
			}
			// Left or right edge
			else if (j == 0 || j == BLOCK_W - 1) {
				block_sprites[y + i][x + j].sprite = '|';
			}
			// Middle of block
			else {
				block_sprites[y + i][x + j].sprite = ' ';
			}
			// Point back to the row and column of the blocks array that these sprites correspond to
			// We're dividing the y & x coord's we got by block height & width, respectively, to resolve
			// to the corresponding members of the blocks array
			block_sprites[y + i][x + j].grid_member = &blocks[y/BLOCK_H][x/BLOCK_W];
		}
	}
}

// Takes y and x coordinates of upper left corner of a block 
// and flips all of the block's sprites (chars) to nulls so it no longer displays
void break_block (int y, int x) {
	for (int i = 0; i < BLOCK_H; ++i) {
		for (int j = 0; j < BLOCK_W; ++j) {
			block_sprites[y + i][x + j].sprite = '\0';
		}
	}
}

// Initialize blocks, based on which are broken and which aren't,
// but don't draw them yet.
void init_blocks (void) {
	// Loop through all blocks
	for (int i = 0; i < BLOCK_ROWS; ++i) {
		for (int j = 0; j < BLOCK_COLS; ++j) {
			// Block is still there
			if ((int) blocks[i][j] == 0) { 
				make_block(i * BLOCK_H, j * BLOCK_W); // Add it to the grid buffer
			}
			// Block is broken
			else {
				break_block(i * BLOCK_H, j * BLOCK_W); // Flip the grid members to null chars
			}
		}
	}
}

// Loops through the block sprite grid (buffer) and prints it out char by char
void print_blocks (void) {
	char sprite_buff[2] = { '\0' }; // Initialize temp buffer to nulls

	// Loop through sprite grid
	for (int i = 0; i < BLOCK_ROWS * BLOCK_H; ++i) {
		for (int j = 0; j < BLOCK_COLS * BLOCK_W; ++j) {
			sprite_buff[0] = block_sprites[i][j].sprite; // Create one-character string (char followed by null terminator)
			mvprintw(i, j, sprite_buff); // Draw sprite using above-created string
		}
	}
}

// Checks to see if ball collided with an x boundary, changes diretion,
// possibly breaking a brick.
// Returns new X direction for ball
int get_direction_x (int x_direction, int next_y, int next_x) {
	// X Collisions
	if (next_x >= MAX_X || next_x <= 0) {
		x_direction *= -1; // Change L/R direction
	}
	// Side of block
	else if (next_y < BLOCK_ROWS * BLOCK_H && next_x < BLOCK_COLS * BLOCK_W && *(block_sprites[next_y][next_x].grid_member) == 0 && block_sprites[next_y][next_x].sprite == '|') {
		x_direction *= -1; // Change L/R direction
		*(block_sprites[next_y][next_x].grid_member) = (char) 1; // Break block
	}

	return x_direction;
}

// Checks to see if ball collided with a y boundary, changes diretion,
// possibly breaking a brick, or returns zero if game is over (ball fell through floor).
// Returns new Y direction for ball
int get_direction_y (int y_direction, int next_y, int next_x, int pad_x) {

	if (next_y <= 0 || (next_y == PAD_Y && next_x >= pad_x && next_x <= pad_x + 3) ) {
		y_direction *= -1; // Change Up/Down direction
	}
	// Top/Bottom of block
	else if (next_y < BLOCK_ROWS * BLOCK_H && next_x < BLOCK_COLS * BLOCK_W && *(block_sprites[next_y][next_x].grid_member) == 0 && block_sprites[next_y][next_x].sprite == '-') {
		y_direction *= -1; // Change Up/Down direction
		*(block_sprites[next_y][next_x].grid_member) = (char) 1; // Break block
	}
	else if (next_y > MAX_Y) {
		y_direction = 0;
	}

	return y_direction;
}


// Checks to see whether all the blocks are broken (1 values in blocks array)
// Returns 0 if there are still blocks remaining, 1 if not.
char game_won (void) {
	char zero_seen = 0; // Boolean. Have we encountered a zero in the blocks array?

	// Loop through blocks one by one
	for (int i = 0; i < BLOCK_ROWS; ++i) {
		for (int j = 0; j < BLOCK_COLS; ++j) {
			if (blocks[i][j] == 0) {
				zero_seen = 1;
			}
		}
	}

	// If we saw a zero, return zero (game not won), otherwise return 1 (game one)
	return zero_seen ? 0 : 1;
}

// Listens for L/R arrow keys,
// Returns 1 to move paddle right, -1 to move left, 0 for no movement
int get_keypress (int pad_x) {
	// Keypress
	int key = getch();

	switch (key) {
		case KEY_LEFT:
			if (pad_x > 1)	// If we haven't hit the left wall yet...
				return -1;	// Move paddle left
			break;
		case KEY_RIGHT:
			if (pad_x < MAX_X - 3)	// If we haven't hit the right wall yet...
				return 1;			// Move paddle right
			break;
		default:
			break;
	}

	// Default to no direction change
	return 0;
}

// Print a fomatted multiline message to user on win/lose
void print_message (char message[]) {
	printf("\n\t#################\n");
	printf("\t# %s #\n", message);
	printf("\t#################\n\n");
} 

/*
   **********************
   ******** MAIN ********
   **********************
*/
int main (void) {
	int ball_x = 0,
		ball_y = BLOCK_H * BLOCK_ROWS,	// Ball x/y coords
		pad_x = 20,						// Paddle x starting position
		next_x = 0,						// Next x/y coords of ball
		next_y = 0,						
		x_direction = 1,				// Ball x/y direction
		y_direction = 1,				
		next_x_dir,						// Next x/y direction (for checking collisions before moving ball)
		next_y_dir,						
		delay_count = 1;				// Delay timer		

	/* *** NCURSES SETUP *** */
	// Initialize screen
	initscr();

	// Setup keyboard
	keypad(stdscr, TRUE);
	// Don't echo inputs to screen
	noecho();
	// Don't display cursor
	curs_set(FALSE);
	
	// Don't block on getch()
	nodelay(stdscr, TRUE);
	cbreak();

	/* *** MAIN LOOP *** */
	while (1) {
		clear();
		
		// Walls, ceiling
		draw_board();
		
		// Blocks
		init_blocks();
		print_blocks();

		// Print Ball
		mvprintw(ball_y, ball_x, "o");
		//Print Paddle
		mvprintw(PAD_Y, pad_x, "---");

		// Refresh window
		refresh();

		// A little delay
		usleep(DELAY);

		// Set up next x/y coordinates for ball
		next_x = ball_x + x_direction;
		next_y = ball_y + y_direction;

		// Only execute this block if we've waited enough cycles (ball delay)
		if (delay_count >= BALL_DELAY) {
			// Get new X direction for ball
			next_x_dir = get_direction_x(x_direction, next_y, next_x);

			// If no direction change
			if (next_x_dir == x_direction) { 
				ball_x += x_direction;		// Advance in the same X direction
			}
			x_direction = next_x_dir;		// Store new direction either way

			// Get new Y direction for ball
			next_y_dir = get_direction_y(y_direction, next_y, next_x, pad_x);

			// Exceeded maximum Y coordinate, so get_direction_y returned a 0
			if (next_y_dir == 0) {
				// Ball fell through, end game
				endwin();
				print_message("You lose! >:(");
				return 0;
			}
			// Advance in same direction if no change
			else if (next_y_dir == y_direction) {
				ball_y += y_direction;	// Advance in same Y direction
			}
			y_direction = next_y_dir;	// Store new direction either way

			delay_count = 0; // Reset delay timer
		}

		// All blocks gone?
		if (game_won()) {
			endwin();
			print_message("You win! >:) ");
			return 0;
		}

		// Change paddle direction on L/R arrow key
		pad_x += get_keypress(pad_x);

		++delay_count; // Increment delay timer
	}

	// If we somehow got here, exit out
	endwin();
	return 0;
}
