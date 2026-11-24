/*
 * ALIMENTATION SYSTEM - KITCHEN (GRAPHIC INTERFACE, GTK 3)
 *
 * 4 tabs:
 *   1. Production      -> chef records what he made + materials used
 *   2. Materials bought -> add flour, oil, ... to stock (with price)
 *   3. Stock           -> products and materials in stock
 *   4. Report          -> production history with cost
 *
 * Uses the same data files as the text version:
 *   data.txt, production.txt, purchases.txt
 *
 * Compile (MSYS2 MINGW64):
 *   gcc kitchen_gui.c -o kitchen_gui.exe `pkg-config --cflags --libs gtk+-3.0` -mwindows
 */

#include <gtk/gtk.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#ifdef _WIN32
#include <windows.h>
#endif

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
/* 2. DATA kept in memory                                              */
/* ------------------------------------------------------------------ */

int    productStock[NUM_PRODUCTS];
double materialStock[NUM_MATERIALS];
double materialPrice[NUM_MATERIALS];

/* ------------------------------------------------------------------ */
/* 3. WIDGETS (things we need to reach from many functions)            */
/* ------------------------------------------------------------------ */

GtkWidget *window;

/* production tab */
GtkWidget *prodCombo;
GtkWidget *prodQtyLabel;
GtkWidget *prodQtyEntry;
GtkWidget *prodUsedLabel[NUM_MATERIALS];
GtkWidget *prodUsedEntry[NUM_MATERIALS];

/* materials bought tab */
GtkWidget *matCombo;
GtkWidget *matQtyLabel;
GtkWidget *matQtyEntry;
GtkWidget *matPriceLabel;
GtkWidget *matPriceEntry;

/* stock and report tabs */
GtkWidget *stockView;
GtkWidget *reportView;
GtkWidget *reportDateEntry;

/* ------------------------------------------------------------------ */
/* 4. SMALL HELPER FUNCTIONS                                           */
/* ------------------------------------------------------------------ */

/* Show a message box */
void showMessage(GtkMessageType type, const char *text)
{
    GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(window),
                                               GTK_DIALOG_MODAL, type,
                                               GTK_BUTTONS_OK, "%s", text);
    gtk_dialog_run(GTK_DIALOG(dialog));
    gtk_widget_destroy(dialog);
}

/* Read a number (0 or more) from an entry box. Returns 1 if OK.       */
int readNumber(GtkWidget *entry, double *value)
{
    char text[100];
    char *end;
    char *c;

    strncpy(text, gtk_entry_get_text(GTK_ENTRY(entry)), 99);
    text[99] = '\0';

    for (c = text; *c != '\0'; c++)     /* accept 1,5 like 1.5 */
        if (*c == ',') *c = '.';

    if (text[0] == '\0') return 0;
    *value = strtod(text, &end);
    if (*end != '\0' || *value < 0) return 0;
    return 1;
}

/* Put text inside a text view */
void setText(GtkWidget *view, const char *text)
{
    GtkTextBuffer *buffer = gtk_text_view_get_buffer(GTK_TEXT_VIEW(view));
    gtk_text_buffer_set_text(buffer, text, -1);
}

/* Today's date like 2026-11-24 */
void getToday(char *text)
{
    time_t now = time(NULL);
    strftime(text, 11, "%Y-%m-%d", localtime(&now));
}

