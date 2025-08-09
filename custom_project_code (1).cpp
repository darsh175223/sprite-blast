/* Your Name & E-mail: Darshan Mundewadi - dmund006@ucr.edu
* Discussion Section: 021
* Assignment: Custom lab Demo 2
* Exercise Description: [optional - include for your own benefit]
*
* I acknowledge all content contained herein, excluding
* template or example code, is my own original work.
*
* Demo Link:  https://youtu.be/EKeDtffRI4Q
*/



#define F_CPU 16000000UL
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
#include <stdlib.h>
#include "timerISR_lab7_sp2025.h"
#include "periph_lab7_sp2025.h"
#include "serialATmega-4 (1).h"
#include "LCD.h"
#include <vector.h>
#include "spiAVR.h"
#include <avr/pgmspace.h>
#include "queue.h"
//0 = off, 255 = change, 1 - 5 = tutoroal, lvl 1-3, boss fight, 6 = HP dec, 7 = score change, 253 = pre-game, 254= post-game
//LCD display queue
Queue q;
bool start = false;

int sonar_read(void);
int projectTypeChooser();
int checkSerialMessage();
void processMonsters();
void processProjectile();
void spawnProjectile(char type);
void checkCollisions();
void removeHitCharacters();
void spawnMonster(int x_spawn, int y_spawn, char type);
void spawnRandomMonster() ;
//declare boss functions
int predictMove1NN(int qdx, int qdy, int qdistX, int qdistY, int qproj);
int calcdistanceX();
int calcdistanceY();
int projectileCheck();
void checkPlayersHit();
void checkBossHit();
void transmitMonsterType();
void setBossPosition();


//data set for boss AI
//saving data to flash NOT RAM because it os overloading the RAM
static const int dx[] PROGMEM  = {
  0,  -2,   1,   0,   3,   0,  -1,   2,   0,  -3,  // Varied horizontal movement
  1,   0,  -2,   0,   2,  -1,   0,  -2,   1,   0,  // Mixed directions
  0,   3,  -1,   1,  -2,   0,   0,   2,  -3,   0,  // Evasion patterns
  0,   2,  -2,   1,  -1,   0,   1,  -2,   2,   0,  // Balanced small movements
  -1,   0,  -1,   2,  -2,   1,  -1,   0,   2,  -2,  // Normal gameplay
  3,   0,   0,   3,   3,   3,   2,   3,   0,   2,  // High-distance scenarios
  3,   0,  -3,   0,   3,  -3,   1,   0,  -3,   3,  // Extreme evasion
  0,   3,   0,  -3,   3,   0,   0,   3,   2,  -2,  // Vertical emphasis
  3,   0,   0,  -3,   3,   0,   0,   3,   3,   0,  // Horizontal focus
  0,   3,   3,   0,   0,   3,   3,   0,   0,   3   // Final patterns
};

static const int dy[] PROGMEM  = {
  0,   0,   2,   0,  -1,   1,  -1,   0,  -2,   1,  // Vertical variation
  -1,   0,   0,   1,   0,   1,  -1,  -1,   1,   0,  // Mixed patterns
  0,   2,  -2,   1,   1,  -3,   3,   0,   0,  -1,  // Evasive vertical
  1,  -1,   0,   2,   0,  -1,   1,   0,  -2,   3,  // Balanced movement
  0,  -2,   2,  -1,   1,   0,  -3,   2,  -1,   0,  // Normal gameplay
  2,   0,   3,   0,   1,   2,   2,   3,   3,   1,  // High-distance patterns
  0,   3,   3,  -1,   0,  -1,   1,  -1,   2,   0,  // Vertical dominance
  -3,   0,   3,   0,  -3,   3,   3,   0,  -2,   2,  // Extreme evasion
  0,   3,   3,   0,  -3,   3,   3,   0,  -2,   2,  // Vertical focus
  0,   3,   3,   0,   0,   3,   3,   0,   0,   3   // Final patterns
};

// Distance to player (X axis)
static const int distanceX[] PROGMEM  = {
 20,  -25,  15, -30,   0,   8,  12,   3,  25,  -5,  // Balanced distances
-20,   5,   5, -10,  10,   7,  -7, -30,   0,  10,  // Edge cases
 30,  30, -30,  30, -30,   0,  28, -10,  15,  30,  // Extreme distances
 30, -30,   5,  30, -30,   0,   3,  30, -30,   0,  // Long-range focus
 18, -18,  30, -30,  30, -30,  10, -10,  30, -30,  // Balanced extremes
 15, -12,   0,   5,  30, -30,   5,   4,   0,   2,  // Transition cases
 30,   1, -15,   0,  20,  -8,   7,   0,  30,  -3,  // Spawn minion triggers
 12,   0, -12,   0,   8,  -8,   9,   0,  -9,   9,  // Mid-range patterns
  0,  14,   0, -14,  25,   0,   0,  22,  18,   0,  // Shooting scenarios
-20,   0,   0,  20, -28,   0,   0, -30,  30,   0   // Final patterns
};

