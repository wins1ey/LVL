#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include <sqlite3.h>

#include "steam.h"
#include "embedded-python.h"
#include "db.h"

void run_python(const char *api_key, const char *steam_id, sqlite3 *db)
{
    Py_Initialize();

    PyObject *pArgs = PyTuple_New(2);
    PyTuple_SetItem(pArgs, 0, PyUnicode_FromString(api_key)); // API Key
    PyTuple_SetItem(pArgs, 1, PyUnicode_FromString(steam_id)); // Steam ID

    // Load the embedded script as a Python string
    PyObject *py_code_str = Py_BuildValue("y#", src_embedded_python_py, src_embedded_python_py_len);
    if (!py_code_str) {
        PyErr_Print();
        fprintf(stderr, "Failed to create Python code string object.\n");
        Py_Finalize();
        return;
    }

    // Execute the embedded Python script in the '__main__' module's namespace
    PyObject *main_module = PyImport_AddModule("__main__");
    PyObject *global_dict = PyModule_GetDict(main_module);
    PyObject *result = PyRun_String(PyBytes_AsString(py_code_str), Py_file_input, global_dict, global_dict);
    Py_DECREF(py_code_str);

    if (!result) {
        PyErr_Print();
        fprintf(stderr, "Failed to execute embedded Python script.\n");
    } else {
        Py_DECREF(result);  // Clean up the result object
    }

    // After the script is executed, retrieve and call the function from the global namespace
    PyObject *pFunc = PyDict_GetItemString(global_dict, "main");
    if (pFunc && PyCallable_Check(pFunc)) {
        PyObject *pValue = PyObject_CallObject(pFunc, pArgs);
        if (pValue != NULL && PyTuple_Check(pValue) && PyTuple_Size(pValue) == 2) {
            PyObject *pNames = PyTuple_GetItem(pValue, 0);
            PyObject *pIDs = PyTuple_GetItem(pValue, 1);

            if (pNames && PyList_Check(pNames) && pIDs && PyList_Check(pIDs) &&
                PyList_Size(pNames) == PyList_Size(pIDs)) {

                // Iterate over the lists of names and IDs
                Py_ssize_t num_games = PyList_Size(pNames);
                for (Py_ssize_t i = 0; i < num_games; i++) {
                    PyObject *pName = PyList_GetItem(pNames, i);
                    PyObject *pID = PyList_GetItem(pIDs, i);

                    int game_id = pID ? PyLong_AsLong(pID) : -1;
                    const char *game_name = pName ? PyUnicode_AsUTF8(pName) : "Unknown";

                    printf("ID: %d, Name: %s\n", game_id, game_name);
                    insert_game(db, game_id, game_name);
                }
            } else {
                PyErr_Print();
                fprintf(stderr, "Returned values are not lists or lists of different lengths\n");
            }
        } else {
            PyErr_Print();
            fprintf(stderr, "Call failed or return type is not a tuple of length 2\n");
        }
        Py_XDECREF(pValue);
    } else {
        PyErr_Print();
        fprintf(stderr, "Function main not found or not callable\n");
    }
}

