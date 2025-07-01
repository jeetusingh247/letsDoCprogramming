#include <stdio.h>

int main() {

    // variable = a reusable container for a value.
    //            Behaves as if it were the value it contains.

    // Integers

    // int age = 22; // integers can hold decimal numbers
    // int year = 2025;
    // int quantity = 2;

    // printf("You are %d years old\n", age);
    // printf("The year is %d\n", year);
    // printf("You have ordered %d x items", quantity);

    // Float

    float scgpa = 8.9;
    float price = 19.99;
    float temperature = -10.1;

    printf("Your scgpa is %.1f\n", scgpa); // C has default behaviour to show 6 digits after decimal point.
    printf("the price is $%.2f\n", price); // 2 decimal point
    printf("Temperature is %fCelcius\n", temperature);


    return 0;
}