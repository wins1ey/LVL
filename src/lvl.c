#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <sqlite3.h>

#include "db.h"
#include "steam.h"

typedef struct {
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *game_list_box;
} AppWidgets;

typedef struct {
    sqlite3 *db;
    char db_path[PATH_MAX];
} SteamDB;

const gchar *selected_game_id = NULL;

// Function to create necessary directories for the app configuration
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

// Retrieves or sets up the path to the configuration directory
void get_config_path(char *out_path)
{
    struct passwd *pw = getpwuid(getuid());
    const char *XDG_CONFIG_HOME = getenv("XDG_CONFIG_HOME");
    if (XDG_CONFIG_HOME) {
        strcpy(out_path, XDG_CONFIG_HOME);
        strcat(out_path, "/lvl");
    } else {
        strcpy(out_path, pw->pw_dir);
        strcat(out_path, "/.config/lvl");
    }
    mkdir_p(out_path, 0700);
}

// Callback to quit GTK main loop
void on_window_destroy(GtkWidget *widget, gpointer data)
{
    gtk_main_quit();
}

// Navigation button callback
void on_button_clicked(GtkWidget *widget, gpointer data)
{
    GtkStack *stack = GTK_STACK(data);
    const gchar *page_name = gtk_button_get_label(GTK_BUTTON(widget));
    gtk_stack_set_visible_child_name(stack, page_name);
}

// Game selection callback
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

// Execute a command for the selected game
void on_run_command_clicked(GtkWidget *widget, gpointer data)
{
    if (selected_game_id) {
        char command[100];
        snprintf(command, sizeof(command), "steam -applaunch %s", selected_game_id);
        system(command);
    } else {
        g_print("No game selected!\n");
    }
}

// Create a single row in the game list
void create_game_row(int id, const char *name, void *user_data)
{
    GtkWidget *game_list_box = (GtkWidget *)user_data;
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(name);

    gtk_label_set_xalign(GTK_LABEL(label), 0.0); // Align text to the left
    gtk_widget_set_hexpand(label, FALSE); // Do not expand horizontally
    gtk_widget_set_halign(label, GTK_ALIGN_FILL); // Fill the horizontal space without expanding
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END); // Ellipsize text at the end if it does not fit
                                                                    //
    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_list_box_insert(GTK_LIST_BOX(game_list_box), row, -1);
    g_object_set_data(G_OBJECT(row), "game_id", GINT_TO_POINTER(id));
}

// Initialize the database and load data
int init_database(SteamDB *steam, const char *api_key, const char *steam_id)
{
    get_config_path(steam->db_path);
    strcat(steam->db_path, "/steam_games.db");
    if (sqlite3_open(steam->db_path, &steam->db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(steam->db));
        return 0;
    }
    create_table(steam->db);
    fetch_data_from_steam_api(api_key, steam_id, steam->db);
    return 1;
}

// Initialize GTK and configure widgets
GtkWidget* create_main_window()
{
    GtkWidget *window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "LVL");
    gtk_window_set_default_size(GTK_WINDOW(window), 800, 600);
    gtk_container_set_border_width(GTK_CONTAINER(window), 10);
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), NULL);
    return window;
}

GtkWidget* create_navigation_buttons(GtkWidget *stack)
{
    GtkWidget *hbox = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 5);
    const char *labels[] = {"Library", "Page 2", "Page 3"};
    for (int i = 0; i < 3; i++) {
        GtkWidget *button = gtk_button_new_with_label(labels[i]);
        gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
        g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), stack);
    }
    return hbox;
}

GtkWidget* create_stack_with_pages()
{
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    const char *page_names[] = {"Library", "Page 2", "Page 3"};
    for (int i = 0; i < 3; i++) {
        GtkWidget *page = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
        gtk_stack_add_named(GTK_STACK(stack), page, page_names[i]);
    }
    return stack;
}

void setup_library_page(GtkWidget *library_page, GtkWidget *game_list_box)
{
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);
    gtk_scrolled_window_set_min_content_width(GTK_SCROLLED_WINDOW(scrolled_window), 600);
    gtk_container_add(GTK_CONTAINER(scrolled_window), game_list_box);
    gtk_paned_pack1(GTK_PANED(paned), scrolled_window, TRUE, TRUE);

    GtkWidget *play_button_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    GtkWidget *run_command_button = gtk_button_new_with_label("Play");
    gtk_box_pack_start(GTK_BOX(play_button_box), run_command_button, FALSE, FALSE, 0);
    gtk_paned_pack2(GTK_PANED(paned), play_button_box, FALSE, TRUE);

    gtk_box_pack_start(GTK_BOX(library_page), paned, TRUE, TRUE, 0);
    g_signal_connect(game_list_box, "row-selected", G_CALLBACK(on_game_selected), NULL);
    g_signal_connect(run_command_button, "clicked", G_CALLBACK(on_run_command_clicked), NULL);
}

int main(int argc, char *argv[])
{
    if (argc < 3) {
        fprintf(stderr, "Usage: %s <API_KEY> <STEAM_ID>\n", argv[0]);
        return 1;
    }

    SteamDB steam;
    if (!init_database(&steam, argv[1], argv[2])) {
        return 0;
    }
    gtk_init(&argc, &argv);
    GtkWidget *window = create_main_window();
    GtkWidget *stack = create_stack_with_pages();
    GtkWidget *hbox = create_navigation_buttons(stack);
    GtkWidget *library_page = gtk_stack_get_child_by_name(GTK_STACK(stack), "Library");
    GtkWidget *game_list_box = gtk_list_box_new();
    gtk_widget_set_vexpand(game_list_box, TRUE);
    setup_library_page(library_page, game_list_box);

    // Set up the grid and pack everything
    GtkWidget *grid = gtk_grid_new();
    gtk_container_add(GTK_CONTAINER(window), grid);
    gtk_grid_attach(GTK_GRID(grid), hbox, 0, 0, 1, 1);
    gtk_grid_attach(GTK_GRID(grid), stack, 0, 1, 1, 1);

    db_fetch_games(steam.db_path, create_game_row, game_list_box);

    gtk_widget_show_all(window);
    gtk_main();

    sqlite3_close(steam.db);

    return 0;
}
