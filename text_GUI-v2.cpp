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
#include <queue>
#include <mutex>              // Added for std::mutex
#include <condition_variable> // Added for std::condition_variable

using namespace std;

// Opcode instructions
// 0 - level info = updates level info if 0 = tutorial, 1-3 = normal levels, 4 = boss level, 5 = game over, 6 = victory, 7 = pre-game
// 1 - score/heart increment/decrement
// 2 - move = moves any character, fetches next two bytes in queue for x and y coordinates
// 3 - spawn/despawn = spawns or despawns any character, fetches next two bytes in queue for x and y coordinates

// Shared game state variables
unsigned char level = 0;
unsigned char hearts = 5;
unsigned char score = 0;

// Character list (comments from original)
//0 = space
//1-4 = monster
//5 = boss
//6 = player
//7 = player bullet
//8 = boss bullet

HANDLE hSerial;

// --- Threading and Synchronization Globals ---
std::queue<unsigned char> g_dataQueue;
std::mutex g_queueMutex;
std::condition_variable g_queueCondVar;

std::mutex g_gameStateMutex; // Protects level, hearts, score, and board
std::atomic<bool> g_stopThreads(false);
std::atomic<bool> g_gameStateChanged(false);
std::atomic<bool> g_gameIsOver(false);
std::atomic<bool> g_victoryIsDeclared(false);

// Forward declaration for the new instruction processing function
void applyInstructionPacket(const std::vector<unsigned char>& packet);


// Initialize and configure the serial port
bool InitializeSerialPort(const char* port_name, DWORD baud_rate) {
    hSerial = CreateFileA(port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL,
                          OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hSerial == INVALID_HANDLE_VALUE) {
        cerr << "Error opening serial port: " << GetLastError() << endl;
        return false;
    }

    DCB dcbSerialParams = {0};
    dcbSerialParams.DCBlength = sizeof(dcbSerialParams);
    if (!GetCommState(hSerial, &dcbSerialParams)) {
        cerr << "Error getting comm state: " << GetLastError() << endl;
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
        cerr << "Error setting comm state: " << GetLastError() << endl;
        CloseHandle(hSerial);
        return false;
    }

    COMMTIMEOUTS timeouts = {0};
    timeouts.ReadIntervalTimeout = 50; // Non-blocking read, return immediately if no data
    timeouts.ReadTotalTimeoutConstant = 0; // Total timeout for read operation
    timeouts.ReadTotalTimeoutMultiplier = 0; // No extra time per byte
    timeouts.WriteTotalTimeoutConstant = 50;
    timeouts.WriteTotalTimeoutMultiplier = 10;

    if (!SetCommTimeouts(hSerial, &timeouts)) {
        cerr << "Error setting comm timeouts: " << GetLastError() << endl;
        CloseHandle(hSerial);
        return false;
    }
    cout << "Serial port " << port_name << " initialized successfully." << endl;
    return true;
}

// Read a single byte from the serial port
bool ReadSerialByte(unsigned char* buffer, DWORD* bytes_read) {
    return ReadFile(hSerial, buffer, 1, bytes_read, NULL);
}

// Write a single byte to the serial port (kept for completeness, not used by reader)
bool WriteSerialByte(unsigned char byte_to_send, DWORD* bytes_written) {
    PurgeComm(hSerial, PURGE_RXCLEAR); // Original code had this
    return WriteFile(hSerial, &byte_to_send, 1, bytes_written, NULL);
}

// Close the serial port
void CloseSerialPort() {
    if (hSerial != INVALID_HANDLE_VALUE) {
        CloseHandle(hSerial);
        hSerial = INVALID_HANDLE_VALUE;
    }
}

class GameBoard {
public:
    static constexpr int ROWS = 27, COLS = 150;
    vector<vector<wstring>> board;

    GameBoard() {
        board.assign(ROWS, vector<wstring>(COLS, L" "));
    }

