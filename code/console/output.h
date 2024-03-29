#ifndef CONSOLE_H
#define CONSOLE_H

#include "color.h"

static Color defaultColor;
static Color activeColor;

void Console_SetDefaultColor(Color color);
void Console_SetActiveColor(Color color);
void Console_ResetColor();
void Console_PrintColored(char* message, Color color, ...);
void Console_Print(char* message, ...);
void Console_Clear();

#endif