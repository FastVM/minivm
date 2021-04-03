def fib(n):
    return n if n < 2 else fib(n-2) + fib(n-1)

print(fib(35))