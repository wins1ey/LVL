#include <stdlib.h>

#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <gtk/gtk.h>

#include "embedded-python.h"

// Global variable to store the selected Steam ID
const gchar *selected_steam_id = NULL;

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
    selected_steam_id = (const gchar *)g_object_get_data(G_OBJECT(row), "steam_id");
}

// Callback function for running a shell command with selected game
void on_run_command_clicked(GtkWidget *widget, gpointer data)
{
    if (selected_steam_id != NULL) {
        char command[100];
        snprintf(command, sizeof(command), "steam -applaunch %s", selected_steam_id);
        system(command);
    } else {
        g_print("No game selected!\n");
    }
}

void run_python()
{
    // Ensure Python interpreter is initialized
    Py_Initialize();

    // Convert the embedded Python code to a Python bytes object
    PyObject *py_code_bytes = Py_BuildValue("y#", embedded_python_py, embedded_python_py_len);
    if (!py_code_bytes) {
        PyErr_Print();
        fprintf(stderr, "Failed to create Python bytes object.\n");
        Py_Finalize();
        return;
    }

    // Convert bytes to string; necessary for PyRun_StringFlags
    PyObject *py_code_str = PyUnicode_FromEncodedObject(py_code_bytes, NULL, NULL);
    Py_DECREF(py_code_bytes);  // Decrement reference of bytes object since it's no longer needed
    if (!py_code_str) {
        PyErr_Print();
        fprintf(stderr, "Failed to convert bytes to string.\n");
        Py_Finalize();
        return;
    }

    // Load '__main__' module and get its dictionary
    PyObject *main_module = PyImport_AddModule("__main__");
    if (!main_module) {
        PyErr_Print();
        fprintf(stderr, "Failed to get __main__ module.\n");
        Py_DECREF(py_code_str);
        Py_Finalize();
        return;
    }
    PyObject *global_dict = PyModule_GetDict(main_module);
    PyObject *local_dict = PyDict_New();
    if (!local_dict) {
        PyErr_Print();
        fprintf(stderr, "Failed to create local dictionary.\n");
        Py_DECREF(py_code_str);
        Py_Finalize();
        return;
    }

    // Execute the script
    PyObject *result = PyRun_String(PyUnicode_AsUTF8(py_code_str), Py_file_input, global_dict, local_dict);
    Py_DECREF(py_code_str);  // Clean up the string object immediately after use
    if (!result) {
        PyErr_Print();
        fprintf(stderr, "Failed to execute embedded Python script.\n");
    } else {
        Py_DECREF(result);
    }

    Py_DECREF(local_dict);
    Py_Finalize();
}

int main(int argc, char *argv[])
{

    run_python();
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

    // Add some sample games
    const gchar *game_names[] = {"Game 1", "Game 2", "Game 3"};
    const gchar *game_steam_ids[] = {"730", "377160", "105600"};
    for (int i = 0; i < G_N_ELEMENTS(game_names); i++) {
        GtkWidget *row = gtk_list_box_row_new();
        GtkWidget *label = gtk_label_new(game_names[i]);
        gtk_container_add(GTK_CONTAINER(row), label);
        gtk_list_box_insert(GTK_LIST_BOX(game_list_box), row, -1);
        g_object_set_data(G_OBJECT(row), "steam_id", (gpointer)game_steam_ids[i]);
    }

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