    wstring getCell(int r, int c) const {
        return (r >= 0 && r < ROWS && c >= 0 && c < COLS)
             ? board[r][c]
             : L" ";
    }

    void setCell(int r, int c, const wstring& val) {
        if (r >= 0 && r < ROWS && c >= 0 && c < COLS)
            board[r][c] = val;
    }

    void clearBoard() {
        for (auto& row : board)
            for (auto& cell : row)
                cell = L" ";
    }

    void displayBoard() const { // Called with g_gameStateMutex locked
        ofstream MyFile("textDisplay.txt");
        if (!MyFile) {
            wcerr << L"Error opening textDisplay.txt for display\n";
            return;
        }

        std::wstring output;
        // Estimate size: ROWS * (COLS * typical_wstring_char_len + newline) + status_line_len
        // Assuming typical_wstring_char_len is 1 for simple characters.
        output.reserve((COLS + 1) * ROWS + 100); 

        for (int r = 0; r < ROWS; ++r) {
            for (int c = 0; c < COLS; ++c) {
                output += board[r][c];
            }
            output += L'\n';
        }

        output += L"Hearts: " + std::to_wstring(int(hearts))
           + L"    Score: " + std::to_wstring(int(score))
           + L"    Level: " + std::to_wstring(int(level))
           + L"\n";

        std::wstring_convert<std::codecvt_utf8<wchar_t>> converter;
        std::string utf8_output = converter.to_bytes(output);
        
        MyFile << utf8_output;
        MyFile.close();
    }
    
    void declareVictory() const { // Called with g_gameStateMutex locked (or just before stopping all)
        const char* art = u8R"EOF(
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

        ofstream out("textDisplay.txt", ios::binary | ios::trunc);
        if (!out) {
            wcerr << L"Error opening textDisplay.txt for victory declaration\n";
            return;
        }
        const unsigned char bom[3] = {0xEF,0xBB,0xBF};
        out.write(reinterpret_cast<const char*>(bom), 3);
        out << art;
        cout << "Victory declared to file." << endl;
    }

    void gameLoss() const { // Called with g_gameStateMutex locked (or just before stopping all)
        const char* art = u8R"EOF(
░██████╗░░█████╗░███╗░░░███╗███████╗  ░█████╗░██╗░░░██╗███████╗██████╗░
██╔════╝░██╔══██╗████╗░████║██╔════╝  ██╔══██╗██║░░░██║██╔════╝██╔══██╗
██║░░██╗░███████║██╔████╔██║█████╗░░  ██║░░██║╚██╗░██╔╝█████╗░░██████╔╝
██║░░╚██╗██╔══██║██║╚██╔╝██║██╔══╝░░  ██║░░██║░╚████╔╝░██╔══╝░░██╔══██╗
╚██████╔╝██║░░██║██║░╚═╝░██║███████╗  ╚█████╔╝░░╚██╔╝░░███████╗██║░░██║
░╚═════╝░╚═╝░░╚═╝╚═╝░░░░░╚═╝╚══════╝  ░╚════╝░░░░╚═╝░░╚══════╝╚═╝░░╚═╝

)EOF";
        ofstream out("textDisplay.txt", ios::binary | ios::trunc);
        if (!out) {
            wcerr << L"Error opening textDisplay.txt for loss declaration\n";
            return;
        }
        const unsigned char bom[3] = {0xEF,0xBB,0xBF};
        out.write(reinterpret_cast<const char*>(bom), 3);
        out << art;
        cout << "Game loss declared to file." << endl;
    }
};

GameBoard board; // Global game board instance

// --- Thread Functions ---

