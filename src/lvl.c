#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <sqlite3.h>

#include "db.h"
#include "steam.h"

const gchar *selected_game_id = NULL;

static void mkdir_p(const char *dir, __mode_t permissions)
{
    char tmp[256] = {0};
    char *p = NULL;
    size_t len;

    snprintf(tmp, sizeof(tmp),"%s",dir);
    len = strlen(tmp);
    if (tmp[len - 1] == '/')
        tmp[len - 1] = 0;
    for (p = tmp + 1; *p; p++)
    {
        if (*p == '/') {
            *p = 0;
            mkdir(tmp, permissions);
            *p = '/';
        }
    }
    mkdir(tmp, permissions);
}

void get_config_path(char* out_path)
{
    struct passwd * pw = getpwuid(getuid());
    char* XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
    char* base_dir = strcat(pw->pw_dir, "/.config/lvl");
    if (XDG_CONFIG_HOME != NULL) {
        char config_dir[PATH_MAX] = {0};
        strcpy(config_dir, XDG_CONFIG_HOME);
        strcat(config_dir, "/lvl");
        strcpy(base_dir, config_dir);
    }
    mkdir_p(base_dir, 0700);
    strcpy(out_path, base_dir);
}

// Callback function for window destruction
void on_window_destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit(); // Quit the GTK main loop when the window is destroyed
}

// Callback function for button clicks
void on_button_clicked(GtkWidget *widget, gpointer data)
{
    GtkStack *stack = GTK_STACK(data);
    const gchar *page_name = gtk_button_get_label(GTK_BUTTON(widget));
    gtk_stack_set_visible_child_name(stack, page_name);
}

// Callback function for when a game is selected
void on_game_selected(GtkListBox *box, GtkListBoxRow *row, gpointer data)
{
    if (!row) return;
    gpointer game_id_ptr = g_object_get_data(G_OBJECT(row), "game_id");
    if (game_id_ptr != NULL) {
        int game_id = GPOINTER_TO_INT(game_id_ptr); // Convert back from pointer to integer
        printf("Selected game ID: %d\n", game_id);
        g_free((gchar*)selected_game_id);
        selected_game_id = g_strdup_printf("%d", game_id); // Store game ID as a string for other uses
    }
}

// Callback function for running a shell command with selected game
void on_run_command_clicked(GtkWidget *widget, gpointer data)
{
    if (selected_game_id != NULL) {
        char command[100];
        snprintf(command, sizeof(command), "steam -applaunch %s", selected_game_id);
        system(command);
    } else {
        g_print("No game selected!\n");
    }
}

void create_game_row(int id, const char *name, void *user_data) {
    GtkWidget *game_list_box = (GtkWidget *)user_data;
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(name);
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_list_box_insert(GTK_LIST_BOX(game_list_box), row, -1);
    g_object_set_data(G_OBJECT(row), "game_id", GINT_TO_POINTER(id));
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <API_KEY> <STEAM_ID>\n", argv[0]);
        return 1;
    }

    sqlite3 *db;
    char *zErr_msg = 0;
    int rc;

    char db_path[PATH_MAX];
    get_config_path(db_path);
    strcat(db_path, "/steam_games.db");
    rc = sqlite3_open(db_path, &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 0;
    } else {
        fprintf(stderr, "Opened database successfully\n");
    }

    create_table(db);

    run_python(argv[1], argv[2], db);
    // Initialize GTK
    gtk_init(&argc, &argv);

    // Create the main window
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "LVL 1");
    gtk_window_set_default_size(GTK_WINDOW(window), 400, 300);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);

    // Connect the destroy signal
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);

    // Create a grid to arrange widgets
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);

    // Create a stack to hold different pages
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_grid_attach(GTK_GRID(grid), stack, 0, 1, 1, 1);

    // Create pages
    GtkWidget *library_page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *page2 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *page3 = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    // Add pages to the stack
    gtk_stack_add_named(GTK_STACK(stack), library_page, "Library");
    gtk_stack_add_named(GTK_STACK(stack), page2, "Page 2");
    gtk_stack_add_named(GTK_STACK(stack), page3, "Page 3");

    // Create buttons for navigation
    GtkWidget *library_button = gtk_button_new_with_label("Library");
    g_signal_connect(library_button, "clicked", G_CALLBACK(on_button_clicked), stack);
    GtkWidget *button2 = gtk_button_new_with_label("Page 2");
    g_signal_connect(button2, "clicked", G_CALLBACK(on_button_clicked), stack);
    GtkWidget *button3 = gtk_button_new_with_label("Page 3");
    g_signal_connect(button3, "clicked", G_CALLBACK(on_button_clicked), stack);

    // Create a horizontal box to hold the navigation buttons
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    gtk_box_pack_start(GTK_BOX(hbox), library_button, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button2, TRUE, TRUE, 0);
    gtk_box_pack_start(GTK_BOX(hbox), button3, TRUE, TRUE, 0);
    gtk_grid_attach(GTK_GRID(grid), hbox, 0, 0, 1, 1); // Attach the box to the grid

    // Create a button for running a shell command
    GtkWidget *run_command_button = gtk_button_new_with_label("Play");
    g_signal_connect(run_command_button, "clicked", G_CALLBACK(on_run_command_clicked), NULL);

    // Create a vertical box to hold the game list
    GtkWidget *game_list_box = gtk_list_box_new();
    gtk_container_add(GTK_CONTAINER(library_page), game_list_box);

    db_fetch_games(db_path, create_game_row, game_list_box);

    // Connect the row-selected signal
    g_signal_connect(game_list_box, "row-selected", G_CALLBACK(on_game_selected), NULL);

    // Add the button to Page 1
    gtk_container_add(GTK_CONTAINER(library_page), run_command_button);

    // Show all widgets
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();

    return 0;
}