// Distance to player (Y axis) 
static const int distanceY[] PROGMEM  = {
  0,    0,    5,    0,  -10,   -8,   12,    2,    0,    3,  // Normal range
 -5,    0,    5,    5,    0,   -7,    7,  -10,   15,    0,  // Balanced spread
 20,  -25,   30,  -18,    8,   30,  -30,   27,  -22,   10,  // Extreme cases
 -5,    5,   30,  -30,    0,   25,  -25,   30,  -30,   15,  // Evasion patterns
-15,    2,   -2,    1,   -1,    0,   30,  -30,    3,   -3,  // Mid-range focus
 -5,    0,   20,    8,    2,    5,    5,   -5,   30,   -1,  // Vertical emphasis
  0,   -1,  -15,    1,    1,    0,    2,   -6,  -10,    0,  // Shooting scenarios
  0,   12,    0,  -12,    0,    0,    0,  -18,    0,    0,  // Balanced spacing
 20,    0,   16,    0,    0,  -22,  -24,    0,   10,  -10,  // Edge cases
  0,   25,    0,  -25,   30,  -30,    5,    0,    0,   28   // Final patterns
};

// Projectile scan values 
static const int projectileScan[]  PROGMEM = {
 32,  32,  32,  32,   5,  32,   3,  32,  10,  32,  // No projectiles
 32,  32,   2,  32,   1,  32,   4,   1,  32,  32,  // Occasional detection
  1,   1,   1,  32,  32,  32,   2,   7,  32,   1,  // Evasion scenarios
 32,   1,   3,  32,   1,  32,   2,   1,   4,  32,  // Mid-range detection
  1,   3,   1,  32,   1,   2,  32,   1,   2,  32,  // Close detection
 32,   2,  32,   3,  32,   4,  32,  32,  32,  32,  // Rare spawns
 32,  32,   1,  32,  32,  32,   2,   1,   1,  32,  // Balanced evasion
 32,   0,   2,   1,  32,   3,   2,  32,   1,   2,  // Critical evasion
  3,  32,   2,  32,   4,   1,  32,   3,   1,  32,  // Extreme cases
  2,   1,  32,   2,  32,   1,   0,   1,   2,  32   // Final patterns
};

// Boss moves: 0 - shoot, 1-4 move, 5 - spawn monster
static const int bossMoves[] PROGMEM = {
  1,   3,   0,   2,   5,   1,   0,   3,   0,   4,  // Balanced initial moves
  2,   0,   1,   2,   0,   1,   0,   4,   3,   1,  // Mixed strategies
  5,   1,   3,   1,   4,   0,   0,   2,   4,   0,  // Evasion + spawn
  0,   0,   3,   0,   0,   5,   0,   4,   0,   3,  // Shooting priority
  0,   1,   0,   2,   0,   0,   4,   0,   2,   4,  // Mid-game patterns
  0,   0,   5,   0,   0,   0,   0,   4,   3,   1,  // Long-range focus
  3,   0,   4,   0,   0,   3,   0,   0,   5,   2,  // Evasion + spawn
  4,   0,   0,   0,   3,   0,   0,   4,   0,   0,  // Critical evasion
  0,   1,   0,   4,   0,   0,   4,   0,   0,   3,  // Final defense
  0,   0,   5,   0,   3,   0,   0,   0,   0,   4   // Strategic spawning
};



unsigned int distance;
bool inCentimeters=false;


int abs(int x) {
    return x >= 0 ? x : -x;
}

// --- Base Character Class ---
class Character {
public:
    int x;
    int y;

    Character(int x_ = 0, int y_ = 0)
        : x(x_), y(y_) {}

    virtual ~Character() = default;

    
};

// --- Player Derived Class ---
class Player : public Character {
public:
    int direction; //0 -> up, 1 -> down, 2-> left, 3-> right
    bool shoot;
    char bulletType;


    Player(int x_ = 27, int y_ = 75, int direction_ = 0, bool shoot_ = false, 
           int bulletType_ = 0 )
        : Character(x_, y_), 
          direction(direction_), shoot(shoot_), bulletType(bulletType_) {}
};


