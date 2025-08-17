/*
 * ATM System in C (with OpenSSL SHA-256 hashing, hidden PIN input, transaction logs)
 * Build (Linux/macOS):  gcc atm.c -o atm -lcrypto
 * Build (Windows, MinGW): gcc atm.c -o atm -lcrypto -lssl -lws2_32
 *
 * First run: uncomment createSampleAccounts(); in main() to seed demo accounts, then re-run with it commented.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <ctype.h>
#include <stdbool.h>

#include <openssl/sha.h>

#ifdef _WIN32
  #include <conio.h>
#else
  #include <termios.h>
  #include <unistd.h>
#endif

/* ======================= Data Model ======================= */

typedef struct {
    int    accountNumber;
    char   name[50];
    char   pinHash[65];    // SHA-256 hex string (64 chars + null)
    double balance;
    int    failedAttempts; // wrong PIN attempts
    int    locked;         // 0 ok, 1 locked
} Account;

/* ======================= Prototypes ======================= */
void   createSampleAccounts(void);
bool   sha256_hex(const char *input, char out_hex[65]);
void   get_hidden_input(char *buf, size_t sz);
void   flush_line(void);

const char* accountsFile(void);

bool   loadAccount(int accountNumber, Account *out);
bool   updateAccount(const Account *acc);
bool   appendAccount(const Account *acc);     // for admin create
bool   accountExists(int accountNumber);

bool   login(Account *outUser);
void   resetFailedAttempts(Account *user);

void   atmMenu(Account *user);
void   balanceInquiry(const Account *user);
void   deposit(Account *user);
void   withdraw(Account *user);
void   changePin(Account *user);
void   transferFunds(Account *user);          // NEW

void   logTransaction(const Account *user, const char *type, double amount, const char *note);
void   showMiniStatement(const Account *user, int lastN); // NEW

/* Admin */
bool   adminLogin(void);
void   adminMenu(void);
void   adminCreateAccount(void);
void   adminListAccounts(void);
void   adminUnlockAccount(void);
void   adminResetPin(void);

/* ======================= Helpers ======================= */
const char* accountsFile(void) {
    return "accounts.dat";
}

void flush_line(void) {
    int c;
    while ((c = getchar()) != '\n' && c != EOF) { /* discard */ }
}

bool sha256_hex(const char *input, char out_hex[65]) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX ctx;
    if (!SHA256_Init(&ctx)) return false;
    if (!SHA256_Update(&ctx, (const unsigned char*)input, strlen(input))) return false;
    if (!SHA256_Final(hash, &ctx)) return false;

    static const char *hex = "0123456789abcdef";
    for (int i = 0; i < SHA256_DIGEST_LENGTH; ++i) {
        out_hex[i*2]     = hex[(hash[i] >> 4) & 0xF];
        out_hex[i*2 + 1] = hex[(hash[i]     ) & 0xF];
    }
    out_hex[64] = '\0';
    return true;
}

/* Hidden input for PIN (no echo) */
void get_hidden_input(char *buf, size_t sz) {
#ifdef _WIN32
    size_t i = 0;
    for (;;) {
        int ch = _getch();           // no echo
        if (ch == '\r' || ch == '\n') {
            putchar('\n');
            break;
        } else if (ch == 8 || ch == 127) { // backspace
            if (i > 0) {
                i--;
            }
        } else if (isprint(ch)) {
            if (i + 1 < sz) {
                buf[i++] = (char)ch;
            }
        }
    }
    buf[i] = '\0';
#else
    struct termios oldt, newt;
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    if (fgets(buf, (int)sz, stdin) == NULL) {
        buf[0] = '\0';
    }
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    // strip newline
    buf[strcspn(buf, "\r\n")] = '\0';
    putchar('\n');
#endif
}