/* A text view that the user can read but not edit (monospace font) */
GtkWidget *makeReadOnlyView(void)
{
    GtkWidget *view = gtk_text_view_new();
    gtk_text_view_set_editable(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_cursor_visible(GTK_TEXT_VIEW(view), FALSE);
    gtk_text_view_set_monospace(GTK_TEXT_VIEW(view), TRUE);
    gtk_text_view_set_left_margin(GTK_TEXT_VIEW(view), 10);
    gtk_text_view_set_top_margin(GTK_TEXT_VIEW(view), 10);
    return view;
}

/* ------------------------------------------------------------------ */
/* 5. SAVE AND LOAD                                                    */
/* ------------------------------------------------------------------ */

void saveData(void)
{
    FILE *f = fopen("data.txt", "w");
    int i;

    if (f == NULL) {
        showMessage(GTK_MESSAGE_ERROR, "ERROR: cannot save data!");
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
/* 6. TAB 3: STOCK                                                     */
/* ------------------------------------------------------------------ */

void refreshStock(void)
{
    GString *text = g_string_new("");
    int i;

    g_string_append(text, "PRODUCTS IN STOCK\n");
    g_string_append(text, "-----------------\n");
    for (i = 0; i < NUM_PRODUCTS; i++)
        g_string_append_printf(text, "%-22s : %d\n",
                               productNames[i], productStock[i]);

    g_string_append(text, "\nMATERIALS IN STOCK\n");
    g_string_append(text, "------------------\n");
    for (i = 0; i < NUM_MATERIALS; i++)
        g_string_append_printf(text, "%-22s : %.2f %s  (%.0f RWF per %s)\n",
                               materialNames[i], materialStock[i],
                               materialUnits[i], materialPrice[i],
                               materialUnits[i]);

    setText(stockView, text->str);
    g_string_free(text, TRUE);
}

/* when the user opens the Stock tab, show fresh numbers */
void on_tab_changed(GtkNotebook *notebook, GtkWidget *page,
                    guint pageNumber, gpointer data)
{
    if (pageNumber == 2) refreshStock();
}

/* ------------------------------------------------------------------ */
/* 7. TAB 1: PRODUCTION                                                */
/* ------------------------------------------------------------------ */

/* When the product changes: show only the materials it uses */
void on_product_changed(GtkComboBox *combo, gpointer data)
{
    int p = gtk_combo_box_get_active(GTK_COMBO_BOX(prodCombo));
    int m;

    if (p < 0) return;

    for (m = 0; m < NUM_MATERIALS; m++) {
        gtk_widget_set_visible(prodUsedLabel[m], uses[p][m]);
        gtk_widget_set_visible(prodUsedEntry[m], uses[p][m]);
        gtk_entry_set_text(GTK_ENTRY(prodUsedEntry[m]), "");
    }

    if (p == 3)
        gtk_label_set_text(GTK_LABEL(prodQtyLabel),
                           "Number of PACKS made (6 pieces each):");
    else
        gtk_label_set_text(GTK_LABEL(prodQtyLabel), "Number of pieces made:");
}

void on_save_production(GtkButton *button, gpointer data)
{
    int p = gtk_combo_box_get_active(GTK_COMBO_BOX(prodCombo));
    int m, quantity;
    double q, cost = 0;
    double used[NUM_MATERIALS];
    char date[11], message[300];
    FILE *f;

    if (p < 0) {
        showMessage(GTK_MESSAGE_WARNING, "Please choose the product.");
        return;
    }

    if (!readNumber(prodQtyEntry, &q) || q < 1) {
        showMessage(GTK_MESSAGE_WARNING,
                    "Type how many were made (a number bigger than 0).");
        return;
    }
    quantity = (int)q;

    /* read the materials used by this product */
    for (m = 0; m < NUM_MATERIALS; m++) {
        used[m] = 0;
        if (uses[p][m] == 1) {
            if (!readNumber(prodUsedEntry[m], &used[m])) {
                sprintf(message, "Type the amount of %s used (type 0 if none).",
                        materialNames[m]);
                showMessage(GTK_MESSAGE_WARNING, message);
                return;
            }
        }
    }

    /* check we have enough materials */
    for (m = 0; m < NUM_MATERIALS; m++) {
        if (used[m] > materialStock[m]) {
            sprintf(message,
                    "Not enough %s in stock (you have %.2f %s).\n"
                    "Nothing was saved. Add materials first (tab 2).",
                    materialNames[m], materialStock[m], materialUnits[m]);
            showMessage(GTK_MESSAGE_ERROR, message);
            return;
        }
    }

    /* update stock and calculate cost */
    for (m = 0; m < NUM_MATERIALS; m++) {
        materialStock[m] -= used[m];
        cost += used[m] * materialPrice[m];
    }
    productStock[p] += quantity;

    /* write history */
    getToday(date);
    f = fopen("production.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s %d %d %.2f\n", date, p, quantity, cost);
        fclose(f);
    }
    saveData();

    /* clear the form */
    gtk_entry_set_text(GTK_ENTRY(prodQtyEntry), "");
    for (m = 0; m < NUM_MATERIALS; m++)
        gtk_entry_set_text(GTK_ENTRY(prodUsedEntry[m]), "");

    sprintf(message, "Saved!\n%d %s recorded.\nCost: %.0f RWF (%.0f RWF per item)",
            quantity, productNames[p], cost, cost / quantity);
    showMessage(GTK_MESSAGE_INFO, message);
}

GtkWidget *buildProductionTab(void)
{
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *label, *button;
    char text[100];
    int p, m;

    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

    /* row 0: product */
    label = gtk_label_new("What did you make?");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    prodCombo = gtk_combo_box_text_new();
    for (p = 0; p < NUM_PRODUCTS; p++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(prodCombo), productNames[p]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(prodCombo), 0);
    gtk_widget_set_hexpand(prodCombo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), prodCombo, 1, 0, 1, 1);

    /* row 1: quantity */
    prodQtyLabel = gtk_label_new("Number of pieces made:");
    gtk_widget_set_halign(prodQtyLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), prodQtyLabel, 0, 1, 1, 1);

    prodQtyEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), prodQtyEntry, 1, 1, 1, 1);

    /* rows 2..8: materials used */
    for (m = 0; m < NUM_MATERIALS; m++) {
        sprintf(text, "%s used (%s):", materialNames[m], materialUnits[m]);
        prodUsedLabel[m] = gtk_label_new(text);
        gtk_widget_set_halign(prodUsedLabel[m], GTK_ALIGN_START);
        gtk_grid_attach(GTK_GRID(grid), prodUsedLabel[m], 0, 2 + m, 1, 1);

        prodUsedEntry[m] = gtk_entry_new();
        gtk_grid_attach(GTK_GRID(grid), prodUsedEntry[m], 1, 2 + m, 1, 1);
    }

    /* last row: save button */
    button = gtk_button_new_with_label("SAVE PRODUCTION");
    gtk_widget_set_size_request(button, -1, 45);
    g_signal_connect(button, "clicked", G_CALLBACK(on_save_production), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 2 + NUM_MATERIALS, 2, 1);

    g_signal_connect(prodCombo, "changed", G_CALLBACK(on_product_changed), NULL);

    return grid;
}

