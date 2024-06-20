#ifndef FLOAT_POINT
#define FLOAT_POINT

#define Q 14
#define F (1 << Q)

// vai retornar apenas 2^p, pois tudo é na base 2
typedef int float_type;
// Lembrar que nao funciona INT_FLOAT(59/60), apenas se INT_FLOAT(59.0/60)
// converter integer to float_type
#define INT_FLOAT(n) ((float_type)(n * F))
// soma/sub entre dois float type
#define FLOAT_ADD(x, y) (x + y)
#define FLOAT_SUB(x, y) (x - y)
// multi/divide entre dois float type
#define FLOAT_MUL(x, y) ((float_type)(((int64_t) x) * (y / F)))
#define FLOAT_DIV(x, y) ((float_type)(((int64_t) x) * (F / y)))
// soma/sub entre float type(x) e integer(n)
#define FLOAT_INT_ADD(x, n) (x + (n * F))
#define FLOAT_INT_SUB(x, n) (x - (n * F))
// multi/div entre float type(x) e integer(n)
#define FLOAT_INT_MUL(x, n) (x * n)
#define FLOAT_INT_DIV(x, n) (x / n)

// converte x to integer, arredondando para zero
// nao sei se esse aqui funciona
// talvez trocar por isso, na doc ta assim
#define FLOAT_INT_ZERO(x) ((int) (x / F)
// round de float
#define FLOAT_INT_ROUND(x) (x >= 0) ? (x + (F/2)) : (x - (F/2))
#endif