void serialReaderLoop() {
    cout << "Serial reader thread started." << endl;
    unsigned char byte;
    DWORD bytesRead;

    while (!g_stopThreads.load()) {
        if (ReadSerialByte(&byte, &bytesRead) && bytesRead > 0) {
            {
                std::lock_guard<std::mutex> lock(g_queueMutex);
                g_dataQueue.push(byte);
                cout << "Read byte: " << static_cast<int>(byte) << endl;
            }
            g_queueCondVar.notify_one(); // Notify processor thread
        } else {
            // ReadFile with current timeouts will return immediately if no data.
            // Sleep briefly to avoid busy-waiting and high CPU usage.
            std::this_thread::sleep_for(std::chrono::milliseconds(5)); // Reduced sleep
        }
    }
    cout << "Serial reader thread stopping." << endl;
}

void instructionProcessorLoop() {
    cout << "Instruction processor thread started." << endl;
    while (!g_stopThreads.load()) {
        std::vector<unsigned char> instructionPacket;
        {
            std::unique_lock<std::mutex> lock(g_queueMutex);
            // Wait until (queue is not empty AND we can form a full instruction) OR stop signal
            g_queueCondVar.wait(lock, [&]{
                if (g_stopThreads.load()) return true;
                if (g_dataQueue.empty()) return false;

                unsigned char firstByte = g_dataQueue.front();
                unsigned char instructionType = (firstByte & 0xC0) >> 6;
                size_t requiredBytes = 1;
                if (instructionType == 2 || instructionType == 3) { // move or spawn/despawn
                    requiredBytes += 2;
                }
                return g_dataQueue.size() >= requiredBytes;
            });

            if (g_stopThreads.load()) break; 
            if (g_dataQueue.empty()) continue; // Should be rare due to predicate

            // At this point, queue is not empty, not stopping, and has enough bytes for an instruction.
            unsigned char firstByte = g_dataQueue.front();
            unsigned char instructionType = (firstByte & 0xC0) >> 6;
            size_t requiredBytes = 1;
            if (instructionType == 2 || instructionType == 3) {
                requiredBytes += 2;
            }

            // Double check after wait, predicate should handle this.
            if (g_dataQueue.size() < requiredBytes) {
                 // Not enough data, wait again.
                continue;
            }
            
            for (size_t i = 0; i < requiredBytes; ++i) {
                instructionPacket.push_back(g_dataQueue.front());
                g_dataQueue.pop();
            }
        } // Mutex g_queueMutex is released here

        if (!instructionPacket.empty()) {
            applyInstructionPacket(instructionPacket);
        }
    }
    cout << "Instruction processor thread stopping." << endl;
}