/* ------------------------------------------------------------------ */
/* 8. TAB 2: MATERIALS BOUGHT                                          */
/* ------------------------------------------------------------------ */

void on_material_changed(GtkComboBox *combo, gpointer data)
{
    int m = gtk_combo_box_get_active(GTK_COMBO_BOX(matCombo));
    char text[100];

    if (m < 0) return;

    sprintf(text, "How many %s did you buy?", materialUnits[m]);
    gtk_label_set_text(GTK_LABEL(matQtyLabel), text);

    sprintf(text, "Price of ONE %s (RWF):", materialUnits[m]);
    gtk_label_set_text(GTK_LABEL(matPriceLabel), text);
}

void on_add_material(GtkButton *button, gpointer data)
{
    int m = gtk_combo_box_get_active(GTK_COMBO_BOX(matCombo));
    double quantity, price;
    char date[11], message[200];
    FILE *f;

    if (m < 0) {
        showMessage(GTK_MESSAGE_WARNING, "Please choose the material.");
        return;
    }
    if (!readNumber(matQtyEntry, &quantity) || quantity <= 0) {
        showMessage(GTK_MESSAGE_WARNING, "Type the quantity bought (bigger than 0).");
        return;
    }
    if (!readNumber(matPriceEntry, &price)) {
        showMessage(GTK_MESSAGE_WARNING, "Type the price of one unit.");
        return;
    }

    materialStock[m] += quantity;
    materialPrice[m] = price;      /* keep the latest price */

    getToday(date);
    f = fopen("purchases.txt", "a");
    if (f != NULL) {
        fprintf(f, "%s %d %.2f %.2f\n", date, m, quantity, price);
        fclose(f);
    }
    saveData();

    gtk_entry_set_text(GTK_ENTRY(matQtyEntry), "");
    gtk_entry_set_text(GTK_ENTRY(matPriceEntry), "");

    sprintf(message, "Saved!\n%s stock is now %.2f %s.",
            materialNames[m], materialStock[m], materialUnits[m]);
    showMessage(GTK_MESSAGE_INFO, message);
}

