#include "threads/fixed-point.h"


int fp_integer_to_fixed(int n)
{
    return n * FIXED_POINT_F;
}


int fp_fixed_to_integer_zero(int x)
{
    return x / FIXED_POINT_F;
}


int fp_fixed_to_integer_nearest(int x)
{
    if(x >= 0)
    {
        return (x + FIXED_POINT_F/2) / FIXED_POINT_F;
    }
    return (x - FIXED_POINT_F/2) / FIXED_POINT_F;
}




int fp_add(int x, int y)
{
    return x + y;
}


int fp_subtract(int x, int y)
{
    return x - y;
}


int fp_add_integer(int x, int n)
{
    return x + n * FIXED_POINT_F;
}


int fp_subtract_integer(int x, int n)
{
    return x - n * FIXED_POINT_F;
}


int fp_multiply(int x, int y)
{
    return ((int64_t)x) * y / FIXED_POINT_F;
}


int fp_multiply_integer(int x, int n)
{
    return x * n;
}


int fp_divide(int x, int y)
{
    return ((int64_t)x) * FIXED_POINT_F / y;
}


int fp_divide_integer(int x, int n)
{
    return x / n;
}