// --- Monster Derived Class ---
class Monster : public Character {
public:
    char monsterType;
    bool hit = false;

    Monster(int x_ = 0, int y_ = 0, char monsterType_ = 0)
        : Character(x_, y_ ), monsterType(monsterType_) {     
    }
};

class Projectile : public Character {
public:
    bool player;      // true if shot by player, false if by monster
    char type;        // projectile type
    int direction;    // direction of the projectile
    bool hit;

    // Default constructor
    Projectile() : Character(), player(false), type(0), direction(0), hit(false) {}

    // Parameterized constructor
    Projectile(int x_, int y_, bool player_, char type_, int direction_)
        : Character(x_, y_), player(player_), type(type_), direction(direction_), hit(false) {}
};

int level = 0;
int hearts = 5;
int score = 5;
int gameState = 0; //0 = tutorial, 1 = game running, 2 = boss fight, 3 = you won 
int gameTime = 0;

// Create some characters/global vars/function
Player player(20, 10);

Vector<Monster> monsters;

Vector<Projectile> magazine;

class Boss : public Character {
public:
    int direction =0 ; //0 -> up, 1 -> down, 2-> left, 3-> right
    bool hit= false;
    int numMinionCharge = 2; 
    int move = 0; //debugging purposes
    int ammo = 5;


    Boss(int x_ = 16, int y_ = 16)
        : Character(x_, y_)
           {}
    
    void setBossMove(int m){
        move = m;
        
        switch (m)
        {
        case 0:
            if(ammo>0){
                boss_shoot();
                ammo--;
            }
            break;
        case 1:
            direction = 0;
            break;
        case 2:
            direction = 1;
            break;
        case 3:
            direction = 2;
            break;
        case 4:
            direction = 3;
            break;
        case 5:
            if(numMinionCharge>0){
                spawnMinion();
                numMinionCharge--;
            }
            break;
        
        default:
            break;
        }
    }

    

    void boss_shoot(){
        magazine.push_back( Projectile(x+1, y, false, 0, 1));
    }

    void spawnMinion(){
      // int randType = (rand())%4; 
      spawnMonster(x+1, y, 0 );
    }
};

Boss boss(16,16 );



// --- Game Board Class ---
class GameBoard {
public:
    static const int ROWS = 32;
    static const int COLS = 32;
    char board[ROWS][COLS]; 

    GameBoard() {
        // Initialize all cells to a default value, e.g., '.' for empty
        for (int i = 0; i < ROWS; i++) {
            for (int j = 0; j < COLS; j++) {
                board[i][j] = '.';
            }
        }
    }
    

    void clearBoard() {
        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < COLS; ++col) {
                board[row][col] ='.';
            }
        }
    }

    // Getter for a specific cell
    char getCell(int row, int col) {
        if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
            return board[row][col];
        }
        return '\0'; 
    }

    // Setter for a specific cell
    void setCell(int row, int col, char value) {
        if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
            board[row][col] = value;
        }
    }

    // Getter for the entire board (returns pointer to the array)
    char (*getBoard())[COLS] {
        return board;
    }

    

};

GameBoard gameBoard; 


const unsigned char NUM_TASKS =6; // TODO: Change to the number of tasks being used
// Task struct for concurrent synchSMs implementations
typedef struct _task {
    signed char state;      // Task's current state
    unsigned long period;   // Task period
    unsigned long elapsedTime; // Time elapsed since last task tick
    int (*TickFct)(int);   // Task tick function
} task;

const unsigned long tasksPeriod_Joystick = 100;
const unsigned long tasksPeriod_button = 200;
const unsigned long periodSonar = 1000;
const unsigned long periodLCD = 500;
const unsigned long periodGameloop = 100;
const unsigned long periodDISPLAY = 100;


const unsigned long GCD_PERIOD = 100; 

enum JoystickState {JoystickStart, Rest, Move};
int tickJoystickStart(int state);

enum ShootButtonState {ShootButtonStart, off, on};
int tickShootButtonState(int state);

enum sonarStates { sonarStart, inch, cm };
int TickFct_sonarStates(int state);

enum LCDStates { LCDStart, LCD_off, LCD_displayMessage };
int TickFct_LCDStates(int state);

enum Gameloop_FSM_States { GL_start,  GL_checkStatus, GL_preStart };
int Gameloop(int state);

enum DISPLAY { DISPLAYStart,  DISPLAY_ON };
int DISPLAY_Tick(int state);


task tasks[NUM_TASKS]; // declared task array with  task


