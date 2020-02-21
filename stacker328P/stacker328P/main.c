/*
 * stacker328P.c
 *
 * Created: 02/19/2020 9:25:18 PM
 *
 * Description: The program is a recreation of the game Stacker.
 *
 * Author : Luigi Moca
 *
 *	Ports
 *	PORTB	- SPI
 *	PORTD	- Inputs (Interrupts INT0 & INT1)
 */ 
#define F_CPU 8000000UL  //8 MHz

#include <avr/io.h>
#include <util/delay.h>
#include <avr/interrupt.h>

#include "SPI328P.h"
#include "SPI328PLedMatrix.h"

// Initializing Interrupt Functions
void initINT(void);

// Declare in-game functions
uint16_t setScore(void);
uint8_t setLives(uint8_t);
uint8_t scan(uint8_t, uint8_t, uint8_t);

void congrats(void);
void gameOver(void);
uint8_t livesRemaining(uint8_t);

uint8_t buttonPress = 1, resetPress = 0;

// Game constructor
struct Game
{
	uint8_t row;			// Declaring beginning row
	uint8_t firstNum;		// Variable holds initial value from 1st row
	uint8_t lives, score;	// Declares # of lives & score
	uint8_t gameStart, checkScore;
};

int main(void)
{
	struct Game stacker;		// Declaring game
	
	initINT();					// Initializing interrupts
	sei();						// Initializing global interrupts
	
	SPIMasterInit();			// Initializing SPI Master
	initLEDMatrix();			// Initializing LED Matrix
	
    while(1)
    {
		//////////////////////RESET VARIABLES///////////////////////////////
		buttonPress = 1; resetPress = 0;		// Reset press & buttonPress
		stacker.row = 0x08; stacker.gameStart = 1;	// Set row to the bottom
		clearDisplay();							// Clear display
		////////////////////////////////////////////////////////////////////
		
		stacker.score = setScore();					// Determine original score
		stacker.lives = setLives(stacker.score);	// Set # of lives
		stacker.row -=1;							// Go to the next row
		
		// Run the game + detects reset press condition
		while(stacker.gameStart && !resetPress)
		{
			buttonPress = 1;						// Resumes the game
			// obtain user score
			stacker.checkScore = scan(stacker.row, stacker.score, stacker.lives);
			stacker.row -=1;						// Go to the next row
			
			// Check row score with original score
			stacker.score &= stacker.checkScore;	// Obtain new score
			
			// Obtain the # of lives after checking current block positions
			stacker.lives = livesRemaining(stacker.score);
			
			// If there are no more lives, game over sequence plays
			if(stacker.lives == 0)
			{
				stacker.gameStart = 0;
				while(!resetPress) gameOver();
			}
			// If player gets to the top
			if(stacker.row == 0 && stacker.lives > 0 && !resetPress)
			{
				stacker.gameStart = 0;
				congrats();
			}
		}
    }
}

/*
*	Function initializes interrupts.
*/
void initINT()
{
	/* Using PORTB PCINT0 & PCINT1 for input interrupts
	DDRB = (0 << PB0) | (0 << PB1);		// PB0 & PB1 as inputs
	PORTB = (1 << PB0) | (1 << PB1);	// Enabling pull-up resistors
	*/
	// using PORTD INT pins
	DDRD = (0 << PD2) | (0 << PD3);		// INT0 & INT1 as inputs
	PORTD = (1 << PD2) | (1 << PD3);	// Enable pull-up resistors
	
	EIMSK = 0x03;						// Enabling INT0 & INT1
	EICRA = 0x0F;						// Detecting rising edge
	/*
	PCICR = 0x01;							// Enabling interrupts [7:0]
	PCMSK0 = (1 << PCINT1) | (1 << PCINT0);	// Selecting PCINT0 & PCINT1
	*/
}

/*
*	Stop Button Interrupt
*/
ISR(INT0_vect)
{
	buttonPress = 0;  // Change game state
	// Pauses program until user lets go of button
	while(PIND & 0x04){};
}

/*
*	Reset Button Interrupt
*/
ISR(INT1_vect)
{
	resetPress = 1; // Resets the game
	while(PIND & 0x08){};
}

/*
*	Function returns the value the
*	user pressed from the bottom row.
*
*	Returns the predefined score set
*	from the 1st row.
*/
uint16_t setScore()
{
	// Variable used to return
	// the original score 
	uint16_t shift = 1;
	
	while(buttonPress && !resetPress)
	{
		// For loops make LEDs shift back & forth
		for(uint8_t i = 0; i < 9; ++i)
		{
			// Since this is the first button press to determine
			// user score, 3 LEDs will be shifted.
			if(shift == 2 || shift == 6) shift |= 0x01;
			
			if(buttonPress && !resetPress) writeMax(0x08,shift);
			if(buttonPress && !resetPress) _delay_ms(100);
			if(buttonPress && !resetPress) shift = shift<<1;  //shift value to the left by 1
			if(buttonPress && !resetPress) writeMax(0x08,shift);
		}
	
		for(uint8_t i = 0; i < 9; ++i)
		{
			if(buttonPress && !resetPress) writeMax(0x08,shift);
			if(buttonPress && !resetPress) _delay_ms(100);
			if(buttonPress && !resetPress) shift = shift>>1;  //shift value to the right by 1
			if(buttonPress && !resetPress) writeMax(0x08,shift);
		}
	}
	
	return shift;
}

