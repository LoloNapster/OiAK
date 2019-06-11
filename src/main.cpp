#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits>
#include <random>
#include <iomanip>
#include "Float80.h"

using namespace std;

void printDoubleAsBites(uint64_t input);

typedef union
{
    long double number;
    struct
    {
        uint64_t mantissa : 64;
        uint16_t exponent : 15;
        uint16_t sign : 1;
    }
    parts;
}
float80;

uint16_t sign(uint16_t input)
{
    return input >>= 15;
}

uint16_t exponent(uint16_t input)
{
    return (input <<= 1) >>= 1;
}

unsigned char revertBites(unsigned char input)
{
    unsigned char output = input;

    for(int i = sizeof(input) * 7; i ; --i)
    {
        output <<= 1;
        input >>= 1;
        output |= input & 1;
    }

    return output;
}

uint64_t revertMantissa(uint64_t input)
{
    uint64_t output = 0;

    for (int i = 0; i < 64; i++)
    {
        if (input % 2 != 0)
        {
            output = output ^ 1;
        }

        if (i != 63)
        {
            output <<= 1;
            input >>= 1;
        }
    }

    return output;
}

void add(float80* a, float80* b, float80* result)
{
    uint16_t result_sign;
    uint16_t result_exponent;
    uint64_t result_mantissa;

    uint16_t a_exponent = a->parts.exponent;
    uint16_t b_exponent = b->parts.exponent;
    uint64_t a_mantissa = a->parts.mantissa >> 1;
    uint64_t b_mantissa = b->parts.mantissa >> 1;

    int16_t different_sign = a->parts.sign ^ b->parts.sign;

    //NaN
    if (!(a_exponent ^ 0x7FFF) && a_mantissa)
    {
        result->parts.sign = a->parts.sign;
        result->parts.exponent = a_exponent;
        result->parts.mantissa = a_mantissa;
        return;
    }

    if (!(b_exponent ^ 0x7FFF) && b_mantissa)
    {
        result->parts.sign = b->parts.sign;
        result->parts.exponent = b_exponent;
        result->parts.mantissa = b_mantissa;
    }

    //Nieskoczonosc
    if (!(a_exponent ^ 0x7FFF) && !(b_exponent ^ 0x7FFF))
    {
        if (different_sign)
        {
            result->parts.sign = a->parts.sign;
            result->parts.exponent = 0x7FFF;
            result->parts.mantissa = a_mantissa + 1;
            return;
        }
        else
        {
            result->parts.sign = a->parts.sign;
            result->parts.exponent = a_exponent;
            result->parts.mantissa = a_mantissa;
            return;
        }
    }
    else if (!(a_exponent ^ 0x7FFF))
    {
        result->parts.sign = a->parts.sign;
        result->parts.exponent = a_exponent;
        result->parts.mantissa = a_mantissa;
        return;
    }
    else if (!(b_exponent ^ 0x7FFF))
    {
        result->parts.sign = b->parts.sign;
        result->parts.exponent = b_exponent;
        result->parts.mantissa = b_mantissa;
        return;
    }

    //Suma takich liczb
    if ((a_exponent == b_exponent) && (a_mantissa == b_mantissa))
    {
        if (!different_sign)
        {
            result->parts.sign = a->parts.sign;
            result->parts.exponent = a_exponent + 1;
            result->parts.mantissa = a_mantissa << 1;
            return;
        }
        else
        {
            result->parts.sign = 0;
            result->parts.exponent = 0;
            result->parts.mantissa = 0;
            return;
        }
    }

    int exp_difference = abs(a_exponent - b_exponent);

    bool x_bigger_abs;
    if      (a_exponent > b_exponent) x_bigger_abs = true;
    else if (a_exponent < b_exponent) x_bigger_abs = false;
    else if (a_mantissa > b_mantissa) x_bigger_abs = true;
    else                                x_bigger_abs = false;

    if (!different_sign)
    {
        result_sign = a->parts.sign;

        if (x_bigger_abs) {
            result_mantissa = a_mantissa + (b_mantissa >> exp_difference);
            result_exponent = a_exponent;
        }
        else
        {
            result_mantissa = b_mantissa + (a_mantissa >> exp_difference);
            result_exponent = b_exponent;
        }

        if ((result_mantissa >> 63) == 1)
        {
            result_exponent = result_exponent + 1;
        }
        else
        {
            result_mantissa = result_mantissa << 1;
        }
    }
    else
    {
        if (x_bigger_abs) {
            result_sign = a->parts.sign;
            result_mantissa = a_mantissa - (b_mantissa >> exp_difference);
            result_exponent = a_exponent;
        }
        else {
            result_sign = b->parts.sign;
            result_mantissa = b_mantissa - (a_mantissa >> exp_difference);
            result_exponent = b_exponent;
        }

        if ((result_mantissa >> 63) == 1)
        {
            result_exponent = result_exponent + 1;
        }
        else
        {
            result_mantissa = result_mantissa << 1;
        }
    }

    result->parts.sign = result_sign;
    result->parts.exponent = result_exponent;
    result->parts.mantissa = result_mantissa;
}

void printNumberAsBites(float80 input)
{
    unsigned char arr[sizeof(long double)];
    memcpy(arr, &input, sizeof(input));

    cout << input.parts.sign << " ";

    for (int i = 14; i >= 0; i--)
    {
        cout << ((input.parts.exponent >> i) & 1);
    }

    cout << " ";

    for (int i = 63; i >= 0; i--)
    {
        cout << ((input.parts.mantissa >> i) & 1);
    }

    cout << endl;
}

int main()
{
    long double ldNumber1;
    long double ldNumber2;
    long double ldResult;

    char input1[] = "25165122.1451";
    char input2[] = "-1515001.745";

    sscanf(input1, "%Lf", &ldNumber1);
    sscanf(input2, "%Lf", &ldNumber2);

    float80 number1 = { .number = ldNumber1 };
    float80 number2 = { .number = ldNumber2 };
    float80 result = { .number = 0 };

    float80* number1_p = &number1;
    float80* number2_p = &number2;
    float80* result_p = &result;

    //cout.precision(3);

    cout << ldNumber1 << " + " << ldNumber2 << " = " << (ldNumber1 + ldNumber2) << endl;
    cout << endl;

    cout << number1.parts.sign << " " << number1.parts.exponent << " " << number1.parts.mantissa << endl;
    printNumberAsBites(number1);
    cout << number2.parts.sign << " " << number2.parts.exponent << " " << number2.parts.mantissa << endl;
    printNumberAsBites(number2);

    add(number1_p, number2_p, result_p);

    cout << result.parts.sign << " " << result.parts.exponent << " " << result.parts.mantissa << endl;
    printNumberAsBites(result);
    cout << endl;
    cout << "Wynik: " << result.number << endl;
}
