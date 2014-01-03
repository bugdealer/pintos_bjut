#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#define FRACTIONAL_BITS 14						   //对于32位整形来说的位模式	
#define FIXED_POINT_F ( 1 << FRACTIONAL_BITS )	   //从而计算出的f值
#include <stdint.h>

/* x and y are fixed point, and n is an integer */
int fp_integer_to_fixed(int n);					   //转换为fixed
int fp_fixed_to_integer_zero(int x);			   //转换为整型
int fp_fixed_to_integer_nearest(int x);			   //转换为整型
int fp_add(int x, int y);						   //fp加
int fp_subtract(int x, int y);					   //fp减
int fp_add_integer(int x, int n);				   //fp加整型
int fp_subtract_integer(int x, int n);			   //fp减整型
int fp_multiply(int x, int y);					   //fp乘
int fp_multiply_integer(int x, int n);			   //fp乘整型
int fp_divide(int x, int y);					   //fp除
int fp_divide_integer(int x, int n);			   //fp除整型


#endif /* threads/fixed-point.h */
