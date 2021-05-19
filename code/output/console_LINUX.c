#include "console.h"

void Console_Print(const char* message, Console_Color color)
{
    // Reset:       \u001b[0m
    // Black:       \u001b[30m
    // BrightBlack: \u001b[30;1m

    if(color == COLOR_DEFAULT)
    {
        printf(message);
    }
    else
    {
        bool bright = color >= COLOR_BRIGHT_BLACK;
        int attribute = (color % 8) + 30;
        char * format = bright ? "\x001b[%d;1m%s\u001b[0m" :
                                "\x001b[%dm%s\u001b[0m";
        printf(format, attribute, message);
    }
}