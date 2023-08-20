def bottom_up_tree(item, depth):
    if depth > 0:
        i = item + item
        depth -= 1
        left = bottom_up_tree(i-1, depth)
        right = bottom_up_tree(i, depth)
        return [item, left, right]
    else:
        return [item]

def item_check(tree):
    if len(tree) != 1:
        return tree[0] + item_check(tree[1]) - item_check(tree[2])
    else:
        return tree[0]

def main(n):
    mindepth = 4
    maxdepth = mindepth + 2
    if maxdepth < n:
        maxdepth = n
    
    print(item_check(bottom_up_tree(0, maxdepth + 1)))

    long_lived_tree = bottom_up_tree(0, maxdepth)

    for depth in range(mindepth, maxdepth+2, 2):
        iterations = 2 ** (maxdepth - depth + mindepth)
        check = 0
        for _ in range(iterations):
            check += item_check(bottom_up_tree(0, depth)) + item_check(bottom_up_tree(, depth))
        print(check)
    
    print(item_check(long_lived_tree))

if __name__ == '__main__':
    main(16)
