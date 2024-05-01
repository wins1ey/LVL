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
    GtkWidget *game_info_label;
    GtkWidget *run_command_button;
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

void open_uri(const char *action) {
    char command[256];

    snprintf(command, sizeof(command), "xdg-open %s", action);

    printf("Executing command: %s\n", command);
    if (system(command) == -1) {
        fprintf(stderr, "Failed to execute command\n");
    }
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
        char command[28];
        snprintf(command, sizeof(command), "steam://rungameid/%s", selected_game_id);
        open_uri(command);
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
    const char *labels[] = {"Library", "Settings", "About"};
    for (int i = 0; i < sizeof(labels) / sizeof(labels[0]); i++) {
        GtkWidget *button = gtk_button_new_with_label(labels[i]);
        gtk_box_pack_start(GTK_BOX(hbox), button, TRUE, TRUE, 0);
        g_signal_connect(button, "clicked", G_CALLBACK(on_button_clicked), stack);
    }
    return hbox;
}

GtkWidget* create_library_page(GtkWidget *stack, AppWidgets *appWidgets)
{
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    appWidgets->game_list_box = gtk_list_box_new();
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), appWidgets->game_list_box);
    gtk_paned_add1(GTK_PANED(paned), scrolled_window);

    GtkWidget *info_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    appWidgets->game_info_label = gtk_label_new("Select a game from the list.");
    gtk_box_pack_start(GTK_BOX(info_vbox), appWidgets->game_info_label, TRUE, TRUE, 0);
    appWidgets->run_command_button = gtk_button_new_with_label("Play");
    gtk_box_pack_start(GTK_BOX(info_vbox), appWidgets->run_command_button, FALSE, FALSE, 0);
    gtk_paned_add2(GTK_PANED(paned), info_vbox);

    g_signal_connect(appWidgets->game_list_box, "row-selected", G_CALLBACK(on_game_selected), appWidgets);
    g_signal_connect(appWidgets->run_command_button, "clicked", G_CALLBACK(on_run_command_clicked), NULL);

    return paned;
}

GtkWidget* create_stack_with_pages(AppWidgets *appWidgets)
{
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 500);

    GtkWidget *library_page = create_library_page(stack, appWidgets);
    gtk_stack_add_named(GTK_STACK(stack), library_page, "Library");

    GtkWidget *settings_page = gtk_label_new("Settings page content here.");
    gtk_stack_add_named(GTK_STACK(stack), settings_page, "Settings");

    GtkWidget *about_page = gtk_label_new("About page content here.");
    gtk_stack_add_named(GTK_STACK(stack), about_page, "About");

    appWidgets->stack = stack;
    return stack;
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

    AppWidgets appWidgets;
    appWidgets.window = create_main_window();
    GtkWidget *stack = create_stack_with_pages(&appWidgets);
    GtkWidget *hbox = create_navigation_buttons(stack);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), stack, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(appWidgets.window), vbox);

    db_fetch_games(steam.db_path, create_game_row, appWidgets.game_list_box);

    gtk_widget_show_all(appWidgets.window);
    gtk_main();

    sqlite3_close(steam.db);

    return 0;
}
