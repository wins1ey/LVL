#include <stdlib.h>
#include <pwd.h>
#include <string.h>
#include <sys/stat.h>

#include <gtk/gtk.h>
#include <sqlite3.h>

#include "db.h"
#include "steam.h"
#include "validation.h"

#define VERSION "1.0.0"

typedef struct {
    GtkWidget *window;
    GtkWidget *stack;
    GtkWidget *game_list_box;
    GtkWidget *game_info_label;
    GtkWidget *game_title_label;
    GtkWidget *playtime_label;
    GtkWidget *run_command_button;
    GtkWidget *api_key_entry;
    GtkWidget *steam_id_entry;
    GtkWidget *game_name_entry;
    GtkWidget *install_path_entry;
    GtkWidget *playtime_entry;
    GtkListBoxRow *selected_game_row;
} AppWidgets;

typedef struct {
    sqlite3 *db;
    char db_path[PATH_MAX];
} Steam;

Steam steam;

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

void open_uri(const char *action)
{
    char command[256];

    snprintf(command, sizeof(command), "xdg-open %s", action);

    printf("Executing command: %s\n", command);
    if (system(command) == -1) {
        fprintf(stderr, "Failed to execute command\n");
    }
}

// Function to write the Steam API key and Steam ID to the configuration file
void write_config(const char *config_path, const char *api_key, const char *steam_id)
{
    FILE *file = fopen(config_path, "w");
    if (file) {
        fprintf(file, "%s\n%s\n", api_key, steam_id);
        fclose(file);
    }
}

// Function to read the Steam API key and Steam ID from the configuration file
int read_config(const char *config_path, char *api_key, char *steam_id)
{
    FILE *file = fopen(config_path, "r");
    if (file) {
        if (fscanf(file, "%255s\n%255s\n", api_key, steam_id) == 2) {
            fclose(file);
            return 1;
        }
        fclose(file);
    }
    return 0;
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

// Initialize the database and load data
int init_database(Steam *steam)
{
    get_config_path(steam->db_path);
    strcat(steam->db_path, "/steam_games.db");
    if (sqlite3_open(steam->db_path, &steam->db)) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(steam->db));
        return 0;
    }
    create_table(steam->db);
    create_non_steam_table(steam->db);

    return 1;
}

// Create a single row in the game list
void create_game_row(int id, const char *name, const char *install_path, int playtime, void *user_data)
{
    GtkWidget *game_list_box = (GtkWidget *)user_data;
    GtkWidget *row = gtk_list_box_row_new();
    GtkWidget *label = gtk_label_new(name);

    gtk_label_set_xalign(GTK_LABEL(label), 0.0); // Align text to the left
    gtk_widget_set_hexpand(label, FALSE); // Do not expand horizontally
    gtk_widget_set_halign(label, GTK_ALIGN_FILL); // Fill the horizontal space without expanding
    gtk_label_set_ellipsize(GTK_LABEL(label), PANGO_ELLIPSIZE_END); // Ellipsize text at the end if it does not fit

    gtk_container_add(GTK_CONTAINER(row), label);
    gtk_list_box_insert(GTK_LIST_BOX(game_list_box), row, -1);
    g_object_set_data(G_OBJECT(row), "game_id", GINT_TO_POINTER(id));
    g_object_set_data(G_OBJECT(row), "playtime", GINT_TO_POINTER(playtime));
    g_object_set_data_full(G_OBJECT(row), "install_path", g_strdup(install_path), g_free);
}