/*
*	Function sets the # of lives
*	depending on the score.
*	Passing Parameter:
*	uint8_t score	- Score
*/
uint8_t setLives(uint8_t score)
{
	uint8_t lives = 0;
	
	// Returns the # of lives
	// depending on the score
	switch(score)
	{
		case 0x01:
			lives = 1;
		break;
		
		case 0x02:
			lives = 2;
		break;
		
		case 0x80:
			lives = 1;
		break;
		
		case 0xC0:
			lives = 2;
		break;
		
		default:
			lives = 3;
		break;
	}
	return lives;
}

/*
*	Function returns player's
*	remaining lives.
*	Passing Parameters:
*	uint8_t score - used to check for high bits
*/
uint8_t livesRemaining(uint8_t score)
{
	uint8_t lives = 0;
	
	// Check all 8 bits to detect high bits
	for(uint8_t i = 0x80; i > 0; i = i >> 1)
	{
		// if bits are the same, increment lives
		if(score & i) lives +=1;
	}
	
	return lives;
}


/*
*	Function scans through each row.
*/
uint8_t scan(uint8_t row, uint8_t score, uint8_t lives)
{
	int8_t addLives = lives - 1;
	uint16_t shift = 1;
	
	while(buttonPress && !resetPress)
	{
		addLives = lives - 1;
		for(uint8_t i = 0; i < 9; ++i)
		{
			// Accommodate LED shifts according
			// to the # of lives
			if(addLives >= 0) 
			{
				shift |= 0x01;
				addLives -=1;
			}
			
			if(buttonPress && !resetPress) writeMax(row, shift);
			if(buttonPress && !resetPress) _delay_ms(100);
			if(buttonPress && !resetPress) shift = shift<<1;  //shift value to the left by 1
			if(buttonPress && !resetPress) writeMax(row, shift);
		}
		for(uint8_t i = 0; i < 9; ++i)
		{
			if(buttonPress && !resetPress) writeMax(row, shift);
			if(buttonPress && !resetPress) _delay_ms(100);
			if(buttonPress && !resetPress) shift = shift>>1;  //shift value to the right by 1
			if(buttonPress && !resetPress) writeMax(row, shift);
		}
	}
	return shift;
}

/*
*	Function executes when player wins the game.
*/
void congrats()
{
	uint8_t y = 0, l = 0;
	
	uint8_t image_1[8] = {0x00, 0x00, 0x00, 0x18, 0x18, 0x00, 0x00, 0x00};
	uint8_t image_2[8] = {0x00, 0x00, 0x3C, 0x24, 0x24, 0x3C, 0x00, 0x00};
	uint8_t image_3[8] = {0x00, 0x7E, 0x42, 0x5A, 0x5A, 0x42, 0x7E, 0x00};
	uint8_t image_4[8] = {0xFF, 0x81, 0xBD, 0xA5, 0xA5, 0xBD, 0x81, 0xFF};
	
	for(y = 8; y > 0; --y)
	{
		writeMax(y, image_1[y-1]);
	}
	_delay_ms(600);
	for(y = 8; y > 0; --y)
	{
		writeMax(y, image_2[y-1]);
	}
	_delay_ms(600);
	for(y = 8; y > 0; --y)
	{
		writeMax(y, image_3[y-1]);
	}
	_delay_ms(600);
	for(y = 8; y > 0; --y)
	{
		writeMax(y, image_4[y-1]);
	}
	_delay_ms(600);
	
	while(l < 3)
	{
		for(y = 8; y > 0; --y)
		{
			writeMax(y, image_3[y-1]);
		}
		_delay_ms(600);
		for(y = 8; y > 0; --y)
		{
			writeMax(y, image_4[y-1]);
		}
		_delay_ms(600);
		l++;
	}
	clearDisplay();  //Clears display
}


void gameOver(void)
{
	resetPress = 0;
	// Array displays an X on display
	uint8_t XImage[8] = {0x81, 0x42, 0x24, 0x18, 0x18, 0x24, 0x42, 0x81};
	
	for(uint8_t k = 0; k < 5; k++)
	{
		if(!resetPress)
		{
			for(uint8_t z = 8; z > 0; --z)
			{
				writeMax(z, XImage[z-1]);
			}
			_delay_ms(500);
			clearDisplay();
			_delay_ms(500);
		}
	}
}