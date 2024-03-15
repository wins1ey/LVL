#include <gtk/gtk.h>
#include <stdlib.h>

// Function declarations
void on_window_destroy(GtkWidget *widget, gpointer data);
void on_button_clicked(GtkWidget *widget, gpointer data);
void on_run_command_clicked(GtkWidget *widget, const gchar *steam_id);

int main(int argc, char *argv[]) {
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
    g_signal_connect(run_command_button, "clicked", G_CALLBACK(on_run_command_clicked), "730");

    // Add the button to Page 1
    gtk_container_add(GTK_CONTAINER(library_page), run_command_button);

    // Show all widgets
    gtk_widget_show_all(window);

    // Start the GTK main loop
    gtk_main();

    return 0;
}

// Callback function for window destruction
void on_window_destroy(GtkWidget *widget, gpointer data) {
    gtk_main_quit(); // Quit the GTK main loop when the window is destroyed
}

// Callback function for button clicks
void on_button_clicked(GtkWidget *widget, gpointer data) {
    GtkStack *stack = GTK_STACK(data);
    const gchar *page_name = gtk_button_get_label(GTK_BUTTON(widget));
    gtk_stack_set_visible_child_name(stack, page_name);
}

// Callback function for running a shell command with Steam ID
void on_run_command_clicked(GtkWidget *widget, const gchar *steam_id) {
    // Construct the command string
    char command[100];
    snprintf(command, sizeof(command), "steam -applaunch %s", steam_id);

    // Execute the command
    system(command);
}