void clear_game_list(GtkWidget *game_list_box) {
    gtk_container_foreach(GTK_CONTAINER(game_list_box), (GtkCallback)gtk_widget_destroy, NULL);
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
    AppWidgets *widgets = (AppWidgets *)data;
    gpointer game_id_ptr = g_object_get_data(G_OBJECT(row), "game_id");
    gpointer playtime_ptr = g_object_get_data(G_OBJECT(row), "playtime");

    if (game_id_ptr != NULL) {
        int game_id = GPOINTER_TO_INT(game_id_ptr); // Convert back from pointer to integer
        int playtime = GPOINTER_TO_INT(playtime_ptr);
        g_print("Selected game ID: %d\n", game_id);
        g_free((gchar*)selected_game_id);
        selected_game_id = g_strdup_printf("%d", game_id); // Store game ID as a string for other uses

        const char *game_title_text = gtk_label_get_text(GTK_LABEL(gtk_bin_get_child(GTK_BIN(row))));
        char *formatted_title = g_markup_printf_escaped("<span font='16'>%s</span>", game_title_text);
        gtk_label_set_markup(GTK_LABEL(widgets->game_title_label), formatted_title);
        g_free(formatted_title);

        char *formatted_playtime = g_strdup_printf("Playtime: %d minutes", playtime);
        gtk_label_set_text(GTK_LABEL(widgets->playtime_label), formatted_playtime);
        g_free(formatted_playtime);
    }
}

// Execute a command for the selected game
void on_run_command_clicked(GtkWidget *widget, gpointer data)
{
    AppWidgets *widgets = (AppWidgets *)data;
    GtkListBoxRow *selected_row = gtk_list_box_get_selected_row(GTK_LIST_BOX(widgets->game_list_box));

    if (selected_row) {
        int game_id = GPOINTER_TO_INT(g_object_get_data(G_OBJECT(selected_row), "game_id"));
        char *install_path_ptr = g_object_get_data(G_OBJECT(selected_row), "install_path");

        if (install_path_ptr && strlen(install_path_ptr) > 0) {
            // Non-Steam game with a path
            g_print("Running non-Steam game with command: %s\n", install_path_ptr);
            system(install_path_ptr);
        } else if (game_id) {
            // It's a Steam game, execute the Steam URI
            char command[256];
            snprintf(command, sizeof(command), "steam://rungameid/%d", game_id);
            g_print("Opening Steam game with ID: %d\n", game_id);
            open_uri(command);
        } else {
            g_print("No valid command or game ID found!\n");
        }
    } else {
        g_print("No game selected!\n");
    }
}

void on_save_settings_clicked(GtkWidget *widget, gpointer data)
{
    AppWidgets *widgets = (AppWidgets *)data;
    const char *api_key = gtk_entry_get_text(GTK_ENTRY(widgets->api_key_entry));
    const char *steam_id = gtk_entry_get_text(GTK_ENTRY(widgets->steam_id_entry));

    if (!validate_steam_credentials(api_key, steam_id)) {
        // If validation fails, show an error message
        GtkWidget *dialog = gtk_message_dialog_new(GTK_WINDOW(widgets->window),
                                                   GTK_DIALOG_DESTROY_WITH_PARENT,
                                                   GTK_MESSAGE_ERROR,
                                                   GTK_BUTTONS_CLOSE,
                                                   "Invalid Steam API Key or Steam ID. Please check your input.");
        gtk_dialog_run(GTK_DIALOG(dialog));
        gtk_widget_destroy(dialog);
        return; // Stop further processing
    }

    char config_path[PATH_MAX];
    get_config_path(config_path);
    strcat(config_path, "/config.txt");

    write_config(config_path, api_key, steam_id);

    fetch_data_from_steam_api(api_key, steam_id, steam.db);

    // Clear the existing list of games
    clear_game_list(widgets->game_list_box);

    // Fetch games with new API key and Steam ID and repopulate the list
    db_fetch_all_games(steam.db_path, create_game_row, widgets->game_list_box);
    gtk_widget_show_all(widgets->window);
}

