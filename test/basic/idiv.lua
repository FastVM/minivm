
assert(1 / 2 == 0.5)

assert(1 // 2 == 0)

print(tostring(10000000 / 0.3) .. ' == ')
print(tostring(10000000 // 0.3) .. ' == ')

print(tostring(1000000000 // 0.3) .. ' == 3333333333')
print(tostring(1000000000 / 0.3) .. ' == 3333333333.333333')
