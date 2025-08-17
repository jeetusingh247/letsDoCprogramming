#include <stdio.h>
#include <string.h>

int main() {

    // SHOPPING CART PROGRAM

    char item[50] = "";
    float price = 0.0f;
    int quantity = 0;
    char currency = '$';
    float total = 0.0f;

    printf("Enter item you need to buy : ");
    fgets(item, sizeof(item), stdin);
    item[strlen(item) - 1] = '\0';

    printf("Enter the price of each product : ");
    scanf("%f", &price);

    printf("Enter Number of product you need to buy : ");
    scanf("%d", &quantity);

    total = price * quantity;

    printf("You have bought %d %s\n", quantity, item);
    printf("the total is : %c%.2f", currency, total);

    return 0;
}