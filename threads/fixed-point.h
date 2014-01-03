#ifndef THREADS_FIXED_POINT_H
#define THREADS_FIXED_POINT_H

#define FRACTIONAL_BITS 14						   //����32λ������˵��λģʽ	
#define FIXED_POINT_F ( 1 << FRACTIONAL_BITS )	   //�Ӷ��������fֵ
#include <stdint.h>

/* x and y are fixed point, and n is an integer */
int fp_integer_to_fixed(int n);					   //ת��Ϊfixed
int fp_fixed_to_integer_zero(int x);			   //ת��Ϊ����
int fp_fixed_to_integer_nearest(int x);			   //ת��Ϊ����
int fp_add(int x, int y);						   //fp��
int fp_subtract(int x, int y);					   //fp��
int fp_add_integer(int x, int n);				   //fp������
int fp_subtract_integer(int x, int n);			   //fp������
int fp_multiply(int x, int y);					   //fp��
int fp_multiply_integer(int x, int n);			   //fp������
int fp_divide(int x, int y);					   //fp��
int fp_divide_integer(int x, int n);			   //fp������


#endif /* threads/fixed-point.h */
