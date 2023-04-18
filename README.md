# crypto.c

A CLI to track the value of your crypto portfolio by extracting data from the coingecko API.

## Usage

```
$ crypto -i portfolio.txt
                  Amount          Price          Value
bitcoin         3.904000   29442.000000  114941.568000 usd
ethereum        8.528000    2069.180000   17645.967040 usd
monero         81.648000     161.710000   13203.298080 usd
nano        10294.001000       0.925382    9525.883233 usd
                                  Total  155316.716353 usd
```

```
$ crypto -h
Usage: crypto [options...]
 -c, --currency <code>   Choose the currency to compare, using the
                         ISO 4217 code
 -i, --input <filename>  Data file containing the gecko id and the
                         amount of cryptos in your portfolio
 -o, --output <filename> Write a csv output in the given file
 -h, --help              Show this help message and quit
 -v, --version           Show version number and quit
```

The default currency is `usd` but you can replace it with any [ISO 4217 code](https://en.wikipedia.org/wiki/ISO_4217#Active_codes_(List_One)) if it is available on the API.

### Input File

This is an example for the input file content :

```
bitcoin 3.904
ethereum 8.528
monero 81.648
nano 10294.001
```

You can either pass it with `--input` option or you can set the `CRYPTO_PORTFOLIO_FILE` environment variable.

## Build

First you need to build and install [cJSON](https://github.com/DaveGamble/cJSON).

Then you can run the following command to build the project:

```
gcc -Wall -O3 src/crypto.c -l cjson -l curl -o bin/crypto
```
