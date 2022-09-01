
function pow2(n) {
    if (n == 0) {
        return 1;
    } else {
        return pow2(n-1) * 2;
    }
}

function bottom_up_tree(item, depth) {
    if (depth > 0) {
        let i = item + item;
        let next_depth = depth - 1;
        let left = bottom_up_tree(i-1, next_depth);
        let right = bottom_up_tree(i, next_depth);
        return [item, left, right];
    } else {
        return [item];
    }
}

function item_check(tree) {
    if (tree.length == 3) {
        return tree[0] + (item_check(tree[1]) - item_check(tree[2]));
    } else {
        return tree[0];
    }
}

function max(a, b) {
    if (a > b) {
        return a;
    } else {
        return b;
    }
}

function main(n) {
    let mindepth = 4;
    let maxdepth = max(mindepth + 2, n);
    
    console.log(item_check(bottom_up_tree(0, maxdepth + 1)));

    let long_lived_tree = bottom_up_tree(0, maxdepth);

    let depth = mindepth;
    while (depth <= maxdepth) {
        let iterations = pow2(maxdepth - depth + mindepth);
        let check = 0;
        let index = 0;
        while (index < iterations) {
            check = check + item_check(bottom_up_tree(1, depth)) + item_check(bottom_up_tree(0-1, depth));
            index = index + 1;
        }
        console.log(check);
        depth = depth + 2;
    }
    
    console.log(item_check(long_lived_tree));
}

main(16);