void TimerISR() {
    for (unsigned int i = 0; i < NUM_TASKS; i++) {
        if (tasks[i].elapsedTime == tasks[i].period) {
            tasks[i].state = tasks[i].TickFct(tasks[i].state);
            tasks[i].elapsedTime = 0;
        }
        tasks[i].elapsedTime += GCD_PERIOD;
    }
}


// simple absolute‐value function
int absVal(int v) {
    return (v < 0) ? -v : v;
}

// 0 = Rest, 1 = Down, 2 = Up, 3 = Left, 4 = Right
int directionChooser(int x, int y)
{
    int dx = x - 519;
    int dy = y - 511;

    // Rest if within dead-zone on both axes
    if (absVal(dx) < 20 && absVal(dy) < 20) {
        return 0;
    }

    // Determine which axis dominates
    if (absVal(dx) > absVal(dy)) {
        // Horizontal
        return (dx > 0) ? 4 : 3; // Right if dx > 0, Left otherwise
    } else {
        // Vertical
        return (dy > 0) ? 2 : 1; // Down if dy > 0, Up otherwise
    }
    
}

// Display 32x32 game board scaled to 128x128
void displayGameBoard() {
  uint16_t color=COLOR_WHITE;
    setAddressWindow(0, 0, 127, 127); // Full 128x128, no offsets
    setCS(0); setA0(1); // Data mode
    for (uint8_t y = 0; y < 32; y++) {
        for (uint8_t block_y = 0; block_y < 4; block_y++) { // Repeat 4 rows per game board row
            for (uint8_t x = 0; x < 32; x++) {
              if(gameBoard.board[y][x]=='.'){
                color = COLOR_WHITE;
              }else if(gameBoard.board[y][x]=='p'){
                color = COLOR_PURPLE;
              }
              else if(gameBoard.board[y][x]=='b'){
                color = COLOR_YELLOW;
              }else if(gameBoard.board[y][x]==0){
                color = COLOR_RED;
              }else if(gameBoard.board[y][x]==1){
                color = COLOR_BLUE;
              }else if(gameBoard.board[y][x]==2){
                color = COLOR_GREEN;
              }else if(gameBoard.board[y][x]==3){
                color = COLOR_BLACK;
              }else if(gameBoard.board[y][x]=='B'){
                color = COLOR_ORANGE;
              }else if(gameBoard.board[y][x]=='a'){
                color = COLOR_CYAN;
              }



                for (uint8_t block_x = 0; block_x < 4; block_x++) { // Repeat 4 pixels per game board column
                    SPI_SEND(color >> 8); // High byte
                    SPI_SEND(color & 0xFF); // Low byte
                }
            }
        }
    }
    setCS(1);
}

int main(void) {
    sei(); // Enable global interrupts


    DDRB = 0b111111;
    PORTB = 0b000000;


    DDRC = 0b000000;
    PORTC = 0b000000;


    DDRD = 0b11111100;
    PORTD = 0b00000000;


     _delay_ms(50);  // Allow LCD to power up

     ADC_init();
      serial_init(9600);

    lcd_init();
    lcd_clear();
    lcd_setup_custom_chars();

   
    displayInit(); // Initialize display
    fillScreenWhite(); // Fill screen with white
    
    unsigned char i=0;
    tasks[i].state = JoystickStart;
    tasks[i].period = tasksPeriod_Joystick;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickJoystickStart;
    ++i;
    tasks[i].state = ShootButtonStart;
    tasks[i].period = tasksPeriod_button;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &tickShootButtonState;
    ++i;
    tasks[i].state = sonarStart;
    tasks[i].period = periodSonar;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_sonarStates;
    ++i;
    tasks[i].state = LCDStart;
    tasks[i].period = periodLCD;
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &TickFct_LCDStates;
    ++i;
    tasks[i].state = GL_start;
    tasks[i].period = periodGameloop; 
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &Gameloop;
    ++i;
    tasks[i].state = DISPLAYStart;
    tasks[i].period = periodDISPLAY; 
    tasks[i].elapsedTime = tasks[i].period;
    tasks[i].TickFct = &DISPLAY_Tick;




    TimerSet(GCD_PERIOD);
    TimerOn();
 
   
    while(1) {}
    return 0;

  
}


