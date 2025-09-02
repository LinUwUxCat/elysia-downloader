#include "main_window.h"
#include <gtk/gtk.h>
#include <iostream>

int main(int argc, char* argv[]) {
    // Initialize GTK
    gtk_init();
    
    try {
        // Create and show the main window
        MainWindow main_window;
        main_window.show();
        
        // Start the GTK main loop
        GMainLoop* loop = g_main_loop_new(nullptr, FALSE);
        g_main_loop_run(loop);
        g_main_loop_unref(loop);
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }
    
    return 0;
}