GtkWidget *buildMaterialsTab(void)
{
    GtkWidget *grid = gtk_grid_new();
    GtkWidget *label, *button;
    int m;

    gtk_container_set_border_width(GTK_CONTAINER(grid), 15);
    gtk_grid_set_row_spacing(GTK_GRID(grid), 10);
    gtk_grid_set_column_spacing(GTK_GRID(grid), 15);

    label = gtk_label_new("Which material did you buy?");
    gtk_widget_set_halign(label, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), label, 0, 0, 1, 1);

    matCombo = gtk_combo_box_text_new();
    for (m = 0; m < NUM_MATERIALS; m++)
        gtk_combo_box_text_append_text(GTK_COMBO_BOX_TEXT(matCombo), materialNames[m]);
    gtk_combo_box_set_active(GTK_COMBO_BOX(matCombo), 0);
    gtk_widget_set_hexpand(matCombo, TRUE);
    gtk_grid_attach(GTK_GRID(grid), matCombo, 1, 0, 1, 1);

    matQtyLabel = gtk_label_new("");
    gtk_widget_set_halign(matQtyLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), matQtyLabel, 0, 1, 1, 1);
    matQtyEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), matQtyEntry, 1, 1, 1, 1);

    matPriceLabel = gtk_label_new("");
    gtk_widget_set_halign(matPriceLabel, GTK_ALIGN_START);
    gtk_grid_attach(GTK_GRID(grid), matPriceLabel, 0, 2, 1, 1);
    matPriceEntry = gtk_entry_new();
    gtk_grid_attach(GTK_GRID(grid), matPriceEntry, 1, 2, 1, 1);

    button = gtk_button_new_with_label("ADD TO STOCK");
    gtk_widget_set_size_request(button, -1, 45);
    g_signal_connect(button, "clicked", G_CALLBACK(on_add_material), NULL);
    gtk_grid_attach(GTK_GRID(grid), button, 0, 3, 2, 1);

    g_signal_connect(matCombo, "changed", G_CALLBACK(on_material_changed), NULL);
    on_material_changed(NULL, NULL);   /* set the first labels */

    return grid;
}

/* ------------------------------------------------------------------ */
/* 9. TAB 4: REPORT                                                    */
/* ------------------------------------------------------------------ */

