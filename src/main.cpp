#include <fstream>
#include <chrono>
#include <stdint.h>
#include <gtest/gtest.h>

using namespace std;

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

class Timer {
public:
    std::chrono::high_resolution_clock::time_point startCounting;
    std::chrono::high_resolution_clock::time_point stopCounting;

    void Start()
    {
		startCounting = std::chrono::high_resolution_clock::now();
	}

    void Stop()
    {
        stopCounting = std::chrono::high_resolution_clock::now();
	}

    long timeInMS()
    {
    	return std::chrono::duration_cast<std::chrono::microseconds>(Timer::stopCounting - Timer::startCounting).count();
	}
};

void addOrSubtract(float80* a, float80* b, float80* result) // parametry wejsciowe jako unie
{
    uint16_t result_sign; // na tych zmiennych robione są obliczenia
    uint16_t result_exponent;
    uint64_t result_mantissa;

    uint16_t a_exponent = a->parts.exponent;
    uint16_t b_exponent = b->parts.exponent;
    uint64_t a_mantissa = a->parts.mantissa >> 1; // mantysy sa przesuwane w prawo, aby zrobic miejsce na nadmiar
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
        return;
    }

    //Nieskonczonosc
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

    //Suma takich samych liczb
    if ((a_exponent == b_exponent) && (a_mantissa == b_mantissa))
    {
        if (!different_sign)
        {
            result->parts.sign = a->parts.sign;
            result->parts.exponent = a_exponent + 1;
            result->parts.mantissa = a_mantissa << 1;
            return;
        }
        //Suma liczb przeciwnych, czyli 0
        else
        {
            result->parts.sign = 0;
            result->parts.exponent = 0;
            result->parts.mantissa = 0;
            return;
        }
    }

    // Różnica eksponentów, aby później odpowiednio przesunąć mantysy
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

        //Sprawdzenie czy najstarszy bit jest 1 - czyli czy wystapilo przepelnienie
        if ((result_mantissa >> 63) == 1)
        {
            result_exponent = result_exponent + 1;
        }
        else
        {
            result_mantissa = result_mantissa << 1;

            while ((result_mantissa >> 63) == 0)
            {
                result_mantissa = result_mantissa << 1;
                result_exponent = result_exponent - 1;
            }
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

            while ((result_mantissa >> 63) == 0)
            {
                result_mantissa = result_mantissa << 1;
                result_exponent = result_exponent - 1;
            }
        }
    }

    result->parts.sign = result_sign;
    result->parts.exponent = result_exponent;
    result->parts.mantissa = result_mantissa;
}

void multiply(float80* a, float80* b, float80* result)
{
    uint16_t result_sign; // na tych zmiennych robione są obliczenia
    uint16_t result_exponent;
    uint64_t result_mantissa;

    uint16_t a_exponent = a->parts.exponent - 16383;
    uint16_t b_exponent = b->parts.exponent - 16383;
    uint64_t a_mantissa = a->parts.mantissa;
    uint64_t b_mantissa = b->parts.mantissa;

    int16_t different_sign = a->parts.sign ^ b->parts.sign;

    result_exponent = (a_exponent + b_exponent) + 16383;

    if (different_sign)
    {
        result->parts.sign = 1;
    }
    else
    {
        result->parts.sign = 0;
    }

    __uint128_t r_mantissa = (__uint128_t)a_mantissa * (__uint128_t)b_mantissa;

    while ((r_mantissa >> 127) == 0)
    {
        r_mantissa = r_mantissa << 1;
    }

    result_mantissa = (uint64_t)(r_mantissa >> 64);

    result->parts.exponent = result_exponent;
    result->parts.mantissa = result_mantissa;
}

//Funkcja pomocnicza do testów dodawania i odejmowania
long double calculateAddOrSubtract(long double a, long double b)
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

//Funkcja pomocnicza do testów mnożenia
long double calculateMultiply(long double a, long double b)
{
    float80 number1 = { .number = a };
    float80 number2 = { .number = b };
    float80 result = { .number = 0 };

    float80* number1_p = &number1;
    float80* number2_p = &number2;
    float80* result_p = &result;

    multiply(number1_p, number2_p, result_p);

    return result.number;
}

//Funkcja do wyświetlania liczby w postaci bitowej
void printNumberAsBites(float80 input)
{
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

void fileMode(bool multiplication)
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

    Timer timer1;
    Timer timer2;

    cout.precision(10);

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

        if (!multiplication)
        {
            timer1.Start();
            ldResult = ldNumber1 + ldNumber2;
            timer1.Stop();
            cout << ldNumber1 << " + " << ldNumber2 << " = " << ldResult << "   ";

            timer2.Start();
            addOrSubtract(number1_p, number2_p, result_p);
            timer2.Stop();
        }
        else
        {
            timer1.Start();
            ldResult = ldNumber1 * ldNumber2;
            timer1.Stop();
            cout << ldNumber1 << " * " << ldNumber2 << " = " << ldResult << "   ";

            timer2.Start();
            multiply(number1_p, number2_p, result_p);
            timer2.Stop();
        }

        if (result.number == ldResult)
        {
            cout << "OK";
        }
        else
        {
            cout << result.number;
        }

        cout << "   TArytm: " << (long)timer1.timeInMS() << ", TBit: " << (long)timer2.timeInMS() << endl;

        //Postać bitowa liczb
//        printNumberAsBites(number1);
//        printNumberAsBites(number2);
//        printNumberAsBites(result);
    }

    file.close();
}

