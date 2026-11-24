/*
 * ALIMENTATION SYSTEM - PART 1: KITCHEN
 *
 * What it does:
 *   1. Chef records production (and the materials used)
 *   2. Owner records materials bought (flour, oil, ...)
 *   3. Show stock of products and materials
 *   4. Production report (with cost)
 *
 * Files created next to the program:
 *   data.txt        -> current stock and material prices
 *   production.txt  -> history of every production
 *   purchases.txt   -> history of every material bought
 *
 * Compile:  gcc kitchen.c -o kitchen
 * Run:      ./kitchen        (Windows: kitchen.exe)
 */

#include <stdio.h>
#include <string.h>
#include <time.h>

/* ------------------------------------------------------------------ */
/* 1. SETTINGS: change names, units and materials here                 */
/* ------------------------------------------------------------------ */

#define NUM_PRODUCTS  4
#define NUM_MATERIALS 7

const char *productNames[NUM_PRODUCTS] = {
    "Sambusa",
    "Ibiraha",
    "Chapati",
    "Amandazi (pack of 6)"
};

const char *materialNames[NUM_MATERIALS] = {
    "Wheat flour",
    "Eggs",
    "Cooking oil",
    "Meat",
    "Irish potatoes",
    "Sugar",
    "Packaging envelopes"
};

const char *materialUnits[NUM_MATERIALS] = {
    "kg", "pieces", "litres", "kg", "kg", "kg", "pieces"
};

/* Which material is used by which product (1 = used, 0 = not used)   */
/*                 flour eggs oil meat potatoes sugar envelopes        */
int uses[NUM_PRODUCTS][NUM_MATERIALS] = {
    {            1,    1,   1,   1,    0,       0,    0 },  /* Sambusa  */
    {            1,    0,   1,   0,    1,       0,    0 },  /* Ibiraha  */
    {            1,    0,   1,   0,    0,       0,    0 },  /* Chapati  */
    {            1,    0,   1,   0,    0,       1,    1 }   /* Amandazi */
};

/* ------------------------------------------------------------------ */
/* 2. DATA kept in memory while the program runs                       */
/* ------------------------------------------------------------------ */

int    productStock[NUM_PRODUCTS];     /* how many we have in stock    */
double materialStock[NUM_MATERIALS];   /* how much material is left    */
double materialPrice[NUM_MATERIALS];   /* price per unit (RWF)         */

/* ------------------------------------------------------------------ */
/* 3. SMALL HELPER FUNCTIONS (safe input)                              */
/* ------------------------------------------------------------------ */

/* Ask for a number (0 or more). Asks again if the user types letters. */
double askNumber(const char *question)
{
    char line[100];
    double value;

    while (1) {
        printf("%s", question);
        if (fgets(line, sizeof(line), stdin) == NULL) return 0;
        if (sscanf(line, "%lf", &value) == 1 && value >= 0) return value;
        printf("  Please type a number (0 or more).\n");
    }
}

/* Ask for a whole number between min and max.                         */
int askChoice(const char *question, int min, int max)
{
    char line[100];
    int value;

    while (1) {
        printf("%s", question);
        if (fgets(line, sizeof(line), stdin) == NULL) return min;
        if (sscanf(line, "%d", &value) == 1 && value >= min && value <= max)
            return value;
        printf("  Please choose a number from %d to %d.\n", min, max);
    }
}

/* Put today's date in text like 2026-11-24                            */
void getToday(char *text)
{
    time_t now = time(NULL);
    strftime(text, 11, "%Y-%m-%d", localtime(&now));
}

/* ------------------------------------------------------------------ */
/* 4. SAVE AND LOAD                                                    */
/* ------------------------------------------------------------------ */

void saveData(void)
{
    FILE *f = fopen("data.txt", "w");
    int i;

    if (f == NULL) {
        printf("ERROR: cannot save data!\n");
        return;
    }
    for (i = 0; i < NUM_PRODUCTS; i++)
        fprintf(f, "%d\n", productStock[i]);
    for (i = 0; i < NUM_MATERIALS; i++)
        fprintf(f, "%f %f\n", materialStock[i], materialPrice[i]);
    fclose(f);
}

void loadData(void)
{
    FILE *f = fopen("data.txt", "r");
    int i;

    if (f == NULL) return;   /* first time: everything stays at 0 */

    for (i = 0; i < NUM_PRODUCTS; i++)
        fscanf(f, "%d", &productStock[i]);
    for (i = 0; i < NUM_MATERIALS; i++)
        fscanf(f, "%lf %lf", &materialStock[i], &materialPrice[i]);
    fclose(f);
}

/* ------------------------------------------------------------------ */
/* 5. SHOW STOCK                                                       */
/* ------------------------------------------------------------------ */

void showStock(void)
{
    int i;

    printf("\n===== PRODUCTS IN STOCK =====\n");
    for (i = 0; i < NUM_PRODUCTS; i++)
        printf("  %-22s : %d\n", productNames[i], productStock[i]);

    printf("\n===== MATERIALS IN STOCK =====\n");
    for (i = 0; i < NUM_MATERIALS; i++)
        printf("  %-22s : %.2f %s   (price: %.0f RWF per %s)\n",
               materialNames[i], materialStock[i], materialUnits[i],
               materialPrice[i], materialUnits[i]);
}

/* ------------------------------------------------------------------ */
/* 6. ADD MATERIALS BOUGHT                                             */
/* ------------------------------------------------------------------ */