void on_show_report(GtkButton *button, gpointer data)
{
    FILE *f = fopen("production.txt", "r");
    GString *text = g_string_new("");
    char date[11], wanted[100];
    int p, quantity, found = 0;
    double cost, allCost = 0;
    int totalQty[NUM_PRODUCTS] = {0};
    double totalCost[NUM_PRODUCTS] = {0};

    if (f == NULL) {
        setText(reportView, "No production recorded yet.");
        g_string_free(text, TRUE);
        return;
    }

    strncpy(wanted, gtk_entry_get_text(GTK_ENTRY(reportDateEntry)), 99);
    wanted[99] = '\0';

    g_string_append(text, "PRODUCTION REPORT\n");
    g_string_append(text, "-----------------\n");

    while (fscanf(f, "%10s %d %d %lf", date, &p, &quantity, &cost) == 4) {
        if (wanted[0] == '\0' || strcmp(wanted, date) == 0) {
            g_string_append_printf(text, "%s  %-22s %5d   cost: %.0f RWF\n",
                                   date, productNames[p], quantity, cost);
            totalQty[p] += quantity;
            totalCost[p] += cost;
            allCost += cost;
            found = 1;
        }
    }
    fclose(f);

    if (!found) {
        g_string_append(text, "Nothing found for that date.\n");
    } else {
        g_string_append(text, "\nTOTALS\n------\n");
        for (p = 0; p < NUM_PRODUCTS; p++)
            g_string_append_printf(text, "%-22s : %5d made, cost %.0f RWF\n",
                                   productNames[p], totalQty[p], totalCost[p]);
        g_string_append_printf(text, "\nTOTAL PRODUCTION COST : %.0f RWF\n", allCost);
    }

    setText(reportView, text->str);
    g_string_free(text, TRUE);
}

GtkWidget *buildReportTab(void)
{
    GtkWidget *box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    GtkWidget *row = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 10);
    GtkWidget *label, *button, *scroll;

    gtk_container_set_border_width(GTK_CONTAINER(box), 15);

    label = gtk_label_new("Date (example 2026-11-24), empty = all:");
    reportDateEntry = gtk_entry_new();
    gtk_widget_set_hexpand(reportDateEntry, TRUE);
    button = gtk_button_new_with_label("SHOW REPORT");
    g_signal_connect(button, "clicked", G_CALLBACK(on_show_report), NULL);

    gtk_box_pack_start(GTK_BOX(row), label, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(row), reportDateEntry, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(row), button, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(box), row, FALSE, FALSE, 0);

    reportView = makeReadOnlyView();
    scroll = gtk_scrolled_window_new(NULL, NULL);
    gtk_container_add(GTK_CONTAINER(scroll), reportView);
    gtk_box_pack_start(GTK_BOX(box), scroll, TRUE, TRUE, 0);

    return box;
}

GtkWidget *buildStockTab(void)
{
    GtkWidget *scroll = gtk_scrolled_window_new(NULL, NULL);

    stockView = makeReadOnlyView();
    gtk_container_add(GTK_CONTAINER(scroll), stockView);
    return scroll;
}

/* ------------------------------------------------------------------ */
/* 10. MAIN                                                            */
/* ------------------------------------------------------------------ */

int main(int argc, char *argv[])
{
    GtkWidget *notebook;

#ifdef _WIN32
    /* Always work in the folder of the .exe, so data files are found */
    {
        char path[MAX_PATH];
        char *slash;
        GetModuleFileNameA(NULL, path, MAX_PATH);
        slash = strrchr(path, '\\');
        if (slash != NULL) {
            *slash = '\0';
            SetCurrentDirectoryA(path);
        }
    }
#endif

    gtk_init(&argc, &argv);
    loadData();

    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Alimentation - Kitchen System");
    gtk_window_set_default_size(GTK_WINDOW(window), 620, 580);
    gtk_window_set_position(GTK_WINDOW(window), GTK_WIN_POS_CENTER);
    g_signal_connect(window, "destroy", G_CALLBACK(gtk_main_quit), NULL);

    notebook = gtk_notebook_new();
    gtk_container_add(GTK_CONTAINER(window), notebook);

    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buildProductionTab(),
                             gtk_label_new("1. Production"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buildMaterialsTab(),
                             gtk_label_new("2. Materials bought"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buildStockTab(),
                             gtk_label_new("3. Stock"));
    gtk_notebook_append_page(GTK_NOTEBOOK(notebook), buildReportTab(),
                             gtk_label_new("4. Report"));

    g_signal_connect(notebook, "switch-page", G_CALLBACK(on_tab_changed), NULL);

    refreshStock();
    gtk_widget_show_all(window);
    on_product_changed(NULL, NULL);   /* hide materials the first product does not use */

    gtk_main();
    return 0;
}