void on_add_game_clicked(GtkWidget *widget, gpointer data)
{
    AppWidgets *widgets = (AppWidgets *)data;
    const char *game_name = gtk_entry_get_text(GTK_ENTRY(widgets->game_name_entry));
    const char *install_path = gtk_entry_get_text(GTK_ENTRY(widgets->install_path_entry));
    const char *playtime_str = gtk_entry_get_text(GTK_ENTRY(widgets->playtime_entry));

    // Validate inputs (simplified check here, should be more robust)
    if (!game_name || !*game_name || !install_path || !*install_path || !playtime_str || !*playtime_str) {
        fprintf(stderr, "Invalid input\n");
        return;
    }

    int playtime = atoi(playtime_str);

    // Call to insert into database
    insert_non_steam_game(steam.db, game_name, install_path, playtime);

    printf("added game: %s\n", game_name);

    // Clear the existing list of games
    clear_game_list(widgets->game_list_box);

    printf("cleared game list\n");

    db_fetch_all_games(steam.db_path, create_game_row, widgets->game_list_box);
    printf("fetched games\n");
    gtk_widget_show_all(widgets->game_list_box);
    printf("showed games\n");
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

GtkWidget* create_about_page()
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_container_set_border_width(GTK_CONTAINER(vbox), 10);

    // Title
    GtkWidget *title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(title_label), "<span font='18' weight='bold'>LVL - Linux Video game Launcher</span>");
    gtk_box_pack_start(GTK_BOX(vbox), title_label, FALSE, FALSE, 0);

    // Version
    char version_text[32], major_version[8];
    const char *dot_position = strchr(VERSION, '.');
    strcpy(major_version, VERSION);
    major_version[dot_position - VERSION] = '\0';
    sprintf(version_text, "Version: %s (LVL %s)", VERSION, major_version);
    GtkWidget *version_label = gtk_label_new(version_text);
    gtk_box_pack_start(GTK_BOX(vbox), version_label, FALSE, FALSE, 0);

    // Description
    GtkWidget *description_label = gtk_label_new(
        "LVL is designed to manage and launch your video games on Linux "
        "effortlessly. It integrates with the Steam API to fetch your game "
        "library and allows you to launch games directly from the application."
    );
    gtk_label_set_line_wrap(GTK_LABEL(description_label), TRUE);
    gtk_box_pack_start(GTK_BOX(vbox), description_label, FALSE, FALSE, 0);

    // Author
    GtkWidget *author_label = gtk_label_new("Developed by: wins1ey");
    gtk_box_pack_start(GTK_BOX(vbox), author_label, FALSE, FALSE, 0);

    return vbox;
}

GtkWidget* create_settings_page(AppWidgets *appWidgets)
{
    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);

    GtkWidget *api_key_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(api_key_entry), "Enter Steam API Key");
    gtk_box_pack_start(GTK_BOX(vbox), api_key_entry, FALSE, FALSE, 0);

    GtkWidget *steam_id_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(steam_id_entry), "Enter Steam ID");
    gtk_box_pack_start(GTK_BOX(vbox), steam_id_entry, FALSE, FALSE, 0);

    GtkWidget *save_button = gtk_button_new_with_label("Save Steam Settings");
    gtk_box_pack_start(GTK_BOX(vbox), save_button, FALSE, FALSE, 0);

    // Save callback
    g_signal_connect(save_button, "clicked", G_CALLBACK(on_save_settings_clicked), appWidgets);

    appWidgets->api_key_entry = api_key_entry;
    appWidgets->steam_id_entry = steam_id_entry;

    // Entry for game name
    GtkWidget *game_name_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(game_name_entry), "Enter Game Name");
    gtk_box_pack_start(GTK_BOX(vbox), game_name_entry, FALSE, FALSE, 0);

    // Entry for install path
    GtkWidget *install_path_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(install_path_entry), "Enter Executable Path");
    gtk_box_pack_start(GTK_BOX(vbox), install_path_entry, FALSE, FALSE, 0);

    // Entry for playtime
    GtkWidget *playtime_entry = gtk_entry_new();
    gtk_entry_set_placeholder_text(GTK_ENTRY(playtime_entry), "Enter Playtime in minutes");
    gtk_box_pack_start(GTK_BOX(vbox), playtime_entry, FALSE, FALSE, 0);

    // Button to add game
    GtkWidget *add_game_button = gtk_button_new_with_label("Add Game");
    gtk_box_pack_start(GTK_BOX(vbox), add_game_button, FALSE, FALSE, 0);

    // Connect signals
    g_signal_connect(add_game_button, "clicked", G_CALLBACK(on_add_game_clicked), appWidgets);

    appWidgets->game_name_entry = game_name_entry;
    appWidgets->install_path_entry = install_path_entry;
    appWidgets->playtime_entry = playtime_entry;

    return vbox;
}

