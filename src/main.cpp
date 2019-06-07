#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits>
#include <random>
#include "Float80.h"

#define exponent(x) (x << 1) >> 1
#define sign(x) x >> 15

using namespace std;

long double powEx(int input)
{
    int ex = input;
    long double output = 1;

    if (input > 0)
    {
        while (ex > 0)
        {
            output *= 2;
            ex--;
        }

        return output;
    }
    else if (input <= 0)
    {
        while (ex < 0)
        {
            output *= 2;
            ex++;
        }

        output = 1/output;

        return output;
    }
    else return 0;
}

unsigned char reverseBits(unsigned char input)
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

void add(Float80* a, Float80* b, Float80* result)
{
    uint64_t result_mantissa;
    uint16_t result_exponent;
    uint16_t result_sign;

    int16_t different_sign = sign(a->signAndExponent) ^ sign(b->signAndExponent);

    //NaN
    if (!(exponent(a->signAndExponent) ^ 0x7FFF) && a->mantissa)
    {
        result->signAndExponent = a->signAndExponent;
        result->mantissa = a->mantissa;
        return;
    }
    if (!(exponent(b->signAndExponent) ^ 0x7FFF) && b->mantissa)
    {
        result->signAndExponent = b->signAndExponent;
        result->mantissa = b->mantissa;
        return;
    }

    //Nieskoczonosc
    if (!(exponent(a->signAndExponent) ^ 0x7FFF) && !(exponent(b->signAndExponent) ^ 0x7FFF))
    {
        if (different_sign)
        {
            result->signAndExponent = 0x7FFF;
            result->mantissa = a->mantissa + 1;
            return;
        }
        else
        {
            result->signAndExponent = a->signAndExponent;
            result->mantissa = a->mantissa;
            return;
        }
    }
    else if (!(exponent(a->signAndExponent) ^ 0x7FFF))
    {
        result->signAndExponent = a->signAndExponent;
        result->mantissa = a->mantissa;
        return;
    }
    else if (!(exponent(b->signAndExponent) ^ 0x7FFF))
    {
        result->signAndExponent = b->signAndExponent;
        result->mantissa = b->mantissa;
        return;
    }

    uint16_t exp_difference = 0;

    if (different_sign)
    {
        exp_difference = exponent(b->signAndExponent) + exponent(a->signAndExponent);
    }
    else
    {
        if (exponent(a->signAndExponent) > exponent(b->signAndExponent))
        {
            exp_difference = exponent(a->signAndExponent) - exponent(b->signAndExponent);
        }
        else if (exponent(a->signAndExponent) < exponent(b->signAndExponent))
        {
            exp_difference = exponent(b->signAndExponent) - exponent(a->signAndExponent);
        }
    }

    bool x_bigger_abs;
    if      (exponent(a->signAndExponent) > exponent(b->signAndExponent)) x_bigger_abs = true;
    else if (exponent(a->signAndExponent) < exponent(b->signAndExponent)) x_bigger_abs = false;
    else if (a->mantissa > b->mantissa) x_bigger_abs = true;
    else                                x_bigger_abs = false;

    if (!different_sign)
    {
        result_sign = sign(a->signAndExponent);

        if (x_bigger_abs) {
            result_mantissa = (a->mantissa << 1) + (b->mantissa << 1) >> exp_difference;
            result_exponent = exponent(a->signAndExponent);
        }
        else {
            result_mantissa = (b->mantissa << 1) + ((a->mantissa << 1) >> exp_difference);
            result_exponent = exponent(b->signAndExponent);
        }
        if (result_mantissa << 63) result_mantissa = (result_mantissa >> 1) + 1;
        else result_mantissa = (result_mantissa >> 1);
    }
    else
    {
        if (x_bigger_abs) {
            result_sign = sign(a->signAndExponent);
            result_exponent = exponent(a->signAndExponent);

            result_mantissa = (a->mantissa << 1) - ((b->mantissa << 1) >> exp_difference );
        }
        else {
            result_sign = sign(b->signAndExponent);
            result_exponent = exponent(b->signAndExponent);

            result_mantissa = (b->mantissa << 1) - ((a->mantissa << 1) >> exp_difference);
        }

        if (result_mantissa << 63)  result_mantissa = ((result_mantissa >> 1) + 1);
        else result_mantissa = (result_mantissa >> 1);
    }

    uint16_t signa = sign(a->signAndExponent);
    uint16_t signb = sign(b->signAndExponent);
    uint16_t expa = exponent(a->signAndExponent);
    uint16_t expb = exponent(b->signAndExponent);

    cout<<"A: " << signa << " " << expa << " " << a->mantissa << endl;
    cout<<"B: " << signb << " " << expb << " " << b->mantissa << endl;
    cout<<"R: " << result_sign << " " << result_exponent << " " << result_mantissa<<endl;

    uint16_t result_signAndExponent = result_sign << 15 | result_exponent;

    result->signAndExponent = result_signAndExponent;
    result->mantissa = result_mantissa;
}

