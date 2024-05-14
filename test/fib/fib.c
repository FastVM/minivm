
int printf(const char *fmt, ...);
int sscanf(const char *str, const char *fmt, ...);

int fib(int n) {
    if (n < 2) {
        return n;
    } else {
        return fib(n-1) + fib(n-2);
    }
}

int main(int argc, char **argv) {
    int n = 35;
    if (argv[1]) {
        sscanf(argv[1], "%i", &n);
    }
    printf("%i\n", fib(n));
    return 0;
}