// 0 = Rest, 1 = Up, 2 = Down, 3 = Left, 4 = Right
int tickJoystickStart(int state){
  int direction = directionChooser(ADC_read(1), ADC_read(0));

  switch (state)
  {
  case JoystickStart:
    state = Rest;
    break;
  case Rest:
    if(direction!=0){
      state = Move;
    }else{
      state = Rest;
    }
    
    break;
  case Move:
    if(direction!=0){
      state = Move;
    }else{
      state = Rest;
    }
    
    break;
    
  
  default:
    break;
  }
  //actions

  switch (state)
  {
  
  case Move:

    direction--;
    //set x and y of player in gameboard to .
    gameBoard.board[player.x][player.y]='.';

    
    switch (direction)
{
    case 0: // down
        player.x++;
        if (player.x > 31) {
            player.x = 0; // Loop around to top
        }
        break;
    case 1: // up
        player.x--; 
        if (player.x < 0) {
            player.x = 31; // Loop around to bottom
        }
        break;
    case 2: // left
        player.y--;
        if (player.y < 0) {
            player.y = 31; // Loop around to right
        }
        break;
    case 3: // right
        player.y++; 
        if (player.y > 31) {
            player.y = 0; // Loop around to left
        }
        break;
  }
  gameBoard.board[player.x][player.y]='p';

    
    break;
  
  default:
    break;
  }



  return state;
}

int tickShootButtonState(int state){
  bool A3 = (PINC>>3)&0x01;
  switch (state)
  {
  case ShootButtonStart:
    state = off;
    break;
  case off:
    if(A3 && start) state = on;
    break;
  case on:
    state = off;
    break;
     
  default:
    break;
  }

  switch (state)
  {
  case on:
      //spawm projectile 

    spawnProjectile(projectTypeChooser());




    break;
  
  default:
    break;
  }



  return state;
}
unsigned char bossMove = 0;

int timeLCD = 0;
int TickFct_LCDStates(int state){
  //4, if no message, '0, 1, 2, 3' are possible messages received => 'n, c, f, p' 

  //transition
  switch (state)
  {
  case LCDStart:
    state = LCD_off;
    break;
  case LCD_off:
    if (q.empty()) {
      state = LCD_off;
    } else {
      state = LCD_displayMessage;
    }
    break;
  case LCD_displayMessage:
    if ( q.empty()){
        state = LCD_off;
    } else {
        state = LCD_displayMessage;

    }
    break;
  
  default:
    break;
  }

  //action
  switch (state)
  {
  case LCD_off:

    lcd_clear();
    lcd_goto_xy(0, 0);
    
    break;
  

  case LCD_displayMessage:
    // callSpecialCharacter(0);
    // lcd_write_str(" - lost 1 HP");  
    lcd_clear();
    lcd_goto_xy(0, 0);

    unsigned char LCD_char = q.pop();

    switch (LCD_char)
    {
    case 254:
      lcd_write_str("Press button to");
      lcd_goto_xy(1, 0);
      lcd_write_str("begin");

      break;
    case 253:
      lcd_write_str("You Lost!");
      break;
    case 252:
      lcd_write_str("You Win!");
      break;
    case 1:
      lcd_write_str("Tutorial");
      break;
    case 2:
      lcd_write_str("Level 1");
      break;
    case 3:
      lcd_write_str("Level 2");
      break;
    case 4:
      lcd_write_str("Level 3");
      break;
    case 5:
      lcd_write_str("Boss Fight");
      break;
    case 6:
      lcd_write_character(hearts +48);
      callSpecialCharacter(0);
      lcd_write_str(" left");

      break;
    case 7:
      //debugging
        lcd_write_character(boss.move+48);


      break;

    case 8: 
        lcd_write_str("Press button");
        lcd_goto_xy(1,0);
        lcd_write_str("to eliminate");
        break;
    case 9: 
        lcd_write_str("hold obj near");
        lcd_goto_xy(1,0);
        lcd_write_str("sonar");
        break;
    case 10: 
        lcd_write_str("hold obj far");
        lcd_goto_xy(1,0);
        lcd_write_str("from sonar");
        break;
    case 11: 
        lcd_write_str("cover");
        lcd_goto_xy(1,0);
        lcd_write_str("photoresistor");
   
        break;
        
    
    
    
    default:
      break;
    }

   
    break;
  
  default:
    break;

  }
  return state;
}


int TickFct_sonarStates(int state) {
  // Always read raw distance in centimeters from sonar sensor

  distance = sonar_read();


  // Update state based on desired unit
  if (inCentimeters) {
      state = cm;
  } else {
      state = inch;
      distance = (int)(distance / 2.54);  // Convert cm → inches
  }


  return state;
}

int DISPLAY_Tick(int state){
  switch (state)
  {
  case DISPLAYStart:
    state = DISPLAY_ON;
    break;
  case DISPLAY_ON:
    state = DISPLAY_ON;
    break;
  
  default:
    break;
  }

  switch (state)
  {
  case DISPLAY_ON:
    displayGameBoard();
    break;
  
  default:
    break;
  }

  return state;
}


