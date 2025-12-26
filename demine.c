#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <tice.h>
// graphics
#include <graphx.h>
// random
#include <sys/util.h>
// input
#include <ti/getcsc.h>

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>





#define COLOR1 16
#define COLOR2 6
#define COLOR3 224
#define COLOR4 8
#define COLOR5 128
#define COLOR6 21
#define COLOR7 216
#define COLOR8 222

#define OFFSETX 15
#define OFFSETY 21

bool partial_redraw;

uint32_t lastTicks = 0;
float deltaTime = 0.0f;

// -- Data structure --
// bit 0-5 : Number
// bit 6 : discovered ?
// bit 7 : bomb ?
// bit 8 : flag ?
uint8_t data[81] = {0};

uint8_t selection = 40;

float gameTimer = 0;
bool timer = false;
bool cheat = false;

// Implement us!
void begin();
int end();
bool step();
void draw();
void drawGrid();
void generateData();
void drawElements();
void floodFill(uint8_t x, uint8_t y);
void drawStaticHUD();
void drawTimerHUD();
void drawCursor();
void drawAll();





int main() {
    begin(); // No rendering allowed!
    gfx_Begin();
    gfx_SetDrawBuffer(); // Draw to the buffer to avoid rendering artifacts

    while (step()) { // No rendering allowed in step!
        if (partial_redraw) // Only redraw part of the previous frame?
            gfx_BlitScreen(); // Copy previous frame as a base for this frame
        draw(); // As little non-rendering logic as possible
        gfx_SwapDraw(); // Queue the buffered frame to be displayed
    }

    gfx_End();
    return(end());
}


void begin()
{
    // Init random generator
    srand(rtc_Time());

    // Init timer
    timer_Control = TIMER2_DISABLE;
    timer_2_ReloadValue = 0;
    timer_2_Counter = 0;
    timer_Control = TIMER2_ENABLE | TIMER2_32K | TIMER2_UP;

    generateData();

    return;
}

void generateData()
{
    // generate bombs
    uint8_t bombs = 0;
    while (bombs < 10)
    {
        uint8_t i = randInt(0, 81);

        if(!(data[i] & (1 << 1)))
        {
            data[i] |= (1 << 1);
            bombs += 1;
        }
    }

    // generate numbers
    for(uint8_t x = 0; x < 9; x++)
    {
        for(uint8_t y = 0; y < 9; y++)
        {

            uint8_t bombsCount = 0;
            uint8_t i = y * 9 + x;
            // for all souroundings cases
            for(int8_t px = -1; px <= 1; px++)
            {
                for(int8_t py = -1; py <= 1; py++)
                {
                    if((y + py) >= 0 && (y + py) < 9 && (x + px) >= 0 && (x + px) < 9)
                    {
                        int8_t pi = (y + py) * 9 + (x + px);
                        if(data[pi] & (1 << 1))
                        {
                            bombsCount++;
                        }
                    }

                }
            }
            unsigned char mask = ((1 << 4) - 1) << 3;
            data[i] = (data[i] & ~mask) | ((bombsCount << 3) & mask);
        }
    }
}



int end()
{
    return 1;
}



// * -----------------
// *    GAME LOOP
// * -----------------

bool step()
{
    // frame time
    uint32_t now = timer_2_Counter;
    uint32_t delta = now - lastTicks;
    lastTicks = now;

    deltaTime = (float)delta / 32768.0f; // to seconds


    uint8_t x = selection % 9;
    uint8_t y = selection / 9;

    // -- Keyboard Input --

    char key = os_GetCSC();

    switch(key)
    {
    case sk_2nd:
    case sk_Alpha:
        return false;

    case sk_Enter:
        if(data[selection] & (1 << 1)) // If bomb
        {
            timer = false;
            for(uint8_t i = 0 ; i < 81 ; i++)
            {
                data[i] |= (1 << 2);
            }
        }
        else
        {
            floodFill(x, y);
            timer = true;
        }
        break;

    case sk_Add:
        data[selection] ^= (1 << 0);
        break;

    case sk_Del:
        for(int i = 0 ; i < 81 ; i++)
        {
            data[i] = 0;
        }
        x = 4;
        y = 4;
        generateData();
        gameTimer = 0;
        timer = false;
        break;

    case sk_Up:
        y--;
        break;

    case sk_Down:
        y++;
        break;

    case sk_Left:
        x--;
        break;

    case sk_Right:
        x++;
        break;
    case sk_Log:
        cheat = !cheat;
        break;
    default:
        break;
    }
    // Turn selection pos in index
    uint8_t i = y * 9 + x;
    if(i >= 0 && i < 81)
    {
        selection = i;
    }

    return true;
}

