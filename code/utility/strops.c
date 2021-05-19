#include "strops.h"

int GetLength(const char* src)
{
    int length = 0;
    while(*src != '\0')
    {
        length++;
        src++;
    }
    return length;
}

char* SkipSpaces(const char* src)
{
    while(*src == ' ')
    {
        src++;
    }
    return (char*)src;
}

bool IsUpper(char letter)
{
    return letter >= 'A' && letter <= 'Z';
}

bool IsLetter(char letter)
{
    return letter >= 'A' && letter <= 'Z'
        || letter >= 'a' && letter <= 'z';
}

char* Copy(char* src)
{
    int length = GetLength(src);
    char *new = (char *)malloc(sizeof(char)*(length+1));
    for(int i=0; i<length+1; i++)
    {
        new[i] = src[i];
    }
    return new;
}

void Capitalise(char* src)
{
    src = SkipSpaces(src);
    if(IsLetter(*src) && !IsUpper(*src))
    {
        *src -= 32;
    }
}

bool CompareCharInsenitive(char a, char b)
{
    // ASCII:  A(65) - a(97) = 32
    return  a == b    ? 1 :
            a+32 == b ? 1 :
            a == b+32 ? 1 : 0;
}