int numMonsters = 1;

bool tutorial1spawn = false;
bool tutorial2spawn = false;
bool tutorial3spawn = false;
bool tutorial4spawn = false;

//for non-instantanuas changes
int lastMSValue = 0;
int lastPlayerX = 0;
int lastPlayerY = 0;

//for instantanuas changes
int last_inst_MSValue = 0;
int last_inst_PlayerX = 0;
int last_inst_PlayerY = 0;
bool lostMsg = false;
bool wonMsg = false;
bool bossSpawned = false;



int Gameloop(int state){
    switch(state){
        case GL_start:
            state = GL_preStart;
            break;

        case GL_preStart:

            if (start) {
                state = GL_checkStatus;
            } else {
                state = GL_preStart;
            }
            break;

        case GL_checkStatus:
            if (!start) {
                state = GL_preStart;
            } else {
                state = GL_checkStatus;
            }
            break;

        default:

            break;
    }

    switch(state){
        case GL_preStart:
            if (hearts <= 0 || score < 0) {
                if (!lostMsg) {
                    for (int j = 0; j < 5; j++) {
                        q.push(253);
                    }
                    lostMsg = true;
                }
                start = false;
                level = 0;
                gameBoard.clearBoard();
            }

            if (((PINC >> 3) & 0x01) && !start) {
                start = true;
                gameTime = 0;
                level = 0;
            }

            if (!start && q.empty()) {
                q.push(254);
                hearts = 5;
            }
            break;

        case GL_checkStatus:
           

                if (level == 0) {
                    if (q.empty()) {
                        q.push(level + 1);
                    }
                    if (gameTime == 20 && !tutorial1spawn) {
                        spawnMonster(0, 10, 0);
                        tutorial1spawn = true;
                    }
                    else if (gameTime > 50 && monsters.empty() && !tutorial2spawn) {
                        spawnMonster(0, 13, 1);
                        tutorial2spawn = true;
                    }
                    else if (gameTime > 80 && monsters.empty() && !tutorial3spawn) {
                        spawnMonster(0, 16, 2);
                        tutorial3spawn = true;
                    }
                    else if (gameTime > 100 && monsters.empty() && !tutorial4spawn) {
                        spawnMonster(0, 19, 3);
                        tutorial4spawn = true;
                    }
                    if (monsters.empty() && gameTime > 20 && tutorial4spawn) {
                        level++;
                        gameTime = 0;
                        q.push(level + 1);
                    }
                }
                else if (level > 0 && level < 4) {
                    // Levels 1–3 logic
                    if (q.empty()) {
                        q.push(level + 1);
                    }
                    int spawnInterval = 5 * (4 - level);
                    if ((gameTime % spawnInterval == 0) && gameTime > 0 && numMonsters > 0) {
                        spawnRandomMonster();
                        numMonsters--;
                    }
                    if (monsters.empty() && numMonsters == 0) {
                        level++;
                        gameTime = 0;
                        numMonsters = 1;
                        q.push(level + 1);
                    }
                }
                else if (level == 4) {
                    // Boss fight logic
                    if (q.empty()) {
                        q.push(level + 1);
                    }
                    if (gameTime % 1 == 0 && gameTime != 0) {
                        int bossMOVE = predictMove1NN(
                            player.x - lastPlayerX,
                            player.y - lastPlayerY,
                            calcdistanceX(),
                            calcdistanceY(),
                            projectileCheck()
                        );
                        boss.setBossMove(bossMOVE);
                        setBossPosition();
                        lastPlayerX = player.x;
                        lastPlayerY = player.y;
                    }
                    if (gameTime % 50 == 0 && gameTime != 0) {
                        boss.numMinionCharge += 1;
                        boss.ammo += 5;
                    }
                    checkBossHit();
                    checkPlayersHit();
                }
                else if (level == 6) {
                    // Win‐state logic
                    if (!wonMsg) {
                        for (int j = 0; j < 5; j++) {
                            q.push(253);
                        }
                        wonMsg = true;
                    }
                    start = false;
                    level = 0;
                    gameBoard.clearBoard();
                }
            
            break;

        default:

            break;
    }

    if (level < 4) {
        if (gameTime % (6 - level) == 0 && gameTime != 0) {
            processMonsters();
        }
    } else if (level > 3) {
        processMonsters();
    }
    processProjectile();
    checkCollisions();
    removeHitCharacters();
    gameTime++;

    return state;
}


