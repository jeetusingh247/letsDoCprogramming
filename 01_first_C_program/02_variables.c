#include <stdio.h>
#include <stdbool.h>

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

    // float scgpa = 8.9;
    // float price = 19.99;
    // float temperature = -10.1;

    // printf("Your scgpa is %.1f\n", scgpa); // C has default behaviour to show 6 digits after decimal point.
    // printf("the price is $%.2f\n", price); // 2 decimal point
    // printf("Temperature is %fCelcius\n", temperature);

    // Double
    // double pi = 3.1415939994859503; // 15-16 digits after decimal can be stored
    // double e = 2.71828383939930;

    // printf("The value of pi is %.15lf\n", pi); // format specifier is long float --> double
    // printf("The value of e is %.15lf\n", e);

    // character

    // char grade = 'A';
    // char symbol = '$';

    // printf("Your Grade is %c\n", grade);
    // printf("Symbol of money is %c\n", symbol);

    // In C program we use array of char to represent a string.

    // char name[] = "Jeetu Singh";
    // char food[] = "Double Cheeze Burger";
    // char email[] = "admin@gmail.com";

    // printf("Hello %s\n", name);
    // printf("Would you like to have %s ?\n", food); // %s to display string
    // printf("Your Email : %s\n", email);

    // Boolean --> data type
    // bool isOnline = true; // 1
    // bool canVote = false; // 0

    // printf("Is he/she online : %d\n", isOnline); // --> 1
    // printf("Can he/she vote : %d\n", canVote); // --> 0

    // if (isOnline)
    // {
    //     printf("Yes You Can Connect !");
    // } else
    // {
    //     printf("You cannot connect !");
    // }

    return 0;
}