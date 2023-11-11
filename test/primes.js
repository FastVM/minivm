
let count = 1;
let max = 1000000;
let pprime = 3;
while (pprime < max) {
    let check = 3;
    let isprime = 1;
    while (check * check <= pprime) {
        if (pprime % check === 0) {
            isprime = 0;
        }
        check += 2;
    }
    count += isprime;
    pprime += 2;
}

console.log(count);
