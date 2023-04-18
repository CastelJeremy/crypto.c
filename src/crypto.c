#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <getopt.h>
#include <curl/curl.h>
#include <cjson/cJSON.h>

#define COINS_MAX_SIZE 32
#define BODY_MAX_SIZE 128

typedef struct coin {
    char* id;
    double amount;
    double price;
    double value;
} Coin;

static inline size_t
write_callback (char* ptr, size_t size, size_t nmemb, char* buffer_in)
{
    if (nmemb > BODY_MAX_SIZE)
    {
        return 0;
    }

    for (size_t i = 0; i < nmemb; i++)
    {
        buffer_in[i] = ptr[i];
    }

    return nmemb;
}

static inline char*
get_url (const char* coin_id, const char* currency)
{
    char* main_path = "https://api.coingecko.com/api/v3/simple/price";
    char* url_arg_1 = "?ids=";
    char* url_arg_2 = "&vs_currencies=";

    size_t url_size = strlen (main_path) + strlen (url_arg_1) + strlen (coin_id) + strlen (url_arg_2) + strlen (currency);
    char* url = calloc(url_size + 1, sizeof (char));

    for (size_t i = 0; i < strlen (main_path); i++)
    {
        url[i] = main_path[i];
    }

    for (size_t i = 0; i < strlen (url_arg_1); i++)
    {
        url[strlen (url)] = url_arg_1[i];
    }

    for (size_t i = 0; i < strlen (coin_id); i++)
    {
        url[strlen (url)] = coin_id[i];
    }

    for (size_t i = 0; i < strlen (url_arg_2); i++)
    {
        url[strlen (url)] = url_arg_2[i];
    }

    for (size_t i = 0; i < strlen (currency); i++)
    {
        url[strlen (url)] = currency[i];
    }

    return url;
}

