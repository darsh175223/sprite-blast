#ifndef SPIAVR_H
#define SPIAVR_H
#include <avr/io.h>
#include <avr/interrupt.h>
#include <util/delay.h>
//B5 should always be SCK(spi clock) and B3 should always be MOSI. If you are using an
//SPI peripheral that sends data back to the arduino, you will need to use B4 asthe MISO pin.
//The SS pin can be any digital pin on the arduino. Right before sending an 8 bin value with
//the SPI_SEND() funtion, you will need to set your SS pin to low. If you havemultiple SPI
//devices, they will share the SCK, MOSI and MISO pins but should have different SSpins.
//To send a value to a specific device, set it's SS pin to low and all other SS pins to high.
// Outputs, pin definitions
#define PIN_SCK PORTB5//SHOULD ALWAYS BE B5 ON THE ARDUINO
#define PIN_MOSI PORTB3//SHOULD ALWAYS BE B3 ON THE ARDUINO
#define PIN_SS PORTB2
//If SS is on a different port, make sure to change the init to take that into account.
void SPI_INIT(){
    DDRB |= (1 << PIN_SCK) | (1 << PIN_MOSI) | (1 << PIN_SS);//initialize your pins.
    SPCR |= (1 << SPE) | (1 << MSTR); //initialize SPI coomunication
}
void SPI_SEND(char data)
{
    SPDR = data;//set data that you want to transmit
    while (!(SPSR & (1 << SPIF)));// wait until done transmitting
}


// Define pins
#define PIN_A0 PB1      // D9 for Data/Command
#define PIN_RESET PB4   // D12 for Reset
#define PIN_CS PIN_SS   // D10, PB2 (from SPIAVR_H)

// Define common colors (RGB565 format, as provided)
#define COLOR_WHITE     0xFFFF // (11111 111111 11111)
#define COLOR_BLACK     0x0000 // (00000 000000 00000)
#define COLOR_BLUE      0xF800 // (11111 000000 00000)
#define COLOR_GREEN     0x07E0 // (00000 111111 00000)
#define COLOR_RED       0x001F // (00000 000000 11111)
#define COLOR_YELLOW  0xFFE0  // (11111 111111 00000)
#define COLOR_PURPLE  0xF81F  // (11111 000000 11111)  
#define COLOR_CYAN    0x07FF  // (00000 111111 11111)
#define COLOR_ORANGE  0xFD20  // (11111 101000 00000)  

// Helper functions for pin control
void setA0(uint8_t state) { if (state) PORTB |= (1 << PIN_A0); else PORTB &= ~(1 << PIN_A0); }
void setReset(uint8_t state) { if (state) PORTB |= (1 << PIN_RESET); else PORTB &= ~(1 << PIN_RESET); }
void setCS(uint8_t state) { if (state) PORTB |= (1 << PIN_CS); else PORTB &= ~(1 << PIN_CS); }

// Initialize display (ST7735 example, adjust for your controller)
void displayInit() {
    SPI_INIT(); // Initialize SPI (sets SCK, MOSI, CS as outputs)
    DDRB |= (1 << PIN_A0) | (1 << PIN_RESET); // Set A0, RESET as outputs
    
    // Hardware reset
    setReset(0); _delay_us(10);
    setReset(1); _delay_ms(120);
    
    // Send initialization commands
    setCS(0);
    setA0(0); SPI_SEND(0x11); // Sleep out
    _delay_ms(120);
    
    setA0(0); SPI_SEND(0x3A); // Set color mode
    setA0(1); SPI_SEND(0x05); // 16-bit RGB565
    setA0(0);
    
    setA0(0); SPI_SEND(0x36); // Memory access control (orientation)
    setA0(1); SPI_SEND(0x00); // Normal orientation
    setA0(0);
    
    setA0(0); SPI_SEND(0x29); // Display on
    setCS(1);
}

// Set address window for 128x128
void setAddressWindow(uint8_t x0, uint8_t y0, uint8_t x1, uint8_t y1) {
    setCS(0); setA0(0);
    SPI_SEND(0x2A); // Column address set
    setA0(1); SPI_SEND(0x00); SPI_SEND(x0); SPI_SEND(0x00); SPI_SEND(x1);
    setA0(0);
    SPI_SEND(0x2B); // Row address set
    setA0(1); SPI_SEND(0x00); SPI_SEND(y0); SPI_SEND(0x00); SPI_SEND(y1);
    setA0(0);
    SPI_SEND(0x2C); // Memory write
    setA0(1);
}

// Fill screen with white
void fillScreenWhite() {
    setAddressWindow(0, 0, 127, 127); // Full 128x128 grid
    setCS(0);
    for (uint16_t i = 0; i < 128 * 128; i++) {
        SPI_SEND(COLOR_WHITE >> 8); // High byte
        SPI_SEND(COLOR_WHITE & 0xFF); // Low byte
    }
    setCS(1);
}


#endif /* SPIAVR_H */
