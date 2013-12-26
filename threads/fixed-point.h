/*
In the table, x and y are fixed-point numbers, n is an integer, 
fixed-point numbers are in signed p.q format where p + q = 31, and f is 1 << q: 
*/
//f is 1 << q: 
#define f 16384

/*Convert n to fixed point: n * f */
#define inte_to_fix(n) (n * f)

/*Convert x to integer (rounding toward zero): x / f  */
#define fix_to_intezero(x) (x / f)

/*Convert x to integer (rounding to nearest): 
(x + f / 2) / f if x >= 0, (x - f / 2) / f if x <= 0.  */
#define fix_to_intenearest(x) (x >= 0 ? (x + f / 2) / f : (x - f / 2) / f)

/*Add x and y: x + y  */
#define addfix(x,y) (x + y)

/*Subtract y from x: x - y  */
#define subfix(x,y) (x - y)

/*Add x and n: x + n * f  */
#define addfixinte(x,n) (x + n * f)

/*Subtract n from x: x - n * f  */
#define subfixinte(x,n) (x - n * f)

/*Multiply x by y: ((int64_t) x) * y / f */
#define Multifix(x,y) (((int64_t) x) * y / f )

/*Multiply x by n: x * n  */
#define Multifixinte(x,n) (x * n)

/*Divide x by y: ((int64_t) x) * f / y  */
#define Dividefix(x,y) (((int64_t) x) * f / y )

/*Divide x by n: x / n  */
#define Dividefixinte(x,n) (x / n)
