
int printf(const char *fmt, ...);

int ack(int m, int n) {
    while (m) {
        if (n) {
            n = ack(m, n-1);
        } else {
            n = 1;
        }
        m -= 1;
    }
    return n+1;
}

int main() {
    printf("%i\n", ack(3, 11));
}
