#include <fstream>
#include <iostream>
#include <sstream>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits>
#include <random>
#include <iomanip>
#include <gtest/gtest.h>

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

void addOrSubtract(float80* a, float80* b, float80* result)
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

    bool x_bigger_abs = false;

    if (a_exponent > b_exponent)
    {
        x_bigger_abs = true;
    }
    else if (a_exponent < b_exponent)
    {
        x_bigger_abs = false;
    }
    else if (a_mantissa > b_mantissa)
    {
        x_bigger_abs = true;
    }
    else
    {
        x_bigger_abs = false;
    }

    if (!different_sign)
    {
        result_sign = a->parts.sign;

        if (x_bigger_abs)
        {
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
        if (x_bigger_abs)
        {
            result_sign = a->parts.sign;
            result_mantissa = a_mantissa - (b_mantissa >> exp_difference);
            result_exponent = a_exponent;
        }
        else
        {
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

long double calculate(long double a, long double b)
{
    float80 number1 = { .number = a };
    float80 number2 = { .number = b };
    float80 result = { .number = 0 };

    float80* number1_p = &number1;
    float80* number2_p = &number2;
    float80* result_p = &result;

    addOrSubtract(number1_p, number2_p, result_p);

    return result.number;
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

void fileMode()
{
    long double ldNumber1;
    long double ldNumber2;
    long double ldResult;

    float80 number1 = { .number = 0 };
    float80 number2 = { .number = 0 };
    float80 result = { .number = 0 };

    float80* number1_p = &number1;
    float80* number2_p = &number2;
    float80* result_p = &result;

    cout.precision(6);

    std::ifstream file;
    file.open("data.txt");

    if (!file.is_open())
    {
        cout << "Nie mozna otworzyc pliku :(" << endl;
        return;
    }

    string line;

    while (getline(file, line))
    {
        istringstream iss(line);

        iss >> std::dec >> ldNumber1;
        iss >> std::dec >> ldNumber2;

        iss.clear();

        number1 = { .number = ldNumber1 };
        number2 = { .number = ldNumber2 };

        ldResult = ldNumber1 + ldNumber2;
        cout << ldNumber1 << " + " << ldNumber2 << " = " << ldResult << "   ";

        addOrSubtract(number1_p, number2_p, result_p);

        if (result.number == ldResult)
        {
            cout << "OK" << endl;
        }
        else
        {
            cout << endl;
        }

        //printNumberAsBites(result);
    }

    file.close();
}

int main(int argc, char **argv)
{
    char option;
    do
    {
        cout << "MENU" << endl;
        cout << endl;
        cout << "1 - tryb testow" << endl;
        cout << "2 - tryb pliku" << endl;
        cout << "0 - wyjscie" << endl;

        cin >> option;
        //option = getchar();
        cout << endl;

        switch(option)
        {
            case '1':
                testing::InitGoogleTest(&argc, argv);
                return RUN_ALL_TESTS();
                break;

            case '2':
                fileMode();
                break;
        }
    }
    while(option != '0');
}

TEST(LongDoubleAddTest, add)
{
    ASSERT_DOUBLE_EQ(6, calculate(2, 4));
    ASSERT_DOUBLE_EQ(31, calculate(17, 14));
    ASSERT_DOUBLE_EQ(120454.25, calculate(120086, 368.25));
    ASSERT_DOUBLE_EQ(547.3694, calculate(478.368, 69.0014));
    ASSERT_DOUBLE_EQ(37.0587, calculate(0.258, 36.8007));
    ASSERT_DOUBLE_EQ(2.41406, calculate(0.04805, 2.36601));
    ASSERT_DOUBLE_EQ(1432.239042, calculate(489.783242, 942.4558));
    ASSERT_DOUBLE_EQ(1123941.55163, calculate(1100863.001, 23078.55063));
    ASSERT_DOUBLE_EQ(3, calculate(1.153, 1.847));
    ASSERT_DOUBLE_EQ(51432.8765, calculate(42687.54253, 8745.33397));
    ASSERT_DOUBLE_EQ(200, calculate(100, 100));
    ASSERT_DOUBLE_EQ(687522.425, calculate(5462.2465, 682060.1785));
    ASSERT_DOUBLE_EQ(36.902416635, calculate(25.900017625, 11.00239901));
    ASSERT_DOUBLE_EQ(0.120015009, calculate(0.020009, 0.100006009));
    ASSERT_DOUBLE_EQ(142532474.9707104, calculate(99978309.7140552, 42554165.2566552));
}

TEST(LongDoubleSubtractTest, sub)
{
    ASSERT_DOUBLE_EQ(423, calculate(-9, 432));
    ASSERT_DOUBLE_EQ(50, calculate(60, -10));
    ASSERT_DOUBLE_EQ(0, calculate(1900025.14, -1900025.14));
    ASSERT_DOUBLE_EQ(-66944.71, calculate(-34799.48, -32145.23));
    ASSERT_DOUBLE_EQ(-450, calculate(-150, -300));
    ASSERT_DOUBLE_EQ(-37.0587, calculate(-0.258, -36.8007));
    ASSERT_DOUBLE_EQ(-2.41406, calculate(-0.04805, -2.36601));
    ASSERT_DOUBLE_EQ(-1432.239042, calculate(-489.783242, -942.4558));
    ASSERT_DOUBLE_EQ(-0.00045, calculate(-0.00041, -0.00004));
    ASSERT_DOUBLE_EQ(-54254.55, calculate(-5426.24, -48828.31));
    ASSERT_DOUBLE_EQ(-200, calculate(-100, -100));
    ASSERT_DOUBLE_EQ(-687522.425, calculate(-5462.2465, -682060.1785));
    ASSERT_DOUBLE_EQ(-36.902416635, calculate(-25.900017625, -11.00239901));
    ASSERT_DOUBLE_EQ(-0.120015009, calculate(-0.020009, -0.100006009));
    ASSERT_DOUBLE_EQ(-142532474.9707104, calculate(-99978309.7140552, -42554165.2566552));
}


