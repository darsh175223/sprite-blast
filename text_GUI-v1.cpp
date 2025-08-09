#include <iostream>
#include <vector>
#include <fstream>
#include <locale>
#include <codecvt>
#include <thread>
#include <chrono>
#include <atomic>
#include <cstdint>
#include <random>   
#include <cmath>  
#include <windows.h>

using namespace std;


void spawnMonster(int x_spawn, int y_spawn, char type);
// BOSS AI database
typedef vector<int> vi;

vi dx = {
    //  0–29
    0,  -1,   0,   1,   0,   1,  -1,   1,   0,  -1,
    1,   0,  -1,   0,   1,  -1,   1,  -1,   0,   0,
    0,   1,  -1,   1,  -1,   0,   0,   1,  -1,   0,
    // 30–49
    0,   2,  -2,   1,  -1,   0,   1,  -2,   2,   0,
   -1,   0,  -1,   2,  -2,   1,  -1,   0,   2,  -2,
    // 50–69
    4,   5,   0,   3,  18,  14,   2,   6,   0,   2,
   16,   0,  12,   0,   7,   3,  14,   0,  18,   2,
    // 70–89: varied small/large mixed edge and general
    3,   0,  -3,   0,  10,  -10,  1,   0,   -5,  5,
    0,   8,   0,  -8,   6,   0,   0,  7,    2,  -2,
    9,   0,   0,  -9,  11,   0,   0,  11,   13,  0
};

vi dy = {
    //  0–29
    0,   0,   1,   0,  -1,   1,  -1,   0,  -1,   1,
   -1,   0,   0,   1,   0,   1,  -1,  -1,   1,   0,
    0,   2,  -2,   1,   1,  -3,   4,   0,   0,  -1,
    // 30–49
    1,  -1,   0,   2,   0,  -1,   1,   0,  -2,   3,
    0,  -2,   2,  -1,   1,   0,  -3,   2,  -1,   0,
    // 50–69
    2,   0,   6,   0,   1,   2,   2,   5,  13,   1,
    0,   3,   3,  -1,   0,  -1,   1,  -1,   2,   0,
    // 70–89
    0,   3,   0,  -3,   0,   0,   0,  -4,    0,  0,
   -6,   0,   9,   0,  -7,   4,   5,   0,   -2,   2,
    0,  10,   8,   0,   0,  12,   3,   0,    0,   9
};

// vector from boss to player (X axis: + = right; Y axis: + = down)
vi distanceX = {
    //  0–29
    20,   5,  10, -15,   0,   8,  12,   3,  25,   0,
   -20,   0,   5,  -5,  10,   7,  -7, -30,   0,  10,
    40,  35, -40,  50,  -50,   0,  28, -10,  15,  60,
    // 30–49
    60,  -60,   5,  30,  -30,   0,   3,  55,  -55,   0,
    18,  -18,  52, -52,   40,  -40,  10,  -10,  48,  -48,
    // 50–69
    15,  -12,   0,   5,   30,  -35,   5,    4,    0,    2,
    60,    1,  -15,   0,   20,   -8,   7,    0,   50,   -3,
    // 70–89
    12,    0,  -12,   0,    8,   -8,   9,    0,  -9,    9,
     0,   14,    0,  -14,  25,    0,   0,  22,    18,   0,
   -20,   0,    0,   20, -28,    0,    0,  -30,   35,   0
};

vi distanceY = {
    //  0–29
     0,   0,   5,   0, -10,  -8,  12,   2,   0,   3,
    -5,   0,   5,   5,   0,  -7,   7, -10,  15,   0,
    20, -25,  30, -18,   8,  35, -40,  27, -22,  10,
    // 30–49
    -5,   5,  50, -50,   0,  25, -25,  60, -60,  15,
   -15,   2,  -2,   1,  -1,   0,  35, -35,   3,  -3,
    // 50–69
    -5,    0,  20,   8,   2,    5,   5,   -5,   40,   -1,
     0,   -1, -15,   1,   1,    0,   2,   -6,  -10,   0,
    // 70–89
     0,  12,   0,  -12,  0,    0,   0,  -18,    0,   0,
   20,   0,   16,    0,   0,  -22, -24,    0,  10,   -10,
     0,  25,    0,  -25,  30,   -30,  5,     0,    0,   28
};