uint16_t createSignAndExponent(long double input)
{
    unsigned char arr[sizeof(long double)];
    memcpy(arr, &input, sizeof(input));

    uint16_t output = 0;

    for (int i = 0; i != 2; ++i)
    {
        unsigned char c = arr[8 + i];

        for (int j = 0; j < 8; j++)
        {
            if (c % 2 != 0)
            {
                output += (1 << (8 * i + j));
            }

            c >>= 1;
        }
    }

    return output;
}

uint64_t createMantissa(long double input)
{
    unsigned char arr[sizeof(long double)];
    memcpy(arr, &input, sizeof(input));

    uint64_t output = 0;

    for (int i = 0; i < 8; ++i)
    {
        unsigned char c = arr[i];

        for (int j = 0; j < 8; j++)
        {
            if (c % 2 != 0)
            {
                output += (1 << (63 - (8 * i + j)));
            }

            c >>= 1;
        }
    }

    return output;
}

void printNumberAsBits(long double input)
{
    cout << input << ": ";

    unsigned char arr[sizeof(long double)];
    memcpy(arr, &input, sizeof(input));

    for (int i = 0; i != 10; ++i)
    {
        unsigned char c = reverseBits(arr[9 - i]);

        for (int j = 0; j < 8; j++)
        {
                cout << c % 2;
                c >>= 1;
        }
    }

    cout << endl;
}

long double convertToFloat(Float80* input)
{
    uint16_t sign = sign(input->signAndExponent);
    uint16_t exponent = exponent(input->signAndExponent);
    uint64_t mantissa = input->mantissa;

    int e = exponent - 32767;

    long double pp = powEx(e);
    long double output = mantissa * pp;

    if (sign == 1) output *= -1;

    return output;
}

int main()
{
    Float80 number1;
    Float80 number2;
    Float80 result;

    Float80* number1_p = &number1;
    Float80* number2_p = &number2;
    Float80* result_p = &result;

    long double fNumber1;
    long double fNumber2;
    long double fResult;

    char input1[] = "12";
    char input2[] = "1";

    sscanf(input1, "%Lf", &fNumber1);
    sscanf(input2, "%Lf", &fNumber2);

    number1.signAndExponent = createSignAndExponent(fNumber1);
    number1.mantissa = createMantissa(fNumber1);

    number2.signAndExponent = createSignAndExponent(fNumber2);
    number2.mantissa = createMantissa(fNumber2);

    printNumberAsBits(fNumber1);
    printNumberAsBits(fNumber2);

    long double sum = *(long double*) &fNumber1 + *(long double*) &fNumber2;
    cout << *(long double*) &fNumber1 << " + " << *(long double*) &fNumber2 << " = " << sum << endl;

    add(number1_p, number2_p, result_p);
    fResult = convertToFloat(result_p);

    //cout << "Wynik: " << *(long double*) &fResult << endl;

    printNumberAsBits(fResult);
}