void floodFill(uint8_t x, uint8_t y)
{
    uint8_t emptyCount = 0;
    uint8_t index = y * 9 + x;
    data[index] |= (1 << 2);

    if(!(data[index] & 0xF8))
        emptyCount++;

    if(data[index] & 0xF8)
        return;

    uint8_t toCheck[81];
    uint8_t toCheckSize = 1;
    toCheck[0] = (x << 4) | y;
    uint8_t checked[81] = {0};
    uint8_t checkedSize = 0;

    while(toCheckSize > 0)
    {
        uint8_t current = toCheck[--toCheckSize];
        uint8_t cx = (current >> 4) & 0x0F;
        uint8_t cy = current & 0x0F;
        checked[checkedSize++] = current;
        int8_t dx[] = {0, 1, 0, -1, 1, 1, -1, -1};
        int8_t dy[] = {-1, 0, 1, 0, -1, 1, -1, 1};

        for(int i = 0; i < 8; i++)
        {
            int8_t nx = cx + dx[i];
            int8_t ny = cy + dy[i];

            if(nx < 0 || nx >= 9 || ny < 0 || ny >= 9)
                continue;

            uint8_t nindex = ny * 9 + nx;
            uint8_t packed = (nx << 4) | ny;
            int alreadyChecked = 0;

            for(int j = 0; j < checkedSize; j++)
            {
                if(checked[j] == packed)
                {
                    alreadyChecked = 1;
                    break;
                }
            }

            if(alreadyChecked)
                continue;

            data[nindex] |= (1 << 2);

            if(!(data[nindex] & 0xF8))
            {
                emptyCount++;
                toCheck[toCheckSize++] = packed;
            }
        }
    }

    if(emptyCount >= 72)
    {
        timer = false;
        cheat = true;
    }
}

// * -----------------
// *    DRAW LOGIC
// * -----------------


// (319, 239)
//gfx_SetPixel
//gfx_Line
//gfx_Circle
//gfx_Rectangle
//timer_IntStatus

// draw loop
void draw()
{
    // black background
    gfx_ZeroScreen();

    // set green color
    gfx_SetTextFGColor(71);
    gfx_SetColor(71);

    drawGrid();
    drawElements();
    drawCursor();
    drawTimerHUD();

    drawStaticHUD();

    if(timer)
        gameTimer += deltaTime;
    return;
}


void drawTimerHUD()
{
    if(cheat)
    {
        char buffer[4];
        // timer text
        gfx_SetTextFGColor(71);
        gfx_SetTextScale(1, 1);
        gfx_SetTextXY(5, 5);
        sprintf(buffer, "%.1f", 1.0 / deltaTime);
        gfx_PrintString(buffer);
    }

    gfx_SetTextFGColor(71);

    gfx_SetTextScale(3, 3);
    gfx_SetTextXY(240, 35);

    gfx_PrintInt((int)gameTimer, 2);
}

void drawStaticHUD()
{
    gfx_SetTextScale(1, 1);
    gfx_SetTextXY(220, 170);
    gfx_PrintString("Move :  Arrows");
    gfx_SetTextXY(220, 180);
    gfx_PrintString("Dig :        Enter");
    gfx_SetTextXY(220, 190);
    gfx_PrintString("Flag :     +(Add)");
    gfx_SetTextXY(220, 200);
    gfx_PrintString("Restart :   del");
    gfx_SetTextXY(220, 210);
    gfx_PrintString("Quit :          2nd");
    gfx_SetTextXY(290, 230);
    gfx_PrintString("matt");
}

void drawGrid()
{
    for(uint8_t i = 0 ; i <= 9 ; i++)
    {
        uint8_t offset = i * 22;

        gfx_Line((OFFSETX + offset) - 1, OFFSETY, (OFFSETX + offset) - 1, OFFSETY + 198);   // y
        gfx_Line(OFFSETX, OFFSETY + offset, OFFSETX + 196, OFFSETY + offset);   // x
    }
}


void drawCursor()
{
    for(uint8_t x = 0; x < 9; x++)
    {
        for(uint8_t y = 0; y < 9; y++)
        {
            uint16_t px = (x*22) + OFFSETX;
            uint16_t py = (y*22) + OFFSETY;

            uint8_t i = y * 9 + x;

            if(i == selection)
            {
                gfx_SetColor(255);
                gfx_Rectangle(px, py + 1, 21, 21);
            }
        }
    }
}




uint8_t getNumberColor(uint8_t i)
{
    uint8_t result = COLOR1;
    switch (i)
    {
    case 1:
        result = COLOR1;
        break;
    case 2:
        result = COLOR2;
        break;
    case 3:
        result = COLOR3;
        break;
    case 4:
        result = COLOR4;
        break;
    case 5:
        result = COLOR5;
        break;
    case 6:
        result = COLOR6;
        break;
    case 7:
        result = COLOR7;
        break;
    case 8:
        result = COLOR8;
        break;
    }
    return result;
}

void drawElements()
{
    for(uint8_t x = 0; x < 9; x++)
    {
        for(uint8_t y = 0; y < 9; y++)
        {
            uint16_t px = (x*22) + OFFSETX;
            uint16_t py = (y*22) + OFFSETY;

            uint8_t i = y * 9 + x;


            if((!(data[i] & (1 << 2))) && !cheat) // Hidden cases
            {
                gfx_SetColor(3);
                gfx_FillRectangle(px, py + 1, 20, 20);
                if(data[i] & (1 << 0))
                {
                    gfx_SetTextFGColor(224);
                    gfx_SetTextScale(2, 2);
                    gfx_SetTextXY(px + 9, py + 4);
                    gfx_PrintChar(33);
                }

            }
            else if(data[i] & (1 << 1)) // Bombs
            {
                gfx_SetTextFGColor(254);
                gfx_SetTextScale(2, 2);
                gfx_SetTextXY(px + 4, py + 4);
                gfx_PrintChar(64);
            }
            else // Numbers
            {

                unsigned char masque = (1 << 4) - 1;
                uint8_t bombsCount = (data[i] >> 3) & masque;

                if(bombsCount > 0)
                {
                    gfx_SetTextFGColor(getNumberColor(bombsCount));
                    gfx_SetTextScale(2, 2);
                    gfx_SetTextXY(px + 4, py + 4);
                    gfx_PrintUInt(bombsCount, 1);
                }
            }
        }
    }

}