// nearest projectile vertical distance in rows (–1 = none)
vi projectileScan = {
    //  0–29
   -1,  -1,  -1,  -1,   5,  -1,   3,  -1,  10,  -1,
   -1,  -1,   2,  -1,   1,  -1,   4,   1,  -1,  -1,
    1,   1,   1,  -1,  -1,  -1,   2,   7,  -1,   1,
    // 30–49
   -1,   1,   3,  -1,   1,  -1,   2,   1,   4,  -1,
    1,   3,   1,  -1,   1,   2,  -1,   1,   2,  -1,
    // 50–69
   -1,   2,  -1,   3,  -1,   4,  -1,  -1,  -1,  -1,
   -1,  -1,   1,  -1,  -1,  -1,   2,   1,   1,  -1,
    // 70–89
   -1,   0,   2,   1,  -1,   3,   2,  -1,   1,   2,
    3,  -1,   2,  -1,   4,   1,  -1,   3,    1,  -1,
    2,   1,  -1,   2,  -1,   1,   0,   1,    2,  -1
};

// boss's "correct" action
// 0=shoot,1=up,2=down,3=left,4=right,5=spawnMinion
vi bossMoves = {
    //  0–29
     0,   0,   2,   3,   1,   1,   5,   0,   4,   0,
     3,   0,   1,   2,   4,   1,   2,   5,   2,   5,
     5,   5,   5,   5,   5,   5,   2,   3,   1,   5,
    // 30–49
     5,   5,   4,   5,   5,   0,   1,   5,   0,   0,
     5,   2,   5,   5,   1,   0,   5,   0,   5,   5,
    // 50–69
     4,   1,   5,   0,   5,   3,   0,   4,   1,   0,
     5,   0,   5,   0,   0,   1,   5,   0,   0,   3,
    // 70–89
     3,   1,   2,   4,   1,   0,   2,   3,   1,   2,
     4,   0,   2,   0,   0,   0,   1,   0,   2,   0
};


HANDLE hSerial;

// Initialize and configure the serial port
bool InitializeSerialPort(const char* port_name, DWORD baud_rate) {
    // Open the serial port
    hSerial = CreateFileA(port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        return false;
    }

    // Configure serial port parameters
    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return false;
    }

    dcbSerialParams.BaudRate = baud_rate;
    dcbSerialParams.ByteSize = 8;
    dcbSerialParams.StopBits = ONESTOPBIT;
    dcbSerialParams.Parity = NOPARITY;
    dcbSerialParams.fOutxCtsFlow = FALSE;
    dcbSerialParams.fOutxDsrFlow = FALSE;
    dcbSerialParams.fDtrControl = DTR_CONTROL_DISABLE;
    dcbSerialParams.fRtsControl = RTS_CONTROL_DISABLE;
    dcbSerialParams.fOutX = FALSE;
    dcbSerialParams.fInX = FALSE;

    if (!SetCommState(hSerial, &dcbSerialParams)) {
        CloseHandle(hSerial);
        return false;
    }

    // Set communication timeouts
    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50;
    timeouts.ReadTotalTimeoutConstant = 1000;
    timeouts.ReadTotalTimeoutMultiplier = 10;
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        CloseHandle(hSerial);
        return false;
    }

    return true;
}

// Read a single byte from the serial port
bool ReadSerialByte(unsigned char* buffer, DWORD* bytes_read) {
    return ReadFile(hSerial, buffer, 1, bytes_read, NULL) && *bytes_read == 1;
}

// Write a single byte to the serial port
bool WriteSerialByte(unsigned char byte_to_send, DWORD* bytes_written) {
    // Clear receive buffer before sending
    PurgeComm(hSerial, PURGE_RXCLEAR);
    return WriteFile(hSerial, &byte_to_send, 1, bytes_written, NULL);
}

// Close the serial port
void CloseSerialPort() {
    CloseHandle(hSerial);
}

int level = 3;
int hearts = 5;
vector<int> numMonsters = {4, 2, 2, 2};
vector<int> speed = {1000, 1000, 800, 600};
int score = 5;
int gameState = 1; //0 = tutorial, 1 = game running, 2 = boss fight, 3 = you won 