static inline double
coin_price (const char* coin_id, const char* currency)
{
    char* body = calloc (BODY_MAX_SIZE + 1, sizeof (char));

    char* url = get_url (coin_id, currency);

    CURL* curl = curl_easy_init ();
    curl_easy_setopt (curl, CURLOPT_URL, url);
    curl_easy_setopt (curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt (curl, CURLOPT_WRITEDATA, body);
    CURLcode res = curl_easy_perform (curl);

    double price = 0;

    if (res != CURLE_OK)
    {
        return price;
    }
    else
    {
        cJSON* cjson = cJSON_Parse (body);

        if (!cJSON_IsInvalid (cjson))
        {
            cJSON* child = cjson->child;

            if (!cJSON_IsInvalid (child))
            {
                cJSON* value = cJSON_GetObjectItemCaseSensitive (child, currency);
            
                if (cJSON_IsNumber (value))
                {
                    price = value->valuedouble;
                }
            }
        }

        cJSON_Delete (cjson);
    }

    curl_easy_cleanup (curl);

    return price;
}

static inline int
read_coins (const char* filename, Coin* coins, size_t* coins_size)
{
    FILE* f;
    f = fopen (filename, "r");

    if (f == NULL)
    {
        return -1;
    }

    const size_t NAME_MAX_LENGTH = 32;
    const size_t AMOUNT_MAX_LENGTH = 32;

    char* name = calloc (NAME_MAX_LENGTH, sizeof (char));
    char* amount_str = calloc (AMOUNT_MAX_LENGTH, sizeof (char));

    int reached_eon = 0;

    int c;
    while ((c = fgetc (f)) != EOF)
    {
        if (c == '\n')
        {
            if ((* coins_size) >= COINS_MAX_SIZE)
            {
                return -3;
            }

            if (reached_eon == 0)
            {
                return -2;
            }

            Coin c;
            c.id = malloc (NAME_MAX_LENGTH * sizeof (char));
            c.amount = strtod (amount_str, NULL);
            c.price = 0;
            c.value = 0;

            for (size_t i = 0; i < NAME_MAX_LENGTH; i++) c.id[i] = name[i];

            coins[(* coins_size)++] = c;

            reached_eon = 0;
            for (size_t i = 0; i < NAME_MAX_LENGTH; i++) name[i] = '\0';
            for (size_t i = 0; i < AMOUNT_MAX_LENGTH; i++) amount_str[i] = '\0';
        }
        else if (c == ' ')
        {
            reached_eon = 1;
        }
        else if (reached_eon == 0 && strlen (name) < NAME_MAX_LENGTH)
        {
            name[strlen (name)] = c;
        }
        else if (reached_eon == 1 && strlen (amount_str) < AMOUNT_MAX_LENGTH)
        {
            amount_str[strlen (amount_str)] = c;
        }
        else
        {
            return -2;
        }
    }

    return 0;
}

static inline int
print_coins (const char* currency, const Coin* coins, const size_t coins_size)
{
    int name_top_length = 0;
    for (size_t i = 0; i < coins_size; i++)
    {
        if (strlen (coins[i].id) > name_top_length)
        {
            name_top_length = strlen (coins[i].id);
        }
    }

    double total = 0;
    printf ("%-*s %14s %14s %14s\n", name_top_length + 1, "", "Amount", "Price", "Value");

    for (size_t i = 0; i < coins_size; i++)
    {
        printf ("%-*s %14f %14f %14f %s\n", name_top_length + 1, coins[i].id, coins[i].amount, coins[i].price, coins[i].value, currency);
    
        total += coins[i].value;
    }

    printf ("%-*s %-14s %14s %14f %s\n", name_top_length + 1, "", "", "Total", total, currency);

    return 0;
}

static inline int
output_csv (const char* filename, const char* currency, const Coin* coins, const size_t coins_size)
{
    FILE* f;
    f = fopen (filename, "w");

    if (f == NULL)
    {
        return -1;
    }

    fprintf (f, "%s,%s,%s,%s\n", "name", "amount", "price", "value");

    for (size_t i = 0; i < coins_size; i++)
    {
        fprintf (f, "%s,%f,%f,%f\n", coins[i].id, coins[i].amount, coins[i].price, coins[i].value);
    }

    return 0;
}

static inline void
print_try_help ()
{
    fprintf (stderr, "Try 'crypto --help' for more information.\n");
}

static inline void
usage (int status)
{
    if (status != EXIT_SUCCESS)
    {
        print_try_help ();
    }
    else
    {
        printf ("\
Usage: crypto [options...]\n\
 -c, --currency <code>   Choose the currency to compare, using the\n\
                         ISO 4217 code\n\
 -i, --input <filename>  Data file containing the gecko id and the\n\
                         amount of cryptos in your portfolio\n\
 -o, --output <filename> Write a csv output in the given file\n\
 -h, --help              Show this help message and quit\n\
 -v, --version           Show version number and quit\n");
    }

    exit (status);
}

static inline void
version ()
{
    printf ("1.0.0\n");
    exit (EXIT_SUCCESS);
}

int
main (int argc, char* argv[])
{
    struct option options[] = {
        { "currency", required_argument, NULL, 'c' },
        { "input",    required_argument, NULL, 'i' },
        { "output",   required_argument, NULL, 'o' },
        { "help",     no_argument,       NULL, 'h' },
        { "version",  no_argument,       NULL, 'v' },
    };

    char* currency = "usd";
    char* input = getenv ("CRYPTO_PORTFOLIO_FILE");
    char* output = NULL;

    char opt;
    while ((opt = getopt_long (argc, argv, "c:i:o:hv", options, NULL)) != -1)
    {
        switch (opt)
        {
            case 'c':
                currency = optarg;
                break;
            case 'i':
                input = optarg;
                break;
            case 'o':
                output = optarg;
                break;
            case 'h':
                usage (EXIT_SUCCESS);
                break;
            case 'v':
                version ();
                break;
            case ':':
            case '?':
                usage (EXIT_FAILURE);
                break;
            default:
                break;
        }
    }

    Coin coins[COINS_MAX_SIZE];
    size_t coins_size = 0;

    int file_status = read_coins (input, coins, &coins_size);

    if (file_status == -1)
    {
        printf ("crypto: cannot stat '%s': failed to open\n", input);
        exit (EXIT_FAILURE);
    }
    else if (file_status == -2)
    {
        printf ("crypto: cannot read '%s': invalid format\n", input);
        exit (EXIT_FAILURE);
    }
    else if (file_status == -3)
    {
        printf ("crypto: cannot read '%s': too many lines\n", input);
        exit (EXIT_FAILURE);
    }

    for (size_t i = 0; i < coins_size; i++)
    {
        coins[i].price = coin_price(coins[i].id, currency);
        coins[i].value = coins[i].amount * coins[i].price;
    }

    print_coins (currency, coins, coins_size);

    if (output != NULL)
    {
        output_csv (output, currency, coins, coins_size);
    }

    return EXIT_SUCCESS;
}