GtkWidget* create_library_page(AppWidgets *appWidgets)
{
    GtkWidget *paned = gtk_paned_new(GTK_ORIENTATION_HORIZONTAL);
    appWidgets->game_list_box = gtk_list_box_new();
    GtkWidget *scrolled_window = gtk_scrolled_window_new(NULL, NULL);
    gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scrolled_window), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    gtk_container_add(GTK_CONTAINER(scrolled_window), appWidgets->game_list_box);
    gtk_paned_add1(GTK_PANED(paned), scrolled_window);

    GtkWidget *info_vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 5);
    gtk_container_set_border_width(GTK_CONTAINER(info_vbox), 10);

    // Game information labels
    appWidgets->game_title_label = gtk_label_new(NULL);
    gtk_label_set_markup(GTK_LABEL(appWidgets->game_title_label), "<span font='16'>Select a game</span>");
    gtk_box_pack_start(GTK_BOX(info_vbox), appWidgets->game_title_label, FALSE, FALSE, 0);
    appWidgets->playtime_label = gtk_label_new(NULL);
    gtk_box_pack_start(GTK_BOX(info_vbox), appWidgets->playtime_label, FALSE, FALSE, 0);

    // Spacer to push the button to the bottom
    GtkWidget *spacer = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(info_vbox), spacer, TRUE, TRUE, 0);

    // Play button
    appWidgets->run_command_button = gtk_button_new_with_label("Play");
    gtk_box_pack_end(GTK_BOX(info_vbox), appWidgets->run_command_button, FALSE, FALSE, 0);

    gtk_paned_add2(GTK_PANED(paned), info_vbox);

    g_signal_connect(appWidgets->game_list_box, "row-selected", G_CALLBACK(on_game_selected), appWidgets);
    g_signal_connect(appWidgets->run_command_button, "clicked", G_CALLBACK(on_run_command_clicked), appWidgets);

    return paned;
}

GtkWidget* create_stack_with_pages(AppWidgets *appWidgets)
{
    GtkWidget *stack = gtk_stack_new();
    gtk_stack_set_transition_type(GTK_STACK(stack), GTK_STACK_TRANSITION_TYPE_SLIDE_LEFT_RIGHT);
    gtk_stack_set_transition_duration(GTK_STACK(stack), 500);

    GtkWidget *library_page = create_library_page(appWidgets);
    gtk_stack_add_named(GTK_STACK(stack), library_page, "Library");

    GtkWidget *settings_page = create_settings_page(appWidgets);
    gtk_stack_add_named(GTK_STACK(stack), settings_page, "Settings");

    GtkWidget *about_page = create_about_page();
    gtk_stack_add_named(GTK_STACK(stack), about_page, "About");

    appWidgets->stack = stack;
    return stack;
}

int main(int argc, char *argv[])
{
    gtk_init(&argc, &argv);

    AppWidgets appWidgets;
    appWidgets.window = create_main_window();
    GtkWidget *stack = create_stack_with_pages(&appWidgets);
    GtkWidget *hbox = create_navigation_buttons(stack);

    GtkWidget *vbox = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_box_pack_start(GTK_BOX(vbox), hbox, FALSE, FALSE, 0);
    gtk_box_pack_start(GTK_BOX(vbox), stack, TRUE, TRUE, 0);

    gtk_container_add(GTK_CONTAINER(appWidgets.window), vbox);
    
    char api_key[256], steam_id[256];
    char config_path[PATH_MAX];
    get_config_path(config_path);
    sprintf(steam.db_path, "%s/steam_games.db", config_path);

    init_database(&steam);
    db_fetch_all_games(steam.db_path, create_game_row, appWidgets.game_list_box);

    gtk_widget_show_all(appWidgets.window);
    gtk_main();

    sqlite3_close(steam.db);

    return 0;
}