// Atomic variable to hold the timer value in milliseconds
std::atomic<uint64_t> timerValue{0};

// Flag to control the timer thread loop
std::atomic<bool> keepRunning{true};

// Timer thread function
void timerThread() {
    using namespace std::chrono;
    auto start = steady_clock::now();

    while (keepRunning.load(std::memory_order_acquire)) {
        auto now = steady_clock::now();
        int elapsed = static_cast<int>(duration_cast<milliseconds>(now - start).count());

        timerValue.store(elapsed, std::memory_order_release);

    }
}

// --- Base Character Class ---
class Character {
public:
    int x;
    int y;
    wstring symbol;

    Character(int x_ = 0, int y_ = 0, wstring symbol_ = L"\u2588")
        : x(x_), y(y_), symbol(symbol_) {}

    virtual ~Character() = default;

    void setPosition(int newX, int newY) {
        x = newX;
        y = newY;
    }
};

// --- Player Derived Class ---
class Player : public Character {
public:
    int direction; //0 -> up, 1 -> down, 2-> left, 3-> right
    bool shoot;
    char bulletType;


    Player(int x_ = 27, int y_ = 75, int direction_ = 0, bool shoot_ = false, 
           int bulletType_ = 0, wstring symbol_ = L"\x2600")
        : Character(x_, y_, symbol_), 
          direction(direction_), shoot(shoot_), bulletType(bulletType_) {}
};


// --- Monster Derived Class ---
class Monster : public Character {
public:
    char monsterType;
    bool hit = false;

    Monster(int x_ = 0, int y_ = 0, char monsterType_ = 0)
        : Character(x_, y_, L"\x26D4"), monsterType(monsterType_) {
        switch (monsterType_) {
            case 'n': symbol = L"\x004E"; break; // Aries L"\x2648"
            case 'c': symbol = L"\x0043"; break; // Taurus L"\x2649"
            case 'f': symbol = L"\x0046"; break; // Gemini L"\x264A"
            case 'p': symbol = L"\x0050"; break; // pisces L"\x2653"
            default: symbol = L"\x26D4"; break; // No Entry
        }
    }
};

class Projectile : public Character {
public:
    bool player;      // true if shot by player, false if by monster
    char type;        // projectile type
    int direction;    // direction of the projectile
    bool hit = false;


    Projectile(int x_, int y_, bool player_, char type_, int direction_, wstring symbol_ = L"\u25CF")
        : Character(x_, y_, symbol_), player(player_), type(type_), direction(direction_) {}
};




// --- Game Board Class ---
class GameBoard {
public:
    static const int ROWS = 27;
    static const int COLS = 150;
    vector<vector<wstring>> board;

    GameBoard() {
        board.resize(ROWS, vector<wstring>(COLS, L" "));
    }

    void clearBoard() {
        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < COLS; ++col) {
                board[row][col] = L" ";
            }
        }
    }

    void updatePositions(const vector<Character*>& characters) {
        clearBoard();
        for (const auto& ch : characters) {
            if (ch && ch->x >= 0 && ch->x < ROWS && ch->y >= 0 && ch->y < COLS) {
                board[ch->x][ch->y] = ch->symbol;
            }
        }
    }

    wstring getCell(int row, int col) const {
        if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
            return board[row][col];
        }
        return L"";
    }

    void setCell(int row, int col, const wstring& value) {
        if (row >= 0 && row < ROWS && col >= 0 && col < COLS) {
            board[row][col] = value;
        }
    }

    void displayBoard() const {
        wofstream file("textDisplay.txt", ios::out | ios::trunc);
        if (!file.is_open()) {
            wcerr << L"Error opening textDisplay.txt\n";
            return;
        }

        file.imbue(locale(file.getloc(), new codecvt_utf8<wchar_t>));

        // Draw the game matrix
        for (int row = 0; row < ROWS; ++row) {
            for (int col = 0; col < COLS; ++col) {
                file << board[row][col];
                if (!file.good()) {
                    wcerr << L"Error writing at " << row << L"," << col << endl;
                    return;
                }
            }
            file << L'\n';
        }

        // Append status line below the board
        file << L"Hearts: " << ::hearts << L"    "
             << L"Score: " << ::score << L"    "
             << L"Level: " << ::level << L"\n";
    }
        // Add this method inside the GameBoard class:
    void declareVictory() const {
        // Open the display file and clear contents
        std::wofstream file("textDisplay.txt", std::ios::out | std::ios::trunc);
        if (!file.is_open()) {
            std::wcerr << L"Error opening textDisplay.txt for victory declaration\n";
            return;
        }

        // Ensure UTF-8 output
        file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>()));

        // Big ASCII art for "YOU WON!"
        file << LR"EOF(

 .----------------.  .----------------.  .----------------.   .----------------.  .----------------.  .-----------------.