int main(int argc, char **argv)
{
    cout << sizeof(long double) << endl;

    char option;
    do
    {
        cout << endl;
        cout << "MENU" << endl;
        cout << endl;
        cout << "1 - tryb testow" << endl;
        cout << "2 - tryb pliku - dodawanie i odejmowanie" << endl;
        cout << "3 - tryb pliku - mnożenie" << endl;
        cout << "0 - wyjscie" << endl;

        cin >> option;
        cout << endl;

        switch(option)
        {
            case '1':
                testing::InitGoogleTest(&argc, argv);
                RUN_ALL_TESTS();
                break;

            case '2':
                fileMode(false);
                break;

            case '3':
                fileMode(true);
                break;
        }
    }
    while(option != '0');
}

TEST(LongDoubleAddTest, add)
{
    ASSERT_DOUBLE_EQ(6, calculateAddOrSubtract(2, 4));
    ASSERT_DOUBLE_EQ(31, calculateAddOrSubtract(17, 14));
    ASSERT_DOUBLE_EQ(120454.25, calculateAddOrSubtract(120086, 368.25));
    ASSERT_DOUBLE_EQ(547.3694, calculateAddOrSubtract(478.368, 69.0014));
    ASSERT_DOUBLE_EQ(37.0587, calculateAddOrSubtract(0.258, 36.8007));
    ASSERT_DOUBLE_EQ(2.41406, calculateAddOrSubtract(0.04805, 2.36601));
    ASSERT_DOUBLE_EQ(1432.239042, calculateAddOrSubtract(489.783242, 942.4558));
    ASSERT_DOUBLE_EQ(1123941.55163, calculateAddOrSubtract(1100863.001, 23078.55063));
    ASSERT_DOUBLE_EQ(3, calculateAddOrSubtract(1.153, 1.847));
    ASSERT_DOUBLE_EQ(51432.8765, calculateAddOrSubtract(42687.54253, 8745.33397));
    ASSERT_DOUBLE_EQ(200, calculateAddOrSubtract(100, 100));
    ASSERT_DOUBLE_EQ(687522.425, calculateAddOrSubtract(5462.2465, 682060.1785));
    ASSERT_DOUBLE_EQ(36.902416635, calculateAddOrSubtract(25.900017625, 11.00239901));
    ASSERT_DOUBLE_EQ(0.120015009, calculateAddOrSubtract(0.020009, 0.100006009));
    ASSERT_DOUBLE_EQ(142532474.9707104, calculateAddOrSubtract(99978309.7140552, 42554165.2566552));
}

TEST(LongDoubleSubtractTest, sub)
{
    ASSERT_DOUBLE_EQ(423, calculateAddOrSubtract(-9, 432));
    ASSERT_DOUBLE_EQ(50, calculateAddOrSubtract(60, -10));
    ASSERT_DOUBLE_EQ(0, calculateAddOrSubtract(1900025.14, -1900025.14));
    ASSERT_DOUBLE_EQ(-66944.71, calculateAddOrSubtract(-34799.48, -32145.23));
    ASSERT_DOUBLE_EQ(-450, calculateAddOrSubtract(-150, -300));
    ASSERT_DOUBLE_EQ(-37.0587, calculateAddOrSubtract(-0.258, -36.8007));
    ASSERT_DOUBLE_EQ(-2.41406, calculateAddOrSubtract(-0.04805, -2.36601));
    ASSERT_DOUBLE_EQ(-1432.239042, calculateAddOrSubtract(-489.783242, -942.4558));
    ASSERT_DOUBLE_EQ(-0.00045, calculateAddOrSubtract(-0.00041, -0.00004));
    ASSERT_DOUBLE_EQ(-54254.55, calculateAddOrSubtract(-5426.24, -48828.31));
    ASSERT_DOUBLE_EQ(-200, calculateAddOrSubtract(-100, -100));
    ASSERT_DOUBLE_EQ(-687522.425, calculateAddOrSubtract(-5462.2465, -682060.1785));
    ASSERT_DOUBLE_EQ(-36.902416635, calculateAddOrSubtract(-25.900017625, -11.00239901));
    ASSERT_DOUBLE_EQ(-0.120015009, calculateAddOrSubtract(-0.020009, -0.100006009));
    ASSERT_DOUBLE_EQ(-142532474.9707104, calculateAddOrSubtract(-99978309.7140552, -42554165.2566552));
}

TEST(LongDoubleMultiplyTest, mul)
{
    ASSERT_DOUBLE_EQ(3888, calculateMultiply(9, 432));
    ASSERT_DOUBLE_EQ(120, calculateMultiply(60, 2));
    ASSERT_DOUBLE_EQ(-220, calculateMultiply(10, -22));
    ASSERT_DOUBLE_EQ(45000, calculateMultiply(-150, -300));
    ASSERT_DOUBLE_EQ(9.4945806, calculateMultiply(-0.258, -36.8007));
}

