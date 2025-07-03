#include <stdio.h>
#include <string.h>

int main() {
    
    int age = 0;
    float sgpa = 0.0f;
    char grade ; // --> '\0' // --> null terminator
    char name[30] = ""; // name has max size --> 30 characters

    // --> User Input

    printf("Enter your Age : ");
    scanf("%d", &age); // to take input from user

    printf("Enter your sgpa : ");
    scanf("%f", &sgpa); // --> by default has \n at end in input buffer

    printf("Enter your grade : ");
    scanf(" %c", &grade); // --> to skip new line character

    // printf("Enter your name : ");
    // scanf("%s", &name); // scanf can't read whitespaces

    getchar();

    printf("Enter your full name : ");
    fgets(name, sizeof(name), stdin); // file get string, 30 is size of the input
    name[strlen(name) - 1] = '\0'; // to avoid new line

    printf("Age is : %d\n", age); // --> don't use variable before assigning them
    printf("Sgpa is : %f\n", sgpa); // --> undefined behaviour
    printf("Name is : %s\n", name);
    printf("Grade is : %c", grade); // --> garbage value

    return 0;

}