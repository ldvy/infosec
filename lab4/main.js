const forge = require('node-forge');
const BigNumber = require('bignumber.js');

class RSA {
    static genaratePrimePair(bits) {
        const options = {
            algorithm: {
                name: 'PRIMEINC',
                workers: -1,
            },
        };
        return new Promise((res, rej) => {
            forge.prime.generateProbablePrime(bits, options, function (err, num) {
                if (err) return rej(err);
                res(new BigNumber(num.toString()));
            });
        });
    }
}

(async () => {
    try {
        const prime_a = await RSA.genaratePrimePair(1024);
        const prime_b = await RSA.genaratePrimePair(1024);

        console.dir({
            a: prime_a.toString(),
            b: prime_b.toString(),
        });

        //Operations example
        console.dir({
            sum: prime_a.plus(prime_b).toString(),
            dif: prime_a.minus(prime_b).toString(),
            mod: prime_a.multipliedBy(prime_b).toString(),
            pow: prime_b.exponentiatedBy(2).toString(),
        });
    } catch (err) {
        console.dir(
            {
                message: 'main error',
                error: err,
            },
            {depth: 100},
        );
    }
})();