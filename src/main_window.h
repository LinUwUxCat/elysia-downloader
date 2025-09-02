#pragma once

#include <gtk/gtk.h>
#include <string>
#include <vector>

class MainWindow {
public:
    MainWindow();
    ~MainWindow();
    
    void show();
    void refresh_image();
    void download_current_image();
    
private:
    GtkWidget* window;
    GtkWidget* main_box;
    GtkWidget* image_widget;
    GtkWidget* button_box;
    GtkWidget* refresh_button;
    GtkWidget* download_button;
    
    std::string current_image_url;
    std::string current_image_filename;
    bool is_dark_theme;
    GtkCssProvider* theme_provider;
    guint theme_check_id;
    
    static void on_refresh_clicked(GtkButton* button, gpointer user_data);
    static void on_download_clicked(GtkButton* button, gpointer user_data);
    static void on_window_destroy(GtkWidget* widget, gpointer user_data);
    
    void setup_ui();
    void setup_css();
    void update_theme_css();
    void detect_and_apply_theme();
    void load_random_image();
    void set_image_from_url(const std::string& url);
    std::string select_download_directory();
};
