import gmpy

def fib(a, b, n):
    a = gmpy.mpz(a)
    b = gmpy.mpz(b)
    for _ in range(n):
        a, b = b, a + b
    return a

def first(n):
    while 9 < n:
        n //= 10
    return n

def main():
    v = 0
    for i in range(1, 10000):
        v += first(fib(0, 1, i))
    print(v)

if __name__ == '__main__':
    main()