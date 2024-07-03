#ifndef FLOAT_POINT
#define FLOAT_POINT

#define Q 14
#define F (1 << Q)

// Retorne 2^P;
typedef int float_type;
// Converter int para float_type;
#define INT_FLOAT(n) ((float_type)(n << Q))

// Soma entre dois float_type;
#define FLOAT_ADD(x, y) (x + y)
// Subtração entre dois float_type;
#define FLOAT_SUB(x, y) (x - y)
// Multiplicação entre dois float_type;
#define FLOAT_MUL(x, y) ((float_type)((((int64_t) x) * y / F)))
// Divisão entre dois float_type;
#define FLOAT_DIV(x, y) ((float_type)((((int64_t) x) * F / y)))

// Soma entre float_type(x) e int(n);
#define FLOAT_INT_ADD(x, n) (x + (n << Q))
// Subtração entre float_type(x) e int(n);
#define FLOAT_INT_SUB(x, n) (x - (n << Q))
// Multiplicação entre float_type(x) e int(n);
#define FLOAT_INT_MUL(x, n) (x * n)
// Divisão entre float_type(x) e int(n);
#define FLOAT_INT_DIV(x, n) (x / n)

// Retornar a parte inteira;
#define FLOAT_INT_ZERO(x) (int) (x >> Q)
// Arredondar para o inteiro mais próximo;
#define FLOAT_INT_ROUND(x) (x >= 0) ? (x + (F/2))/F : (x - (F/2))/F
#endif