int projectTypeChooser() {
  unsigned int lightLevel = ADC_read(5);

  int returnValue = 1;


    if (distance > 30 && lightLevel > 10) {

        returnValue= 1;
    }
    else if (distance < 5 && lightLevel > 10) {
        returnValue= 2;
    }
    else if (distance >= 5 && distance <= 20 && lightLevel > 10) {
        returnValue= 3;
    }
    else if (lightLevel < 10) {
        returnValue= 4;
    }
    returnValue--;
    return returnValue;
}

int sonar_read(void) {
    uint16_t count = 0;

    DDRC |=  (1 << PC4);
    PORTC &= ~(1 << PC4);
    _delay_us(2);
    PORTC |=  (1 << PC4);
    _delay_us(10);
    PORTC &= ~(1 << PC4);

    DDRB &= ~(1 << PB0);
    PORTB &= ~(1 << PB0);

    while (!(PINB & (1 << PB0)));
    while (PINB & (1 << PB0)) {
        _delay_us(1);
        count++;
    }

    DDRB |=  (1 << PB0);

    return (int)(count / 58);
}



void spawnMonster(int x_spawn, int y_spawn, char type){
  //type = n, c, f, p  = 1, 2, 3, 4
    Monster newMonster(x_spawn, y_spawn, type);
    //TODO: transmit spawn:
    type= type%4;

    // gameBoard.board[x_spawn][y_spawn]=type;
    
     q.push(type+8);



    monsters.push_back(newMonster);
}


// new: spawn a monster at x=0, random y and random type
void spawnRandomMonster() {
    //using ports to create random values
    int randType = (rand())%4; 
    int randY = (rand())%31;
   
    unsigned char types[] = { 'n', 'c', 'f', 'p' };
    spawnMonster(0, randY, randType);
}

void processMonsters() {
    // Walk backwards so we can erase in-place
    for (int i = int(monsters.size()) - 1; i >= 0; --i) {
        Monster& m = monsters[i];
        gameBoard.board[m.x][m.y]='.';

        // move monster down by 1
        m.x += 1;


        if (m.x >= 32) {
            // monster has passed the bottom row
            if(hearts>0)
              --hearts;
            q.push(6);
            // lose a heart
           
            monsters.erase(i);      
        }else if(m.x < 32){
            gameBoard.board[m.x][m.y] = m.monsterType;

        }
        
    }
}

void processProjectile() {


    // Iterate backwards so erasing doesn’t skip any
    for (int i = int(magazine.size()) - 1; i >= 0; --i) {
        auto& p = magazine[i];
        gameBoard.board[p.x][p.y]='.';
       // 2) Update position vertically only
        if (p.direction == 0) {
            p.x -= 1;   // player shot goes up
        } else if (p.direction == 1) {
            p.x += 1;   // boss shot goes down
        }

        // 3) Check out-of-bounds
        if (p.x < 0 || p.x >= 32 || p.y < 0 || p.y >= 32) {
            // Remove from magazine
            magazine.erase(i);
        }else{
          gameBoard.board[p.x][p.y]='b';
          if(p.player){
            gameBoard.board[p.x][p.y]='b';

          }else{
            gameBoard.board[p.x][p.y]='a';

          }
        }
    }
}

void spawnProjectile(char type) {
    
    bool isPlayer = true;
    int x = player.x-1;
    int y = player.y;

    // gameBoard.board[x][y] = 'b';

    magazine.push_back( Projectile(x-1, y, isPlayer, type, 0));
}

// --- Collision Checker (uses global monsters & magazine) ---
void checkCollisions() {
    for (size_t i = 0; i < monsters.size(); ++i) {
        auto& mon = monsters[i];
        if (mon.hit) continue;  // already dead

        for (size_t j = 0; j < magazine.size(); ++j) {
            auto& proj = magazine[j];
            if (proj.hit) continue;  // already expended

            // same row?
            if (mon.x == proj.x) {
                int colDelta = abs(mon.y - proj.y);

                // either same column (0) or up to three columns away (1, 2, or 3)
                if (colDelta == 0 || colDelta == 1 || colDelta == 2 ) {
                    if (mon.monsterType == proj.type) {

                        // correct type  both vanish
                        mon.hit = true;
                        proj.hit = true;
                        score++;
                    }
                    else if (mon.monsterType != proj.type && proj.player) {
                        // wrong type  only the projectile expends
                        proj.hit = true;
                        if (level != 0)
                            score--;
                    }
                    // in either case, this projectile is done
                    break;
                }
            }
        }
    }
}

