#include <stdio.h>
#include <math.h>

int main()
{
    double count = 1;
    for (double i = 3; i < 1000000; i += 2)
    {
        for (double j = 3; j * j <= i; j += 2)
        {
            if (fmod(i, j) == 0)
            {
                goto end;
            }
        }
        count += 1;
    end:
        continue;
    }
    printf("%.0f\n", count);
}