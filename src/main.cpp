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

void add(Float80* a, Float80* b, Float80* result)
{
    uint16_t result_sign;
    uint16_t result_exponent;
    uint64_t result_mantissa;

    uint16_t a_exponent = exponent(a->signAndExponent);
    uint16_t b_exponent = exponent(b->signAndExponent);
    uint64_t a_mantissa = revertMantissa(a->mantissa) >> 1;
    uint64_t b_mantissa = revertMantissa(b->mantissa) >> 1;

    printDoubleAsBites(a_mantissa);
    printDoubleAsBites(b_mantissa);

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

    //Suma takich liczb
    if ((a_exponent == b_exponent) && (a_mantissa == b_mantissa))
    {
        if (!different_sign)
        {
            uint16_t signAndExponent = (sign(a->signAndExponent) << 15) | (a_exponent + 1);

            result->signAndExponent = signAndExponent;
            result->mantissa = a->mantissa;
            return;
        }
        else
        {
            result->signAndExponent = 0x0000;
            result->mantissa = 0;
            return;
        }
    }

    int exp_difference;

//    if (different_sign)
//    {
//        exp_difference = exponent(b->signAndExponent) + exponent(a->signAndExponent);
//    }
//    else
//    {
//        exp_difference = abs(a_exponent - b_exponent);
//    }

exp_difference = abs(a_exponent - b_exponent);

    bool x_bigger_abs;
    if      (exponent(a->signAndExponent) > exponent(b->signAndExponent)) x_bigger_abs = true;
    else if (exponent(a->signAndExponent) < exponent(b->signAndExponent)) x_bigger_abs = false;
    else if (a->mantissa > b->mantissa) x_bigger_abs = true;
    else                                x_bigger_abs = false;

    if (!different_sign)
    {
        result_sign = sign(a->signAndExponent);

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

        printDoubleAsBites(result_mantissa);
        result_mantissa = revertMantissa(result_mantissa);

    }
    else
    {
        if (x_bigger_abs) {
            result_sign = sign(a->signAndExponent);
            result_mantissa = a_mantissa - (b_mantissa >> exp_difference);
            result_exponent = a_exponent;
        }
        else {
            result_sign = sign(b->signAndExponent);
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

        printDoubleAsBites(result_mantissa);
        result_mantissa = revertMantissa(result_mantissa);
    }

    result->signAndExponent = (result_sign << 15 | result_exponent);
    result->mantissa = result_mantissa;
}

int checkLastOneBit(uint64_t input)
{
    uint64_t number = input;
    int output = 0;

    for (int i = 0; i < 64; ++i)
    {
        if (number % 2 != 0)
        {
            output = i + 1;
        }

        number >>= 1;
    }

    return output;
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

void printNumberAsBites(long double input)
{
    unsigned char arr[sizeof(long double)];
    memcpy(arr, &input, sizeof(input));

    for (int i = 0; i != 10; ++i)
    {
        unsigned char c = revertBites(arr[9 - i]);

        for (int j = 0; j < 8; j++)
        {
            cout << c % 2;
            c >>= 1;

            if (((i * 8) + j) == 0 | ((i * 8) + j) == 15)
            {
                cout << " ";
            }
        }
    }

    cout << endl;
}

void printDoubleAsBites(uint64_t input)
{
    unsigned char arr[sizeof(uint64_t)];
    memcpy(arr, &input, sizeof(input));

    for (int i = 0; i != 8; ++i)
    {
        unsigned char c = revertBites(arr[7 - i]);

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
    uint16_t _sign = sign(input->signAndExponent);
    uint16_t _exponent = exponent(input->signAndExponent);
    uint64_t mantissa = input->mantissa;
    long double man = 0;
    long double d = 1;

    int e = _exponent - 16383;

    for (int i = 0; i < 64; ++i)
    {
        if (mantissa % 2 != 0)
        {
            man += (1 / pow(2, i));
        }

        mantissa >>= 1;
    }

    long double pp = pow(2, e);
    long double output = man * pp;

    if (_sign == 1) output = -(output);

    return output;
}

void printNumber(long double input, bool endLine = false)
{
    cout.precision(15);
    cout << input;

    if (endLine)
    {
        cout << endl;
    }
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

    char input1[] = "22.5";
    char input2[] = "-2501";

    sscanf(input1, "%Lf", &fNumber1);
    sscanf(input2, "%Lf", &fNumber2);

    number1.signAndExponent = createSignAndExponent(fNumber1);
    number1.mantissa = createMantissa(fNumber1);

    number2.signAndExponent = createSignAndExponent(fNumber2);
    number2.mantissa = createMantissa(fNumber2);

    long double testResult = fNumber1 + fNumber2;
    printNumber(fNumber1);
    cout << " + ";
    printNumber(fNumber2);
    cout << " = ";
    printNumber(testResult, true);

    printNumberAsBites(fNumber1);
    printNumberAsBites(fNumber2);

    cout << endl;
    cout << "Mantysy:" << endl;
    add(number1_p, number2_p, result_p);
    fResult = convertToFloat(result_p);
    cout << endl;

    cout << "Wynik: ";
    printNumber(fResult, true);
    printNumberAsBites(fResult);
}

