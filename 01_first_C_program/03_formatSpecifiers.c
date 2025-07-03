#include <stdio.h>

int main() {

    // format specifier --> Special tokens that begin with a % symbol,
    //                      followed by a character that specifies the data type
    //                      and optional modifiers (width, precision, flags).
    //                      they control how data is displayed or interpreted.

    // int a = 20;
    // float price = 19.99;
    // double pi = 3.144859990202884;
    // char currency = '$';
    // char name[] = "Jeetu Singh";

    // printf("Age is %d\n", a); // for int
    // printf("Price is %.2f\n", price); // for floating point
    // printf("Pi value is %lf\n", pi); // for double int
    // printf("Currency is : %c\n", currency); // for character
    // printf("Name is : %s\n", name); // for string or array of char

    // how we can add width of values ?

    int num1 = 1;
    int num2 = 10;
    int num3 = -100;

    // printf("%3d\n", num1);
    // printf("%3d\n", num2); // --> 3 specifies minimum number of character to print
    // printf("%3d\n", num3);

    // printf("%-3d\n", num1);
    // printf("%-3d\n", num2); // --> 3 specifies minimum number of character to print
    // printf("%-3d\n", num3);

    // printf("%03d\n", num1);
    // printf("%03d\n", num2); // --> 3 specifies minimum number of character to print
    // printf("%03d\n", num3);

    // printf("%+d\n", num1);
    // printf("%+d\n", num2); // --> 3 specifies minimum number of character to print
    // printf("%+d\n", num3);

    // how to add precision to values ?

    // float price1 = 19.99;
    // float price2 = 1.50;
    // float price3 = -100.00;

    // printf("%.2f\n", price1);
    // printf("%.2f\n", price2); // precise upto 2 decimal places
    // printf("%.2f\n", price3);

    // width --> precision --> flags can be done together.

    return 0;

}