void applyInstructionPacket(const std::vector<unsigned char>& packet) {
    if (packet.empty()) return;

    // Lock game state for modification
    std::lock_guard<std::mutex> gsLock(g_gameStateMutex);

    unsigned char currentOpcode = packet[0];
    unsigned char instruction = (currentOpcode & 0xC0) >> 6; // Top two bits for instruction type

    // cout << "Processing opcode: " << static_cast<int>(currentOpcode) << endl;

    switch (instruction) {
      case 0: // Level info: 00LLLLLL
        level = currentOpcode & 0x3F; // Low 6 bits for level number
        // cout << "Level set to: " << static_cast<int>(level) << endl;
        if (level == 5) { // Game Over
            board.gameLoss(); // This writes to file
            g_gameIsOver = true;
            g_stopThreads = true; // Signal all threads to stop
            g_queueCondVar.notify_all(); // Wake up threads if waiting
        } else if (level == 6) { // Victory
            board.declareVictory(); // This writes to file
            g_victoryIsDeclared = true;
            g_stopThreads = true; // Signal all threads to stop
            g_queueCondVar.notify_all(); // Wake up threads if waiting
        } else {
            g_gameStateChanged = true; // Mark for display update
        }
        break;

      case 1: { // Heart/Score: 01HSIIII (H=Heart/Score, S=Inc/Decr, I=Ignored lower 4 bits)
        // In your original code, it seems you intended 01HSxxxx where xxxx is value.
        // The provided comment "01HSIIII" and logic implies specific bits.
        // Original logic:
        // unsigned char heartOrScore  = (currentOpcode & 0x20) >> 5; // Bit 5
        // unsigned char incOrDecr   = (currentOpcode & 0x10) >> 4; // Bit 4
        // if (heartOrScore == 0) hearts += (incOrDecr ? +1 : -1); else score  += (incOrDecr ? +1 : -1);
        
        // Let's assume the format based on your original code structure
        unsigned char heartOrScoreBit = (currentOpcode & 0x20) >> 5; // 0 for hearts, 1 for score
        unsigned char incOrDecBit = (currentOpcode & 0x10) >> 4;     // 1 for increment, 0 for decrement
        
        if (heartOrScoreBit == 0) { // Hearts
            hearts += (incOrDecBit ? 1 : -1);
            // cout << (incOrDecBit ? "Increment hearts. " : "Decrement hearts. ") << "Hearts: " << static_cast<int>(hearts) << endl;
        } else { // Score
            score += (incOrDecBit ? 1 : -1);
            // cout << (incOrDecBit ? "Increment score. " : "Decrement score. ") << "Score: " << static_cast<int>(score) << endl;
        }
        if (hearts == 0 && !g_gameIsOver && !g_victoryIsDeclared) { // Check for game over by hearts depletion
            board.gameLoss();
            g_gameIsOver = true;
            g_stopThreads = true;
            g_queueCondVar.notify_all();
        } else {
            g_gameStateChanged = true;
        }
        break;
      }

case 2: { // Move: 10DDxxxx (DD = direction), needs 2 more bytes for R, C
    if (packet.size() < 3) return; // Should have been caught by processor loop logic

    unsigned char dirBits = (currentOpcode & 0x30) >> 4; // Bits 4-5 for direction
    int row = static_cast<int>(packet[1]);
    int col = static_cast<int>(packet[2]);

    // cout << "Move: dir=" << static_cast<int>(dirBits) << " r=" << row << " c=" << col << endl;

    if (row < 0 || row >= GameBoard::ROWS || col < 0 || col >= GameBoard::COLS) {
         // cout << "Move out of bounds" << endl;
         break; // Don't try to move if original position is invalid.
    }

    wstring symbol = board.getCell(row, col);
    if (symbol != L" ") { // Only move if there's something there
        board.setCell(row, col, L" "); // Clear old position

        int newRow = row, newCol = col;
        switch (dirBits) {
          case 0: // down
              newRow = row + 1;
              if (newRow > 26) {
                  newRow = 0; // Loop around to top
              }
              break;
          case 1: // up
              newRow = row - 1;
              if (newRow < 0) {
                  newRow = 26; // Loop around to bottom
              }
              break;
          case 2: // left
              newCol = col - 2;
              if (newCol < 0) {
                  newCol = 149; // Proper wrap-around for negative values
              }
              break;
          case 3: // right
              newCol = col + 2;
              if (newCol > 149) {
                  newCol = 0; // Proper wrap-around for positive overflow
              }
              break;
        }

        // No bounds checking needed since we handle wrapping
        board.setCell(newRow, newCol, symbol);
        g_gameStateChanged = true;
    }
    break;
}
      case 3: {  // Spawn/Despawn: 11CCCCxx (CCCC = char ID), needs 2 more bytes for R, C
        if (packet.size() < 3) return; // Should have been caught

        unsigned char charId = (currentOpcode & 0x3C) >> 2; // Bits 2-5 for character ID
        int row = static_cast<int>(packet[1]);
        int col = static_cast<int>(packet[2]);
        
        // cout << "Spawn/Despawn: charID=" << static_cast<int>(charId) << " r=" << row << " c=" << col << endl;

        if (row < 0 || row >= GameBoard::ROWS || col < 0 || col >= GameBoard::COLS) {
            // cout << "Spawn/Despawn out of bounds" << endl;
            break;
        }

        wstring symbol;
        switch (charId) {
            case 0:  symbol = L" "; break;    // Despawn (space)
            case 1:  symbol = L"\x25C9"; break;  // 'n' monster
            case 2:  symbol = L"\x25B3"; break;  // 'c' monster
            case 3:  symbol = L"\x25C7"; break;  // 'f' monster
            case 4:  symbol = L"\x263E"; break;  // 'p' monster
            case 5:  symbol = L"\x26D4"; break;  // boss
            case 6:  symbol = L"\x2600"; break;  // player
            case 7:  symbol = L"\u26A1"; break;  // player projectile
            case 8:  symbol = L"\u2693"; break;  // boss projectile
            default: symbol = L"?"; break; // Unknown character
        }
        board.setCell(row, col, symbol);
        g_gameStateChanged = true;
        break;
      }
       default:
        // Unknown instruction type, ignore.
        // cout << "Unknown instruction type: " << static_cast<int>(instruction) << endl;
        break;
    }
}