// --- Removal of Hit Characters (uses globals monsters & magazine) ---
void removeHitCharacters() {
    // Remove dead monsters
    for (int i = int(monsters.size()) - 1; i >= 0; --i) {
        if (monsters[i].hit) {
            gameBoard.board[monsters[i].x][monsters[i].y] = '.';
            



            monsters.erase(i); // Use index-based erase

        }
    }

    // Remove spent projectiles
    for (int i = int(magazine.size()) - 1; i >= 0; --i) {
        if (magazine[i].hit) {
            gameBoard.board[magazine[i].x][magazine[i].y] = '.';

            magazine.erase(i); // Use index-based erase
        }
    }
}



// 1-NN predictor: returns the boss's next action based on the query features. Taking info in memory not RAM
int predictMove1NN(int qdx, int qdy, int qdistX, int qdistY, int qproj) {
    const int n = 100;            // or 100 if you want all 100 entries
    int bestIdx = 0;
    unsigned int bestDist = 0xFFFF;

    for (int i = 0; i < n; ++i) {
        // Read each table entry from flash (PROGMEM) into a regular 'int'
        int vx     = (int)pgm_read_word_near(dx           + i);
        int vy     = (int)pgm_read_word_near(dy           + i);
        int vdistX = (int)pgm_read_word_near(distanceX    + i);
        int vdistY = (int)pgm_read_word_near(distanceY    + i);
        int vproj  = (int)pgm_read_word_near(projectileScan + i);

        // Compute absolute differences
        unsigned int ddx = abs(vx    - qdx);
        unsigned int ddy = abs(vy    - qdy);
        unsigned int dX  = abs(vdistX - qdistX);
        unsigned int dY  = abs(vdistY - qdistY);
        unsigned int dP  = abs(vproj  - qproj);

        // getting the closest point to the input 
        unsigned int distSq = ddx*ddx + ddy*ddy + dX*dX + dY*dY + dP*dP;
        if (distSq < bestDist) {
            bestDist = distSq;
            bestIdx  = i;
        }
    }

    int result = (int)pgm_read_word_near(bossMoves + bestIdx);
    return result;
}



int calcdistanceX(){
    return boss.x - player.x;
}

int calcdistanceY(){
    return boss.y - player.y;
}
// returns the minimum vertical distance (in rows) from the boss to any projectile in the same column as the boss; 32 if none
int projectileCheck() {
    int bestDist = -1;                 // –1 means “no projectile seen yet”
    int bossRow = boss.x;              // boss’s current row
    int bossCol = boss.y;              // boss’s column

    // scan every active projectile
    for (size_t i = 0; i < magazine.size(); ++i) {
        const Projectile& p = magazine[i];

        // only consider projectiles in the same column
        if (p.y == bossCol) {
            int d = p.x - bossRow;     // vertical offset (positive if below boss)
            int absd = (d < 0 ? -d : d);

            if (bestDist < 0 || absd < bestDist) {
                bestDist = absd;
            }
        }
    }

    // if no projectile was seen, return 32
    return (bestDist < 0) ? 32 : bestDist;
}


// Check if player is hit by any projectile immediately left or right
void checkPlayersHit() {
    for (int i = 0; i < magazine.size(); ++i) {
        // Only consider monster‐fired projectiles (player==false)
        if (!magazine[i].player) {
            if (magazine[i].x == player.x && magazine[i].y == player.y) {
                magazine[i].hit = true;
                if(hearts>0)
              --hearts;
                q.push(6);
            }
        }
    }
}

void checkBossHit() {
    for (int i = 0; i < magazine.size(); ++i) {
        if (magazine[i].x == boss.x && magazine[i].y == boss.y && magazine[i].player) {
            boss.hit = true;
            magazine[i].hit = true;
            level = 6;
        }
    }
}

void setBossPosition() {
    gameBoard.board[boss.x][boss.y] = '.';
    switch (boss.direction) {
        case 0: // up
            boss.x--;
            if (boss.x < 0) {
                boss.x = 31; // Loop around to bottom
            }
            break;
        case 1: // down
            boss.x++;
            if (boss.x > 31) {
                boss.x = 0; // Loop around to top
            }
            break;
        case 2: // left
            boss.y--;
            if (boss.y < 0) {
                boss.y = 31; // Loop around to right
            }
            break;
        case 3: // right
            boss.y++;
            if (boss.y > 31) {
                boss.y = 0; // Loop around to left
            }
            break;
        default:
            break;
    }
    gameBoard.board[boss.x][boss.y] = 'B';

}