void addMaterials(void)
{
    int m;
    double quantity, price;
    char date[11], question[100];
    FILE *f;

    printf("\n--- MATERIALS BOUGHT ---\n");
    for (m = 0; m < NUM_MATERIALS; m++)
        printf("  %d. %s\n", m + 1, materialNames[m]);

    m = askChoice("Which material did you buy? (number): ", 1, NUM_MATERIALS) - 1;

    sprintf(question, "How many %s did you buy? ", materialUnits[m]);
    quantity = askNumber(question);

    sprintf(question, "Price of ONE %s (RWF): ", materialUnits[m]);
    price = askNumber(question);

    materialStock[m] += quantity;
    materialPrice[m] = price;      /* keep the latest price */

    /* write in history file */
    getToday(date);
    f = fopen("purchases.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s %d %.2f %.2f\n", date, m, quantity, price);
        fclose(f);
    }

    saveData();
    printf("\nDone! %s stock is now %.2f %s.\n",
           materialNames[m], materialStock[m], materialUnits[m]);
}

/* ------------------------------------------------------------------ */
/* 7. RECORD PRODUCTION (the chef uses this)                           */
/* ------------------------------------------------------------------ */

void recordProduction(void)
{
    int p, m, quantity;
    double used[NUM_MATERIALS];    /* amount used of each material */
    double cost = 0;
    char date[11], question[100];
    FILE *f;

    printf("\n--- RECORD PRODUCTION ---\n");
    for (p = 0; p < NUM_PRODUCTS; p++)
        printf("  %d. %s\n", p + 1, productNames[p]);

    p = askChoice("What did you make? (number): ", 1, NUM_PRODUCTS) - 1;

    if (p == 3)   /* amandazi is counted in packs */
        quantity = (int)askNumber("How many PACKS (of 6 pieces) did you make? ");
    else
        quantity = (int)askNumber("How many pieces did you make? ");

    if (quantity <= 0) {
        printf("Nothing recorded.\n");
        return;
    }

    /* Ask only for the materials this product uses */
    printf("\nNow type the materials you used:\n");
    for (m = 0; m < NUM_MATERIALS; m++) {
        used[m] = 0;
        if (uses[p][m] == 1) {
            sprintf(question, "  %s used (%s): ", materialNames[m], materialUnits[m]);
            used[m] = askNumber(question);
        }
    }

    /* Check we have enough materials in stock */
    for (m = 0; m < NUM_MATERIALS; m++) {
        if (used[m] > materialStock[m]) {
            printf("\nSTOP: not enough %s in stock (you have %.2f %s).\n",
                   materialNames[m], materialStock[m], materialUnits[m]);
            printf("Nothing was recorded. Add materials first (menu 2).\n");
            return;
        }
    }

    /* Update stock and calculate cost */
    for (m = 0; m < NUM_MATERIALS; m++) {
        materialStock[m] -= used[m];
        cost += used[m] * materialPrice[m];
    }
    productStock[p] += quantity;

    /* write in history file */
    getToday(date);
    f = fopen("production.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s %d %d %.2f\n", date, p, quantity, cost);
        fclose(f);
    }

    saveData();
    printf("\nDone! %d %s recorded.\n", quantity, productNames[p]);
    printf("Cost of this production: %.0f RWF  (%.0f RWF per item)\n",
           cost, cost / quantity);
}

/* ------------------------------------------------------------------ */
/* 8. PRODUCTION REPORT                                                */
/* ------------------------------------------------------------------ */

void productionReport(void)
{
    FILE *f = fopen("production.txt", "r");
    char date[11], wanted[100];
    int p, quantity;
    double cost;
    int totalQty[NUM_PRODUCTS] = {0};
    double totalCost[NUM_PRODUCTS] = {0};
    double allCost = 0;
    int found = 0;

    if (f == NULL) {
        printf("\nNo production recorded yet.\n");
        return;
    }

    printf("\nType a date (example 2026-11-24) or just press Enter for ALL: ");
    fgets(wanted, sizeof(wanted), stdin);
    wanted[strcspn(wanted, "\n")] = '\0';   /* remove the Enter key */

    printf("\n===== PRODUCTION REPORT =====\n");
    while (fscanf(f, "%10s %d %d %lf", date, &p, &quantity, &cost) == 4) {
        if (wanted[0] == '\0' || strcmp(wanted, date) == 0) {
            printf("  %s  %-22s %5d   cost: %.0f RWF\n",
                   date, productNames[p], quantity, cost);
            totalQty[p] += quantity;
            totalCost[p] += cost;
            allCost += cost;
            found = 1;
        }
    }
    fclose(f);

    if (!found) {
        printf("  Nothing found for that date.\n");
        return;
    }

    printf("\n----- TOTALS -----\n");
    for (p = 0; p < NUM_PRODUCTS; p++)
        printf("  %-22s : %5d made, cost %.0f RWF\n",
               productNames[p], totalQty[p], totalCost[p]);
    printf("  TOTAL PRODUCTION COST   : %.0f RWF\n", allCost);
}

/* ------------------------------------------------------------------ */
/* 9. MAIN MENU                                                        */
/* ------------------------------------------------------------------ */

int main(void)
{
    int choice;

    loadData();

    do {
        printf("\n==============================\n");
        printf("   ALIMENTATION - KITCHEN\n");
        printf("==============================\n");
        printf("  1. Record production\n");
        printf("  2. Add materials bought\n");
        printf("  3. Show stock\n");
        printf("  4. Production report\n");
        printf("  0. Exit\n");
        choice = askChoice("Choose (number): ", 0, 4);

        if (choice == 1) recordProduction();
        else if (choice == 2) addMaterials();
        else if (choice == 3) showStock();
        else if (choice == 4) productionReport();

    } while (choice != 0);

    printf("Goodbye!\n");
    return 0;
}
