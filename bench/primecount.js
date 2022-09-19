function isprime(num) {
    for (let check=3; check * check <= num; check += 2) {
        if (num % check == 0) {
            return 0;
        }
    } 
    return 1;
}

function primes(upto) {
    let count = 2;
    for (let val = 5; val < upto; val += 2) {
        count += isprime(val);
    }
    console.log(count);
}

primes(1000000);
