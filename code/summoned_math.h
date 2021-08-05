/* date = August 5th 2021 2:16 pm */

#ifndef SUMMONED_MATH_H
#define SUMMONED_MATH_H

typedef union V2
{
    struct
    {
        F32 x, y;
    };
    F32 e[2];
} V2;

V2
V2F32(F32 x, F32 y)
{
    V2 result;
    
    result.x = x;
    result.y = y;
    
    return result;
}

V2
ScaleV2(F32 a, V2 b)
{
    V2 result;
    
    result.x = a*b.x;
    result.y = a*b.y;
    
    return result;
}

V2
OppositeV2(V2 a)
{
    V2 result;
    
    result.x = -a.x;
    result.y = -a.y;
    
    return result;
}

V2
AddV2(V2 a, V2 b)
{
    V2 result;
    
    result.x = a.x + b.x;
    result.y = a.y + b.y;
    
    return result;
}

V2
SubtractV2(V2 a, V2 b)
{
    V2 result;
    
    result.x = a.x - b.x;
    result.y = a.y - b.y;
    
    return result;
}

#endif //SUMMONED_MATH_H