int main(){
    // The COM port might need adjustment based on your system.
    // Common ports are COM3, COM4, COM5, etc. For Linux, it would be /dev/ttyUSB0 or /dev/ttyACM0.
    // This code uses CreateFileA, so it's Windows-specific.
    if (!InitializeSerialPort("\\\\.\\COM5", CBR_9600)) { // Adjust COM port as needed
        cerr << "Failed to initialize serial port." << endl;
        return 1;
    }

    // Initial game setup
    {
        std::lock_guard<std::mutex> lock(g_gameStateMutex);
        board.setCell(20, 10, L"\u2600"); // Player initial position
        g_gameStateChanged = true; // Mark for initial display
    }

    std::thread readerThread(serialReaderLoop);
    std::thread processorThread(instructionProcessorLoop);

    auto lastDisplayTime = chrono::steady_clock::now();
    const auto displayInterval = chrono::milliseconds(50); // ~20 FPS

    cout << "Main loop starting. Press Ctrl+C to attempt exit (or game over/victory)." << endl;

    while (true) {
        if (g_gameIsOver.load() || g_victoryIsDeclared.load() || g_stopThreads.load()) {
            break; // Exit main loop if game ended or threads signaled to stop
        }

        bool displayUpdateRequest = g_gameStateChanged.exchange(false); // Check and reset flag

        if (displayUpdateRequest) {
            auto currentTime = chrono::steady_clock::now();
            if (currentTime - lastDisplayTime >= displayInterval) {
                std::lock_guard<std::mutex> lock(g_gameStateMutex);
                // Double-check game state before display, as it might have changed
                // to game over/victory right after the flag was set.
                if (!g_gameIsOver.load() && !g_victoryIsDeclared.load()) {
                    board.displayBoard();
                }
                lastDisplayTime = currentTime;
            } else {
                // If change happened but interval not passed, set it back to true
                // so we try to display on the next iteration.
                g_gameStateChanged = true; 
            }
        }
        
        // Sleep to reduce CPU usage in the main loop
        std::this_thread::sleep_for(std::chrono::milliseconds(10)); 
    }

    cout << "Main loop exited. Signaling threads to stop..." << endl;
    g_stopThreads = true;       // Ensure all threads know to stop
    g_queueCondVar.notify_all(); // Wake up any waiting threads

    if (readerThread.joinable()) {
        readerThread.join();
        cout << "Reader thread joined." << endl;
    }
    if (processorThread.joinable()) {
        processorThread.join();
        cout << "Processor thread joined." << endl;
    }

    // Final state output if game ended through game over/victory flags
    // The board.gameLoss() and board.declareVictory() are already called by the processor thread.
    // This just confirms.
    if (g_gameIsOver.load()) {
        cout << "Game Over." << endl;
    } else if (g_victoryIsDeclared.load()) {
        cout << "Victory Achieved!" << endl;
    }

    CloseSerialPort();
    cout << "Serial port closed. Exiting." << endl;
    return 0;
}