#include <stdio.h>
#include <math.h>

int main()
{
    int count = 2;
    for (int i = 3; i < 10000000; i += 2)
    {
        for (int j = 3; j * j <= i; j += 2)
        {
            if (i % j == 0)
            {
                goto end;
            }
        }
        count += 1;
    end:
        continue;
    }
    printf("%i\n", count);
}