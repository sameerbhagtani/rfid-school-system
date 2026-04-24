#include "display.h"
#include "config.h"

#include <Wire.h>
#include <LiquidCrystal_I2C.h>

static LiquidCrystal_I2C lcd(0x27, LCD_COLS, LCD_ROWS);

void initLCD()
{
    Wire.begin();
    lcd.init();
    lcd.backlight();
}

void printToLCD(const String &msg)
{
    int row = 0;
    int col = 0;

    lcd.clear();

    for (unsigned int i = 0; i < msg.length(); i++)
    {
        if (msg[i] == '\n')
        {
            row++;
            col = 0;
            continue;
        }

        if (col == LCD_COLS)
        {
            row++;
            col = 0;
        }

        if (row >= LCD_ROWS)
            break;

        lcd.setCursor(col, row);
        lcd.print(msg[i]);
        col++;
    }
}