/* ======================= Storage ======================= */
bool loadAccount(int accountNumber, Account *out) {
    FILE *fp = fopen(accountsFile(), "rb");
    if (!fp) return false;

    Account tmp;
    while (fread(&tmp, sizeof(Account), 1, fp) == 1) {
        if (tmp.accountNumber == accountNumber) {
            *out = tmp;
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

bool updateAccount(const Account *acc) {
    FILE *fp = fopen(accountsFile(), "rb+");
    if (!fp) return false;

    Account tmp;
    while (fread(&tmp, sizeof(Account), 1, fp) == 1) {
        if (tmp.accountNumber == acc->accountNumber) {
            if (fseek(fp, -(long)sizeof(Account), SEEK_CUR) != 0) { fclose(fp); return false; }
            if (fwrite(acc, sizeof(Account), 1, fp) != 1)        { fclose(fp); return false; }
            fclose(fp);
            return true;
        }
    }
    fclose(fp);
    return false;
}

bool appendAccount(const Account *acc) {
    FILE *fp = fopen(accountsFile(), "ab");
    if (!fp) return false;
    bool ok = fwrite(acc, sizeof(Account), 1, fp) == 1;
    fclose(fp);
    return ok;
}

bool accountExists(int accountNumber) {
    Account x;
    return loadAccount(accountNumber, &x);
}

/* ======================= Logging ======================= */
void logTransaction(const Account *user, const char *type, double amount, const char *note) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%d_log.txt", user->accountNumber);

    FILE *fp = fopen(filename, "a");
    if (!fp) return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char ts[32];
    strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(fp, "[%s] %-10s Amount: %.2f  Balance: %.2f", ts, type, amount, user->balance);
    if (note && *note) fprintf(fp, "  Note: %s", note);
    fputc('\n', fp);
    fclose(fp);
}

void showMiniStatement(const Account *user, int lastN) {
    char filename[64];
    snprintf(filename, sizeof(filename), "%d_log.txt", user->accountNumber);

    FILE *fp = fopen(filename, "r");
    if (!fp) {
        printf("No transactions yet.\n");
        return;
    }

    // Store last N lines (simple ring buffer)
    char **bufs = (char**)calloc(lastN, sizeof(char*));
    for (int i = 0; i < lastN; ++i) {
        bufs[i] = (char*)calloc(256, 1);
    }

    int count = 0;
    while (fgets(bufs[count % lastN], 256, fp)) {
        count++;
    }
    fclose(fp);

    int toShow = count < lastN ? count : lastN;
    if (toShow == 0) {
        printf("No transactions to show.\n");
    } else {
        printf("\n--- Last %d transactions ---\n", toShow);
        int start = (count - toShow);
        for (int i = 0; i < toShow; ++i) {
            int idx = (start + i) % lastN;
            printf("%s", bufs[idx]);
        }
        printf("---------------------------\n");
    }

    for (int i = 0; i < lastN; ++i) free(bufs[i]);
    free(bufs);
}

/* ======================= Auth ======================= */
void resetFailedAttempts(Account *user) {
    if (user->failedAttempts != 0) {
        user->failedAttempts = 0;
        (void)updateAccount(user);
    }
}

bool login(Account *outUser) {
    int accNo;
    printf("\nEnter Account Number: ");
    if (scanf("%d", &accNo) != 1) { printf("Invalid input.\n"); return false; }
    flush_line();

    Account user;
    if (!loadAccount(accNo, &user)) {
        printf("Account not found.\n");
        return false;
    }

    if (user.locked) {
        printf("❌ Account is LOCKED due to multiple wrong PIN attempts. Contact admin.\n");
        return false;
    }

    char pin[64];
    printf("Enter PIN (hidden): ");
    get_hidden_input(pin, sizeof(pin));

    char pinHash[65];
    if (!sha256_hex(pin, pinHash)) {
        printf("Hash error.\n");
        return false;
    }
    if (strcmp(pinHash, user.pinHash) != 0) {
        user.failedAttempts++;
        if (user.failedAttempts >= 3) {
            user.locked = 1;
            printf("❌ Invalid PIN. Account LOCKED after 3 failed attempts.\n");
        } else {
            printf("❌ Invalid PIN. Attempts: %d/3\n", user.failedAttempts);
        }
        (void)updateAccount(&user);
        return false;
    }

    resetFailedAttempts(&user);
    *outUser = user;
    printf("\n✅ Login successful! Welcome, %s (A/C %d)\n", user.name, user.accountNumber);
    return true;
}

/* ======================= Features ======================= */
void balanceInquiry(const Account *user) {
    printf("💰 Current Balance: %.2f\n", user->balance);
}

void deposit(Account *user) {
    double amount;
    printf("Enter deposit amount: ");
    if (scanf("%lf", &amount) != 1) { printf("Invalid input.\n"); flush_line(); return; }
    flush_line();

    if (amount <= 0) {
        printf("❌ Amount must be positive.\n");
        return;
    }
    user->balance += amount;
    if (!updateAccount(user)) {
        printf("⚠️ Failed to update account on disk.\n");
        return;
    }
    logTransaction(user, "DEPOSIT", amount, "");
    printf("✅ Deposit successful. New Balance: %.2f\n", user->balance);
}

void withdraw(Account *user) {
    double amount;
    printf("Enter withdrawal amount: ");
    if (scanf("%lf", &amount) != 1) { printf("Invalid input.\n"); flush_line(); return; }
    flush_line();

    if (amount <= 0) {
        printf("❌ Amount must be positive.\n");
        return;
    }
    if (amount > user->balance) {
        printf("❌ Insufficient funds.\n");
        return;
    }
    user->balance -= amount;
    if (!updateAccount(user)) {
        printf("⚠️ Failed to update account on disk.\n");
        return;
    }
    logTransaction(user, "WITHDRAW", amount, "");
    printf("✅ Withdrawal successful. New Balance: %.2f\n", user->balance);
}

void changePin(Account *user) {
    char oldpin[64], newpin1[64], newpin2[64];

    printf("Enter current PIN (hidden): ");
    get_hidden_input(oldpin, sizeof(oldpin));

    char oldHash[65];
    if (!sha256_hex(oldpin, oldHash)) { printf("Hash error.\n"); return; }
    if (strcmp(oldHash, user->pinHash) != 0) { printf("❌ Incorrect current PIN.\n"); return; }

    printf("Enter new PIN (hidden): ");
    get_hidden_input(newpin1, sizeof(newpin1));
    printf("Confirm new PIN (hidden): ");
    get_hidden_input(newpin2, sizeof(newpin2));

    if (strcmp(newpin1, newpin2) != 0) {
        printf("❌ PINs do not match.\n");
        return;
    }
    if (strlen(newpin1) < 4) {
        printf("❌ PIN too short (min 4 digits recommended).\n");
        return;
    }

    char newHash[65];
    if (!sha256_hex(newpin1, newHash)) { printf("Hash error.\n"); return; }
    strcpy(user->pinHash, newHash);

    if (!updateAccount(user)) {
        printf("⚠️ Failed to update account on disk.\n");
        return;
    }
    logTransaction(user, "PIN-CHG", 0.0, "PIN updated");
    printf("✅ PIN changed successfully.\n");
}

void transferFunds(Account *user) {
    int toAcc;
    double amount;
    printf("Enter target Account Number: ");
    if (scanf("%d", &toAcc) != 1) { printf("Invalid input.\n"); flush_line(); return; }
    flush_line();

    if (toAcc == user->accountNumber) {
        printf("❌ Cannot transfer to the same account.\n");
        return;
    }

    Account target;
    if (!loadAccount(toAcc, &target)) {
        printf("❌ Target account not found.\n");
        return;
    }
    if (target.locked) {
        printf("❌ Target account is locked.\n");
        return;
    }

    printf("Enter amount to transfer: ");
    if (scanf("%lf", &amount) != 1) { printf("Invalid input.\n"); flush_line(); return; }
    flush_line();

    if (amount <= 0) {
        printf("❌ Amount must be positive.\n");
        return;
    }
    if (amount > user->balance) {
        printf("❌ Insufficient funds.\n");
        return;
    }

    user->balance   -= amount;
    target.balance  += amount;

    if (!updateAccount(user)) { printf("⚠️ Failed to update sender on disk.\n"); return; }
    if (!updateAccount(&target)) { printf("⚠️ Failed to update target on disk.\n"); return; }

    char note1[64], note2[64];
    snprintf(note1, sizeof(note1), "to %d", target.accountNumber);
    snprintf(note2, sizeof(note2), "from %d", user->accountNumber);

    logTransaction(user,   "TRANSFER-", amount, note1);
    logTransaction(&target,"TRANSFER+", amount, note2);

    printf("✅ Transferred %.2f to A/C %d. New Balance: %.2f\n", amount, target.accountNumber, user->balance);
}

/* ======================= Menus ======================= */
void atmMenu(Account *user) {
    for (;;) {
        int choice;
        printf("\n--- ATM Menu ---\n");
        printf("1. Balance Inquiry\n");
        printf("2. Deposit\n");
        printf("3. Withdraw\n");
        printf("4. Change PIN\n");
        printf("5. Transfer Funds\n");
        printf("6. Mini Statement (last 5)\n");
        printf("7. Exit\n");
        printf("Enter choice: ");
        if (scanf("%d", &choice) != 1) { printf("Invalid input.\n"); flush_line(); continue; }
        flush_line();

        switch (choice) {
            case 1: balanceInquiry(user); break;
            case 2: deposit(user); break;
            case 3: withdraw(user); break;
            case 4: changePin(user); break;
            case 5: transferFunds(user); break;
            case 6: showMiniStatement(user, 5); break;
            case 7: printf("👋 Thank you for using ATM.\n"); return;
            default: printf("Invalid choice.\n");
        }
    }
}

/* ======================= Admin ======================= */
/* Super simple admin auth (demo only). 
   Admin password: "admin123" (hashed here at runtime for compare). */
bool adminLogin(void) {
    char pass[64];
    char inHash[65], targetHash[65];

    printf("\n--- Admin Login ---\nPassword (hidden): ");
    get_hidden_input(pass, sizeof(pass));

    if (!sha256_hex(pass, inHash)) return false;
    // precompute hash of "admin123"
    (void)sha256_hex("admin123", targetHash);

    if (strcmp(inHash, targetHash) == 0) {
        printf("✅ Admin authenticated.\n");
        return true;
    }
    printf("❌ Wrong admin password.\n");
    return false;
}

void adminCreateAccount(void) {
    Account a = {0};
    char pin[64];

    printf("\n--- Create Account ---\n");
    printf("Enter new Account Number: ");
    if (scanf("%d", &a.accountNumber) != 1) { printf("Invalid input.\n"); flush_line(); return; }
    flush_line();

    if (accountExists(a.accountNumber)) {
        printf("❌ Account number already exists.\n");
        return;
    }

    printf("Enter name: ");
    if (!fgets(a.name, sizeof(a.name), stdin)) { printf("Input error.\n"); return; }
    a.name[strcspn(a.name, "\r\n")] = '\0';

    printf("Enter initial PIN (hidden, min 4): ");
    get_hidden_input(pin, sizeof(pin));
    if (strlen(pin) < 4) { printf("❌ PIN too short.\n"); return; }
    (void)sha256_hex(pin, a.pinHash);

    printf("Enter initial balance: ");
    if (scanf("%lf", &a.balance) != 1) { printf("Invalid input.\n"); flush_line(); return; }
    flush_line();

    a.failedAttempts = 0;
    a.locked = 0;

    if (!appendAccount(&a)) {
        printf("⚠️ Failed to save account.\n");
        return;
    }
    printf("✅ Account created: %d (%s) with balance %.2f\n", a.accountNumber, a.name, a.balance);
}

void adminListAccounts(void) {
    FILE *fp = fopen(accountsFile(), "rb");
    if (!fp) { printf("No accounts file.\n"); return; }

    printf("\n--- All Accounts ---\n");
    Account a;
    int n = 0;
    while (fread(&a, sizeof(Account), 1, fp) == 1) {
        printf("A/C %-6d | %-20s | Bal: %10.2f | Locked: %d | Attempts: %d\n",
               a.accountNumber, a.name, a.balance, a.locked, a.failedAttempts);
        n++;
    }
    if (n == 0) printf("(none)\n");
    fclose(fp);
}

void adminUnlockAccount(void) {
    int acc;
    printf("Enter account to unlock: ");
    if (scanf("%d", &acc) != 1) { printf("Invalid input.\n"); flush_line(); return; }
    flush_line();

    Account a;
    if (!loadAccount(acc, &a)) { printf("Account not found.\n"); return; }

    a.locked = 0;
    a.failedAttempts = 0;
    if (!updateAccount(&a)) { printf("Failed to update.\n"); return; }
    printf("✅ Account %d unlocked.\n", acc);
}

void adminResetPin(void) {
    int acc;
    char newpin[64], hash[65];

    printf("Enter account to reset PIN: ");
    if (scanf("%d", &acc) != 1) { printf("Invalid input.\n"); flush_line(); return; }
    flush_line();

    Account a;
    if (!loadAccount(acc, &a)) { printf("Account not found.\n"); return; }

    printf("Enter new PIN (hidden, min 4): ");
    get_hidden_input(newpin, sizeof(newpin));
    if (strlen(newpin) < 4) { printf("❌ PIN too short.\n"); return; }

    (void)sha256_hex(newpin, hash);
    strcpy(a.pinHash, hash);
    a.failedAttempts = 0;
    a.locked = 0;

    if (!updateAccount(&a)) { printf("Failed to update.\n"); return; }
    printf("✅ PIN reset for A/C %d.\n", acc);
}

void adminMenu(void) {
    if (!adminLogin()) return;

    for (;;) {
        int ch;
        printf("\n--- Admin Menu ---\n");
        printf("1. Create Account\n");
        printf("2. List Accounts\n");
        printf("3. Unlock Account\n");
        printf("4. Reset PIN\n");
        printf("5. Exit Admin\n");
        printf("Enter choice: ");
        if (scanf("%d", &ch) != 1) { printf("Invalid input.\n"); flush_line(); continue; }
        flush_line();

        switch (ch) {
            case 1: adminCreateAccount(); break;
            case 2: adminListAccounts();  break;
            case 3: adminUnlockAccount(); break;
            case 4: adminResetPin();      break;
            case 5: return;
            default: printf("Invalid choice.\n");
        }
    }
}

/* ======================= Seed Sample Accounts ======================= */
void createSampleAccounts(void) {
    FILE *fp = fopen(accountsFile(), "wb");
    if (!fp) {
        printf("Failed to create accounts file.\n");
        exit(1);
    }

    Account users[2];

    users[0].accountNumber = 1001;
    strcpy(users[0].name, "Alice");
    users[0].balance = 5000.0;
    sha256_hex("1234", users[0].pinHash);  // demo PIN 1234
    users[0].failedAttempts = 0;
    users[0].locked = 0;

    users[1].accountNumber = 1002;
    strcpy(users[1].name, "Bob");
    users[1].balance = 3000.0;
    sha256_hex("4321", users[1].pinHash);  // demo PIN 4321
    users[1].failedAttempts = 0;
    users[1].locked = 0;

    if (fwrite(users, sizeof(Account), 2, fp) != 2) {
        printf("Failed to write accounts.\n");
        fclose(fp);
        exit(1);
    }
    fclose(fp);
    printf("Sample accounts created: [1001/1234], [1002/4321]\n");
}

/* ======================= main ======================= */
int main(void) {
    printf("🏦 ATM System (C + OpenSSL)\n");

    // ---- First run ONLY: uncomment the next line to seed demo accounts, then comment again ----
    createSampleAccounts();

    for (;;) {
        int mode;
        printf("\n1. User Login\n2. Admin\n3. Exit\nChoose: ");
        if (scanf("%d", &mode) != 1) { printf("Invalid input.\n"); flush_line(); continue; }
        flush_line();

        if (mode == 1) {
            Account user;
            if (!login(&user)) {
                printf("Exiting to main menu.\n");
                continue;
            }
            atmMenu(&user);
        } else if (mode == 2) {
            adminMenu();
        } else if (mode == 3) {
            printf("Bye!\n");
            break;
        } else {
            printf("Invalid choice.\n");
        }
    }
    return 0;
}
