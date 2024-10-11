#define F (1<<14)
#define INT_MAX ((1<<32) - 1)
#define INT_MIN (-(1<<31))


int convert_int_to_fp(int n);
int convert_fp_to_int_toward_zero(int x);
int convert_fp_to_int_round(int x);
int add_fps(int x, int y);
int sub_fps(int x, int y);
int add_fp_and_int(int x, int n);
int sub_fp_and_int(int x, int n);
int mul_fps(int x, int y);
int mul_fp_and_int(int x, int n);
int div_fps(int x, int y);
int div_fp_and_int(int x, int n);


int convert_int_to_fp(int n)
{
  return n * F;
}

int convert_fp_to_int_toward_zero(int x)
{
  return x / F;
}

int convert_fp_to_int_round(int x)
{
  if (x >= 0)
      return (x + F/2)/F;
  else
      return (x - F/2)/F;
}

int add_fps(int x, int y)
{
  return x + y;
}

int sub_fps(int x, int y)
{
  return x - y;
}

int add_fp_and_int(int x, int n)
{
  return x+n*F;
}

int sub_fp_and_int(int x, int n)
{
  return x - n*F;
}

int mul_fps(int x, int y)
{
  return ((int64_t) x) * y / F;
}

int mul_fp_and_int(int x, int n)
{
  return x * n;
}

int div_fps(int x, int y)
{
  return ((int64_t) x) * F / y;
}

int div_fp_and_int(int x, int n)
{
  return x / n;
}