| .--------------. || .--------------. || .--------------. | | .--------------. || .--------------. || .--------------. |
| |  ____  ____  | || |     ____     | || | _____  _____ | | | | _____  _____ | || |     ____     | || | ____  _____  | |
| | |_  _||_  _| | || |   .'    `.   | || ||_   _||_   _|| | | ||_   _||_   _|| || |   .'    `.   | || ||_   \|_   _| | |
| |   \ \  / /   | || |  /  .--.  \  | || |  | |    | |  | | | |  | | /\ | |  | || |  /  .--.  \  | || |  |   \ | |   | |
| |    \ \/ /    | || |  | |    | |  | || |  | '    ' |  | | | |  | |/  \| |  | || |  | |    | |  | || |  | |\ \| |   | |
| |    _|  |_    | || |  \  `--'  /  | || |   \ `--' /   | | | |  |   /\   |  | || |  \  `--'  /  | || | _| |_\   |_  | |
| |   |______|   | || |   `.____.'   | || |    `.__.'    | | | |  |__/  \__|  | || |   `.____.'   | || ||_____|\____| | |
| |              | || |              | || |              | | | |              | || |              | || |              | |
| '--------------' || '--------------' || '--------------' | | '--------------' || '--------------' || '--------------' |
 '----------------'  '----------------'  '----------------'   '----------------'  '----------------'  '----------------' 

                                            )EOF";
        // file << L"\n          *** Congratulations! YOU WON! ***\n";
        file.flush();
    }

    
void gameLoss() const {
    // Open the display file and clear contents
    std::wofstream file("textDisplay.txt", std::ios::out | std::ios::trunc);
    if (!file.is_open()) {
        std::wcerr << L"Error opening textDisplay.txt for loss declaration\n";
        return;
    }

    // Ensure UTF-8 output
    file.imbue(std::locale(file.getloc(), new std::codecvt_utf8<wchar_t>()));

    // Big ASCII art for "YOU LOST!"
    file << LR"EOF(


░██████╗░░█████╗░███╗░░░███╗███████╗  ░█████╗░██╗░░░██╗███████╗██████╗░
██╔════╝░██╔══██╗████╗░████║██╔════╝  ██╔══██╗██║░░░██║██╔════╝██╔══██╗
██║░░██╗░███████║██╔████╔██║█████╗░░  ██║░░██║╚██╗░██╔╝█████╗░░██████╔╝
██║░░╚██╗██╔══██║██║╚██╔╝██║██╔══╝░░  ██║░░██║░╚████╔╝░██╔══╝░░██╔══██╗
╚██████╔╝██║░░██║██║░╚═╝░██║███████╗  ╚█████╔╝░░╚██╔╝░░███████╗██║░░██║
░╚═════╝░╚═╝░░╚═╝╚═╝░░░░░╚═╝╚══════╝  ░╚════╝░░░░╚═╝░░░╚══════╝╚═╝░░╚═╝

)EOF";

    file.flush();
}

};


//Global vars
GameBoard board;

// Create some characters
Player player(20, 10);
vector<Monster> monsters;

vector<Projectile> magazine;

char playerMove = ' ';

class Boss : public Character {
public:
    int direction; //0 -> up, 1 -> down, 2-> left, 3-> right
    bool hit= false;
    int numMinionCharge = 2; 


    Boss(int x_ = 27, int y_ = 75, int direction_ = 0,  wstring symbol_ = L"\x26D4")
        : Character(x_, y_, symbol_), 
          direction(direction_) {}
    
    void setBossMove(int move){
        switch (move)
        {
        case 0:
            boss_shoot();
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
        bool isPlayer = false;

        wstring symbol = L"\u2693";
        magazine.push_back( Projectile(x, y, isPlayer, 'n', 1, symbol));

    }

    void spawnMinion(){
        static random_device rd;
        static mt19937 gen(rd());
        constexpr char types[] = { 'n', 'c', 'f', 'p' };
        uniform_int_distribution<int> distType(0, 3);
        char randMonsterType = types[distType(gen)];

        spawnMonster(x, y, randMonsterType );
    }
};




Boss boss(15, 75);
void setBossPosition() {
    switch (boss.direction) {
        case 0: // up
            boss.x--;
            if (boss.x < 0) {
                boss.x = 26; // Loop around to bottom
            }
            break;
        case 1: // down
            boss.x++;
            if (boss.x > 26) {
                boss.x = 0; // Loop around to top
            }
            break;
        case 2: // left
            boss.y--;
            if (boss.y < 0) {
                boss.y = 149; // Loop around to right
            }
            break;
        case 3: // right
            boss.y++;
            if (boss.y > 149) {
                boss.y = 0; // Loop around to left
            }
            break;
    }
}



void setPlayerMove() {
    unsigned char buffer[1];
    DWORD bytes_read;

    // Try to read one byte; if none available, just keep the last playerMove
    if (!ReadSerialByte(buffer, &bytes_read) || bytes_read != 1) {
        return;
    }

    unsigned char instruction = buffer[0];

    // Lower 3 bits (0–7) represent movement codes
    unsigned char moveBits = instruction & 0x07;

    // Upper bits represent projectile types when nonzero
    unsigned char highBits = instruction >> 3;    // shifts bits 3–7 down to 0–4
    bool        isProjectile = (highBits != 0);   

    if (!isProjectile) {
        // Movement
        switch (moveBits) {
            case 1: playerMove = 's'; break;
            case 2: playerMove = 'w'; break;
            case 3: playerMove = 'a'; break;
            case 4: playerMove = 'd'; break;
            default: playerMove = ' '; break;
        }
    } else {
        // Projectile: projType in 1–7 but we only care about 1–4
        unsigned char projType = highBits & 0x07;
        switch (projType) {
            case 1: playerMove = 'n'; break;  // normal
            case 2: playerMove = 'c'; break;  // close sonar
            case 3: playerMove = 'f'; break;  // far sonar
            case 4: playerMove = 'p'; break;  // photoresistor
            default: playerMove = ' '; break;
        }
    }

    // Debug print if you like:
    if (playerMove != ' ')
        cout << "Decoded move/type: " << playerMove << endl;
}




void playerPositionChanger(Player* p){
    if (playerMove == 'w') {
        (p->x)--;
    } else if (playerMove == 'a') {
        (p->y)--;
    } else if (playerMove == 's') {
        (p->x)++;
    } else if (playerMove == 'd') {
        (p->y)++;
    }
}

bool playerMoved() {
    return (playerMove == 'w' || playerMove == 'a' || playerMove == 's' || playerMove == 'd');
}

bool playerShoot() {
    //n = normal, p  = photoresistor, c = close to sonar, f = far from sonar
    // cout<<(playerMove == 'n' || playerMove == 'c' || playerMove == 'f' || playerMove == 'p')<<playerMove<<"."<<endl;
    return (playerMove == 'n' || playerMove == 'c' || playerMove == 'f' || playerMove == 'p');
}


void processMonsters(vector<Monster>& monsters) {
    // Walk backwards so we can erase in-place
    for (int i = int(monsters.size()) - 1; i >= 0; --i) {
        Monster& m = monsters[i];
        // move monster down by 1
        m.x += 1;

        if (m.x >= 27) {
            // monster has passed the bottom row
            --hearts;                             // lose a heart
            monsters.erase(monsters.begin() + i); // remove this monster
        }
        else if (m.x > 26) {
            // clamp to last valid row just in case
            m.x = 26;
        }
    }
}

//Create the processProjectile function  HERE!!
void processProjectile(std::vector<Projectile>& magazine) {
    // Move each projectile according to its direction
    for (auto& proj : magazine) {
        switch (proj.direction) {
            case 0: proj.x--; break;   // up
            case 1: proj.x++; break;   // down
            case 2: proj.y--; break;   // left
            case 3: proj.y++; break;   // right
        }
    }

    // Manually remove any projectiles that have gone out of bounds
    // (iterate backwards so erase() doesn't invalidate future indices)
    for (int i = int(magazine.size()) - 1; i >= 0; --i) {
        const auto& p = magazine[i];
        if (p.x < 0 || p.x >= 27 || p.y < 0 || p.y >= 150) {
            magazine.erase(magazine.begin() + i);
        }
    }
}


void spawnProjectile(Player* player) {
    // Example: spawn a projectile just above the player, going up
    
    bool isPlayer = true;
    char type = playerMove; // or any type you want
    int direction = player->direction; //0 -> up, 1 -> down, 2-> left, 3-> right
    int x = player->x;
    int y = player->y;



    wstring symbol = L"\u26A1";
    magazine.push_back( Projectile(x, y, isPlayer, type, direction, symbol));
}

//FINISH THIS IMPLEMETNATIONS
void spawnMonster(int x_spawn, int y_spawn, char type){
    Monster newMonster(x_spawn, y_spawn, type);
    monsters.push_back(newMonster);
}


// new: spawn a monster at x=0, random y and random type
void spawnRandomMonster() {
    static std::random_device rd;
    static std::mt19937 gen(rd());

    // y in [0, COLS-1]
    std::uniform_int_distribution<int> distY(0, GameBoard::COLS - 1);

    // pick one of 'n','c','f','p'
    constexpr char types[] = { 'n', 'c', 'f', 'p' };
    std::uniform_int_distribution<int> distType(0, 3);

    int y = distY(gen);
    char type = types[distType(gen)];

    spawnMonster(0, y, type);
}

// --- Collision Checker (uses global monsters & magazine) ---

void checkCollisions() {
    for (auto& mon : monsters) {
        if (mon.hit) continue;  // already dead

        for (auto& proj : magazine) {
            if (proj.hit) continue;  // already expended

            // same row?
            if (mon.x == proj.x) {
                int colDelta = std::abs(mon.y - proj.y);

                // either same column (0) or exactly two columns away (2)
                if (colDelta == 0 || colDelta == 1 || colDelta == 2 || colDelta == 3 ) {
                    if (mon.monsterType == proj.type) {
                        // correct type → both die
                        mon.hit  = true;
                        proj.hit = true;
                        score++;
                    }
                    else if(mon.monsterType != proj.type && proj.player){
                        // wrong type → only the projectile expends
                        proj.hit = true;
                        if(gameState!=0)
                            score--;
                    }
                    // in either case, this projectile is done
                    break;
                }
            }
        }
    }
}

//check if boss is hit
void checkBossHit(){
    for (auto& proj : magazine){
        if(proj.x==boss.x && proj.y == boss.y && proj.player){
            boss.hit = true;
            proj.hit = true; 
            gameState = 3;   

        }
    }

}

//check if player is hit
void checkPlayersHit(){
    for (auto& proj : magazine){
        if(proj.x==player.x && proj.y == player.y){
            proj.hit = true; 
             hearts--;
        }
    }

}

// 1-NN predictor: returns the boss's next action based on the query features.
int predictMove1NN(int qdx, int qdy, int qdistX, int qdistY, int qproj) {
    size_t n = dx.size();            // assume all "feature" vectors and move are the same length

    int bestIdx  = 0;
    int bestDist = numeric_limits<int>::max();

    for (size_t i = 0; i < n; ++i) {
        int ddx   = dx[i]         - qdx;
        int ddy   = dy[i]         - qdy;
        int dX    = distanceX[i]  - qdistX;
        int dY    = distanceY[i]  - qdistY;
        int dProj = projectileScan[i] - qproj;

        int distSq = ddx*ddx + ddy*ddy + dX*dX + dY*dY + dProj*dProj;
        if (distSq < bestDist) {
            bestDist = distSq;
            bestIdx  = static_cast<int>(i);
        }
    }

    return bossMoves[bestIdx];
}
//TODO: distanceX/Y calc + projectileScan

int calcdistanceX(){
    return boss.x - player.x;
}

int calcdistanceY(){
    return boss.y - player.y;
}
//check dist to nearest projectile
int projectileCheck(){

    for(int i = boss.x; i<27; i++ ){
        if(board.getCell(i, boss.y) == L"n" || board.getCell(i, boss.y) == L"c" || board.getCell(i, boss.y) == L"p" || board.getCell(i, boss.y) == L"f"){
            return i-boss.x;
        }
    }
    return -1;

}



// --- Removal of Hit Characters (uses globals monsters & magazine) ---
void removeHitCharacters() {
    // Remove dead monsters
    for (int i = int(monsters.size()) - 1; i >= 0; --i) {
        if (monsters[i].hit) {
            monsters.erase(monsters.begin() + i);
        }
    }

    // Remove spent projectiles
    for (int i = int(magazine.size()) - 1; i >= 0; --i) {
        if (magazine[i].hit) {
            magazine.erase(magazine.begin() + i);
        }
    }
}

bool monsterSpawn = true;
int tutorialCount = 0;
bool tutorialMonsterSpawn1 = true;
bool tutorialMonsterSpawn2 = true;
bool tutorialMonsterSpawn3 = true;
bool tutorialMonsterSpawn4 = true;


void gameloop(){
    cout<<"Start"<<endl;
    //for non-instantanuas changes
    int lastMSValue = 0;
    int lastPlayerX = 0;
    int lastPlayerY = 0;

    //for instantanuas changes
    int last_inst_MSValue = 0;
    int last_inst_PlayerX = 0;
    int last_inst_PlayerY = 0;


    int j = 0;
    while(true){
        int ms= timerValue.load(memory_order_acquire);

        if(hearts<=0 || score<0){
            cout<<"game end"<<endl;
            board.gameLoss();

            return;
        }
        
       if((ms%100)>0 && (ms%100)<26){
            setPlayerMove();
            if(playerMoved()){
                    playerPositionChanger(&player);
                }
                //shooting logic
                if(playerShoot()){     
                    spawnProjectile(&player);   
                }
       }



        if(gameState==0){
            //run tutoiral
            // cout<<"tutorial"<<j<<endl;
            // j++;
          

            if(ms>1000 && ms<1205 && tutorialMonsterSpawn1){

                spawnMonster(0, 10, 'n');
                // cout<<"spawned1"<<endl;
                tutorialMonsterSpawn1=false;
                tutorialCount++;

            }
            if(ms==4000 && tutorialMonsterSpawn2){
                spawnMonster(0, 13, 'c');
                tutorialMonsterSpawn2=false;
                // cout<<"spawned2"<<endl;
                                tutorialCount++;


            }
            if(ms==6000&& tutorialMonsterSpawn3){
                spawnMonster(0, 16, 'f');
                tutorialMonsterSpawn3=false;
                                // cout<<"spawned3"<<endl;

                                tutorialCount++;


            }
            if(ms==8000 && tutorialMonsterSpawn4){
                spawnMonster(0, 19, 'p');
                tutorialMonsterSpawn4=false;
                                // cout<<"spawned4"<<endl;

                                tutorialCount++;


            }

            
                //monster movement
             if(ms%(speed[level])==0 && ms!=0)
                processMonsters(monsters);


            //proejctile and rnderign logic
            if(ms%100==0 && ms!=0){

               
                

                processProjectile(magazine);
                // Put them in a vector of Character*
                vector<Character*> characters = { &player };
                for (auto& monster : monsters) {
                    characters.push_back(&monster);
                }
                for (auto& proj : magazine) {
                    characters.push_back(&proj);
                }
                //clossion check
                checkCollisions();
                removeHitCharacters();
                // Update and display
                // cout<<"Display changed"<<endl;
                board.updatePositions(characters);
                board.displayBoard();
                
            }
            if(tutorialCount==4 && monsters.size()==0){
                gameState=1;
                level++;
                cout<<"end turorial"<<endl;

            }



        }else if(gameState == 1 && level<4){

            //do player movement logic stuff
            if(ms%(speed[level])==0 && ms!=0){
                 //monster movement
                processMonsters(monsters);

            }


            //monster spawning mechanics
            if(ms%(5*speed[level])==0 && ms!=0 && monsterSpawn && numMonsters[level]>0){
                spawnRandomMonster();
                monsterSpawn=false;
                numMonsters[level] = numMonsters[level]-1;
                cout<<numMonsters[level]<<endl;

            }else if(ms%(5*speed[level])==0 && ms!=0 && !monsterSpawn){
                monsterSpawn=true;
            }


            //proejctile and rnderign logic
            if(ms%100==0 && ms!=0){
        
                processProjectile(magazine);
                // Put them in a vector of Character*
                vector<Character*> characters = { &player };
                for (auto& monster : monsters) {
                    characters.push_back(&monster);
                }
                for (auto& proj : magazine) {
                    characters.push_back(&proj);
                }
                //clossion check
                checkCollisions();
                removeHitCharacters();
                // Update and display
                // cout<<"Display changed"<<endl;
                board.updatePositions(characters);
                board.displayBoard();
                
            }
            
            if(numMonsters[level]<=0 && monsters.size()<1){
                cout<<numMonsters[level]<<endl;

                level++;
                // cout<<"abcdef: "+level<<endl;
                cout<<"abcd"<<endl;

            }
            if(level == 4){
                gameState=2;

            }


        }else if(gameState==2){
            //Boss fight

            //calc foresight
            if(ms-lastMSValue>1000){
                // cout<<"Vector: ("<<abs(player.x- lastPlayerX)<<", "<<abs(player.y- lastPlayerY)<<")"<<endl;
                int bossMOVE = predictMove1NN(player.x- lastPlayerX, player.y- lastPlayerY, calcdistanceX(), calcdistanceY(), projectileCheck());
                boss.setBossMove(bossMOVE);
                cout<<bossMOVE<<endl;
                setBossPosition();
                lastPlayerX = player.x;
                lastPlayerY = player.y;
                lastMSValue = ms;

            }

            //calc instantannus
            if(ms-last_inst_MSValue>100){
                // cout<<"Vector: ("<<abs(player.x- lastPlayerX)<<", "<<abs(player.y- lastPlayerY)<<")"<<endl;

                int bossMOVE = predictMove1NN(player.x- lastPlayerX, player.y- lastPlayerY, calcdistanceX(), calcdistanceY(), projectileCheck());
                boss.setBossMove(bossMOVE);
                cout<<bossMOVE<<endl;
                setBossPosition();
                last_inst_PlayerX = player.x;
                last_inst_PlayerY = player.y;
                last_inst_MSValue = ms;

            }

            //do player movement logic stuff
            if(ms%1000==0 && ms!=0){
                 //monster movement
                 cout<<"processing"<<endl;
                processMonsters(monsters);

            }    

            //proejctile and rnderign logic
            if(ms%100==0 && ms!=0){
                
                processProjectile(magazine);
                // Put them in a vector of Character*
                vector<Character*> characters = { &player, &boss };
                for (auto& monster : monsters) {
                    characters.push_back(&monster);
                }
                for (auto& proj : magazine) {
                    characters.push_back(&proj);
                }
                //clossion check
                checkCollisions();
                removeHitCharacters();
                checkBossHit();
                checkPlayersHit();
                // Update and display
                // cout<<"Display changed"<<endl;
                board.updatePositions(characters);
                board.displayBoard();
                
            }

            if(ms%5000==0 & ms!=0){
                boss.numMinionCharge= boss.numMinionCharge+1;
                cout<<boss.numMinionCharge<<endl;
            }

            



            

        }else if(gameState ==3){
            //calling displayVictory funciton HERE

            board.declareVictory();
            return;
            
        }
        

    }
}

// --- Main Function ---
int main() {
    thread t(timerThread);
    if (!InitializeSerialPort("\\\\.\\COM5", CBR_9600)) {
        return 1;
    }
    
  
    

    gameloop();
    
   

    
 

    

    wcout << L"Game board written to textDisplay.txt\n";
    return 0;
}