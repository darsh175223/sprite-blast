#ifndef LCD_H_
#define LCD_H_

#include <avr/io.h>
#include <util/delay.h>

#define DATA_BUS	PORTD
#define CTL_BUS		PORTD
#define DATA_DDR	DDRD
#define CTL_DDR		DDRD
#define LCD_D4			4
#define LCD_D5			5
#define LCD_D6			6
#define LCD_D7			7
#define LCD_EN			3			
#define	LCD_RS			2

#define LCD_CMD_CLEAR_DISPLAY	          0x01
#define LCD_CMD_CURSOR_HOME		          0x02

// Display control
#define LCD_CMD_DISPLAY_OFF                0x08
#define LCD_CMD_DISPLAY_NO_CURSOR          0x0c
#define LCD_CMD_DISPLAY_CURSOR_NO_BLINK    0x0E
#define LCD_CMD_DISPLAY_CURSOR_BLINK       0x0F

// Function set
#define LCD_CMD_4BIT_2ROW_5X7              0x28
#define LCD_CMD_8BIT_2ROW_5X7              0x38

void lcd_send_command (uint8_t command)
{
	DATA_BUS=(command&0b11110000); 
	CTL_BUS &=~(1<<LCD_RS);
	CTL_BUS |=(1<<LCD_EN);
	_delay_ms(1);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(1);
	DATA_BUS=((command&0b00001111)<<4);
	CTL_BUS |=(1<<LCD_EN);
	_delay_ms(1);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(1);
}

void lcd_init(void)
{	
	
	DATA_DDR = (1<<LCD_D7) | (1<<LCD_D6) | (1<<LCD_D5)| (1<<LCD_D4);
	CTL_DDR |= (1<<LCD_EN)|(1<<LCD_RS);

	
	DATA_BUS = (0<<LCD_D7)|(0<<LCD_D6)|(1<<LCD_D5)|(0<<LCD_D4);
	CTL_BUS|= (1<<LCD_EN)|(0<<LCD_RS);
	_delay_ms(1);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(1);
	
	lcd_send_command(LCD_CMD_4BIT_2ROW_5X7);
	_delay_ms(1);
	lcd_send_command(LCD_CMD_DISPLAY_CURSOR_BLINK);
	_delay_ms(1);
	lcd_send_command(0x80);
    _delay_ms(10);
	
}


void lcd_write_character(char character)
{
	
	DATA_BUS=(DATA_BUS & 0x0F) | (character & 0b11110000);
	CTL_BUS|=(1<<LCD_RS);
	CTL_BUS |=(1<<LCD_EN);
	_delay_ms(2);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(2);
	DATA_BUS= (DATA_BUS & 0x0F) | ((character & 0b00001111)<<4);
	CTL_BUS |=(1<<LCD_EN);
	_delay_ms(2);
	CTL_BUS &=~(1<<LCD_EN);
	_delay_ms(2);
	
}

void lcd_write_str(char* str)
{
	int i=0;
	while(str[i]!='\0')
	{
		lcd_write_character(str[i]);
		i++;
	}
}

void lcd_clear()
{
	lcd_send_command(LCD_CMD_CLEAR_DISPLAY);
	_delay_ms(5);
}
void lcd_goto_xy (uint8_t line,uint8_t pos)				//line = 0 or 1
{
	lcd_send_command((0x80|(line<<6))+pos);
	_delay_us (50);
}
// --- custom char bitmaps (only lower 5 bits used) ---

// 5×8 “bullseye”: outer ring + inner ring + center dot
static const uint8_t bullseye_char_pattern[8] = {
    0b00000,  // Row 0: (all off for top padding)
    0b01010,  // Row 1:  . # . # . 
    0b11111,  // Row 2:  # # # # #
    0b11111,  // Row 3:  # # # # #
    0b11111,  // Row 4:  # # # # #
    0b01110,  // Row 5:  . # # # . 
    0b00100,  // Row 6:  . . # . . 
    0b00000   // Row 7: (all off for bottom padding)
};

// 5×8 upward-pointing triangle
static const uint8_t triangle_char_pattern[8] = {
    0b00100,  //   █  
    0b01110,  //  ███ 
    0b11111,  // █████
    0b11111,  // █████
    0b11111,  // █████
    0b00000,  //   █  
    0b00000,  //   █  
    0b00000   //   █  
};

// 5×8 diamond (perfectly centered)
static const uint8_t diamond_char_pattern[8] = {
    0b00100,  //   █  
    0b01010,  //  █ █ 
    0b10001,  // █   █
    0b10001,  // █   █
    0b10001,  // █   █
    0b01010,  //  █ █ 
    0b00100,  //   █  
    0b00000   //      
};

// 5×8 lunar crescent
static const uint8_t crescent_char_pattern[8] = {
    0b01110,  //  ███ 
    0b11001,  // ██  █
    0b10000,  // █    
    0b10000,  // █    
    0b10000,  // █    
    0b11001,  // ██  █
    0b01110,  //  ███ 
    0b00000   //      
};


/** write one pattern into CGRAM slot [0..7] **/
void lcd_create_custom_char(uint8_t loc, const uint8_t *pat) {
    if (loc > 7) return;
    // CGRAM address = 0x40 + loc*8
    lcd_send_command(0x40 | (loc<<3));
    _delay_us(50);
    for (uint8_t i = 0; i < 8; i++) {
        lcd_write_character(pat[i]);
        _delay_us(50);
    }
}

/** upload all four, then return to DDRAM */
void lcd_setup_custom_chars(void) {
    lcd_create_custom_char(0, bullseye_char_pattern);
    lcd_create_custom_char(1, triangle_char_pattern);
    lcd_create_custom_char(2, diamond_char_pattern);
    lcd_create_custom_char(3, crescent_char_pattern);
    // go back into DDRAM/text mode
}

/** display one of them by writing code 0..3 **/
static inline void callSpecialCharacter(int num) {
    if (num >= 0 && num <= 3) {
        lcd_write_character((char)num);
    }
}

#endif /* LCD_H_ */