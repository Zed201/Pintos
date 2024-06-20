#ifndef FLOAT_POINT
#define FLOAT_POINT

#define Q 14
#define F (1 << Q)

// vai retornar apenas 2^p, pois tudo é na base 2
typedef int float_type;
// converter integer to float_type
#define INT_FLOAT(n) ((float_type)(n << Q))
// soma/sub entre dois float type
#define FLOAT_ADD(x, y) (x + y)
#define FLOAT_SUB(x, y) (x - y)
// multi/divide entre dois float type
#define FLOAT_MUL(x, y) ((float_type)((((int64_t) x * y) / F)))
#define FLOAT_DIV(x, y) ((float_type)((((int64_t) x * F) / y)))
// soma/sub entre float type(x) e integer(n)
#define FLOAT_INT_ADD(x, n) (x + (n << Q))
#define FLOAT_INT_SUB(x, n) (x - (n << Q))
// multi/div entre float type(x) e integer(n)
#define FLOAT_INT_MUL(x, n) (x * n)
#define FLOAT_INT_DIV(x, n) (x / n)

// basicamente retorna a parte inteira
#define FLOAT_INT_ZERO(x) (int) (x >> Q)
// round de float
#define FLOAT_INT_ROUND(x) (x >= 0) ? (x + (F/2)) : (x - (F/2))
#endif
