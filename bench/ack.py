import sys
sys.setrecursionlimit(2**20)

def ackermann(m: int, n: int) -> int:
    while m:
        if n:
            n = ackermann(m, n-1)
        else:
            n = 1
        m -= 1
    return n+1

print(ackermann(3, 11))