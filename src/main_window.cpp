#include "main_window.h"
#include "danbooru_client.h"
#include "image_downloader.h"
#include <gtk/gtk.h>
#include <glib.h>
#include <iostream>
#include <random>
#include <filesystem>
#include <algorithm>

MainWindow::MainWindow() {
    is_dark_theme = false;
    theme_provider = nullptr;
    theme_check_id = 0;
    setup_ui();
    
    // Auto-detect and apply theme
    detect_and_apply_theme();
    
    // Start theme monitoring
    theme_check_id = g_timeout_add(2000, [](gpointer user_data) -> gboolean {
        MainWindow* self = static_cast<MainWindow*>(user_data);
        self->detect_and_apply_theme();
        return G_SOURCE_CONTINUE; // Continue monitoring
    }, this);
    
    // Don't load image immediately, wait for the UI to be ready
    // We'll use a timer to trigger the first image load
    g_timeout_add(100, [](gpointer user_data) -> gboolean {
        MainWindow* self = static_cast<MainWindow*>(user_data);
        self->load_random_image();
        return G_SOURCE_REMOVE; // Only run once
    }, this);
}

MainWindow::~MainWindow() {
    // Clean up theme monitoring
    if (theme_check_id > 0) {
        g_source_remove(theme_check_id);
        theme_check_id = 0;
    }
    
    // Clean up theme provider
    if (theme_provider) {
        g_object_unref(theme_provider);
    }
    // GTK widgets are automatically destroyed
}

void MainWindow::setup_ui() {
    // Create main window
    window = gtk_window_new();
    gtk_window_set_title(GTK_WINDOW(window), "Elysia Downloader");
    gtk_window_set_default_size(GTK_WINDOW(window), 600, 750);
    gtk_window_set_resizable(GTK_WINDOW(window), FALSE); // Make window non-resizable
    g_signal_connect(window, "destroy", G_CALLBACK(on_window_destroy), this);
    
    // Create main vertical box with proper spacing and margins
    main_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
    gtk_widget_set_margin_start(main_box, 40);
    gtk_widget_set_margin_end(main_box, 40);
    gtk_widget_set_margin_top(main_box, 30);
    gtk_widget_set_margin_bottom(main_box, 30);
    gtk_window_set_child(GTK_WINDOW(window), main_box);
    
    // Create header section with title and subtitle
    GtkWidget* header_box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 10);
    gtk_widget_set_halign(header_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(header_box, 20);
    
    GtkWidget* title_label = gtk_label_new("Elysia Downloader");
    gtk_widget_add_css_class(title_label, "display-1");
    gtk_widget_add_css_class(title_label, "accent");
    gtk_widget_set_halign(title_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(header_box), title_label);
    
    GtkWidget* subtitle_label = gtk_label_new("Download beautiful Elysia Art!");
    gtk_widget_add_css_class(subtitle_label, "title-3");
    gtk_widget_add_css_class(subtitle_label, "dim-label");
    gtk_widget_set_halign(subtitle_label, GTK_ALIGN_CENTER);
    gtk_box_append(GTK_BOX(header_box), subtitle_label);
    
    gtk_box_append(GTK_BOX(main_box), header_box);
    
    // Create button box with improved styling
    button_box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 15);
    gtk_widget_set_halign(button_box, GTK_ALIGN_CENTER);
    gtk_widget_set_margin_bottom(button_box, 20);
    
    // Create refresh button with glass button style (icon only)
    refresh_button = gtk_button_new_from_icon_name("view-refresh-symbolic");
    gtk_widget_add_css_class(refresh_button, "glass-button");
    gtk_widget_add_css_class(refresh_button, "icon-button");
    gtk_widget_set_size_request(refresh_button, 50, 50);
    gtk_widget_set_tooltip_text(refresh_button, "Refresh Image");
    g_signal_connect(refresh_button, "clicked", G_CALLBACK(on_refresh_clicked), this);
    gtk_box_append(GTK_BOX(button_box), refresh_button);
    
    // Create download button with glass button style (icon only)
    download_button = gtk_button_new_from_icon_name("document-save-symbolic");
    gtk_widget_add_css_class(download_button, "glass-button");
    gtk_widget_add_css_class(download_button, "icon-button");
    gtk_widget_set_size_request(download_button, 50, 50);
    gtk_widget_set_tooltip_text(download_button, "Download Image");
    g_signal_connect(download_button, "clicked", G_CALLBACK(on_download_clicked), this);
    gtk_box_append(GTK_BOX(button_box), download_button);
    
    gtk_box_append(GTK_BOX(main_box), button_box);
    
    // Create image container with proper styling
    GtkWidget* image_container = gtk_box_new(GTK_ORIENTATION_VERTICAL, 0);
    gtk_widget_add_css_class(image_container, "image-container");
    gtk_widget_set_hexpand(image_container, TRUE);
    gtk_widget_set_vexpand(image_container, TRUE);
    
    // Create image widget
    image_widget = gtk_picture_new();
    gtk_widget_set_hexpand(image_widget, TRUE);
    gtk_widget_set_vexpand(image_widget, TRUE);
    gtk_widget_add_css_class(image_widget, "main-image");
    
    // Set size request for better image display
    gtk_widget_set_size_request(image_widget, 500, 300);
    
    gtk_box_append(GTK_BOX(image_container), image_widget);
    
    gtk_box_append(GTK_BOX(main_box), image_container);
    
    // Apply CSS styling
    setup_css();
}

void MainWindow::setup_css() {
    theme_provider = gtk_css_provider_new();
    update_theme_css();
}

void MainWindow::detect_and_apply_theme() {
    // Get current GTK theme
    GtkSettings* settings = gtk_settings_get_default();
    gchar* theme = NULL;
    g_object_get(settings, "gtk-theme-name", &theme, NULL);
    
    if (theme) {
        gboolean should_be_dark = FALSE;
        if (g_strcmp0(theme, "ElysiaOS-HoC") == 0) {
            should_be_dark = TRUE;
        } else if (g_strcmp0(theme, "ElysiaOS") == 0) {
            should_be_dark = FALSE;
        } else {
            // For other themes, try to detect based on theme name
            gchar* lower_theme = g_ascii_strdown(theme, -1);
            if (g_strrstr(lower_theme, "dark") || g_strrstr(lower_theme, "black")) {
                should_be_dark = TRUE;
            }
            g_free(lower_theme);
        }
        
        if (is_dark_theme != should_be_dark) {
            is_dark_theme = should_be_dark;
            update_theme_css();
        }
        
        g_free(theme);
    }
}

void MainWindow::update_theme_css() {
    if (!theme_provider) return;
    
    const char* css;
    if (is_dark_theme) {
        css = 
            "window {"
            "  background: linear-gradient(135deg, #333 0%, #2c3e50 100%);"
            "  color: #ffffff;"
            "  font-family: 'ElysiaOSNew12', sans-serif;"
            "}"
            ".display-1 {"
            "  font-size: 34px;"
            "  font-weight: bold;"
            "  color: #fc77d9;"
            "}"
            ".title-3 {"
            "  font-size: 18px;"
            "  font-weight: normal;"
            "}"
            ".dim-label {"
            "  color: #cccccc;"
            "}"
            ".accent {"
            "  color: #fc77d9;"
            "}"
            ".glass-button {"
            "  background: rgba(255, 255, 255, 0.1);"
            "  border: 1px solid rgba(252, 119, 217, 0.4);"
            "  border-radius: 20px;"
            "  color: #ffffff;"
            "  font-weight: 600;"
            "  backdrop-filter: blur(10px);"
            "  transition: all 0.2s ease;"
            "}"
            ".glass-button:hover {"
            "  background: rgba(255, 255, 255, 0.2);"
            "  border-color: #fc77d9;"
            "  border-radius: 20px;"
            "  color: #ffffff;"
            "  font-weight: 600;"
            "  backdrop-filter: blur(10px);"
            "  transition: all 0.2s ease;"
            "}"
            ".glass-button:active {"
            "  transform: translateY(0);"
            "  box-shadow: 0 2px 6px rgba(252, 119, 217, 0.3);"
            "}"
            ".icon-button {"
            "  border-radius: 25px;"
            "  padding: 0;"
            "}"
            ".icon-button image {"
            "  font-size: 24px;"
            "}"
            ".image-container {"
            "  background: rgba(255, 255, 255, 0.05);"
            "  border-radius: 16px;"
            "  border: 2px solid rgba(252, 119, 217, 0.3);"
            "  padding: 30px;"
            "  backdrop-filter: blur(10px);"
            "  min-height: 600px;"
            "}"
            ".main-image {"
            "  border-radius: 12px;"
            "  overflow: hidden;"
            "  min-width: 500px;"
            "  min-height: 300px;"
            "  object-fit: contain;"
            "  max-width: 100%;"
            "  max-height: 100%;"
            "}"
            ".loading-label {"
            "  font-size: 18px;"
            "  color: #cccccc;"
            "  text-align: center;"
            "}"
            ".error-label {"
            "  font-size: 16px;"
            "  color: #e74c3c;"
            "  text-align: center;"
            "  background: rgba(231, 76, 60, 0.1);"
            "  padding: 20px;"
            "  border-radius: 12px;"
            "  border: 1px solid rgba(231, 76, 60, 0.3);"
            "}"
            ".info-label {"
            "  font-size: 16px;"
            "  color: #3498db;"
            "  text-align: center;"
            "  background: rgba(52, 152, 219, 0.1);"
            "  padding: 20px;"
            "  border-radius: 12px;"
            "  border: 1px solid rgba(52, 152, 219, 0.3);"
            "}";
    } else {
        css = 
            "window {"
            "  background: linear-gradient(135deg, #ffedfa 0%, #f0f7ff 100%);"
            "  color: #333;"
            "  font-family: 'ElysiaOSNew12', sans-serif;"
            "}"
            ".display-1 {"
            "  font-size: 34px;"
            "  font-weight: bold;"
            "  color: #fc77d9;"
            "}"
            ".title-3 {"
            "  font-size: 18px;"
            "  font-weight: normal;"
            "}"
            ".dim-label {"
            "  color: #666;"
            "}"
            ".accent {"
            "  color: #fc77d9;"
            "}"
            ".glass-button {"
            "  background: rgba(255, 255, 255, 0.2);"
            "  border: 1px solid rgba(252, 119, 217, 0.3);"
            "  border-radius: 20px;"
            "  color: #333;"
            "  font-weight: 600;"
            "  backdrop-filter: blur(10px);"
            "  transition: all 0.2s ease;"
            "}"
            ".glass-button:hover {"
            "  background: rgba(255, 255, 255, 0.3);"
            "  border-color: #fc77d9;"
            "  transform: translateY(-2px);"
            "  box-shadow: 0 4px 12px rgba(252, 119, 217, 0.3);"
            "}"
            ".glass-button:active {"
            "  transform: translateY(0);"
            "  box-shadow: 0 2px 6px rgba(252, 119, 217, 0.2);"
            "}"
            ".icon-button {"
            "  border-radius: 25px;"
            "  padding: 0;"
            "}"
            ".icon-button image {"
            "  font-size: 24px;"
            "}"
            ".image-container {"
            "  background: rgba(255, 255, 255, 0.1);"
            "  border-radius: 16px;"
            "  border: 2px solid rgba(252, 119, 217, 0.2);"
            "  padding: 30px;"
            "  backdrop-filter: blur(10px);"
            "  min-height: 600px;"
            "}"
            ".main-image {"
            "  border-radius: 12px;"
            "  overflow: hidden;"
            "  min-width: 500px;"
            "  min-height: 300px;"
            "  object-fit: contain;"
            "  max-width: 100%;"
            "  max-height: 100%;"
            "}"
            ".loading-label {"
            "  font-size: 18px;"
            "  color: #666;"
            "  text-align: center;"
            "}"
            ".error-label {"
            "  font-size: 16px;"
            "  color: #e74c3c;"
            "  text-align: center;"
            "  background: rgba(231, 76, 60, 0.1);"
            "  padding: 20px;"
            "  border-radius: 12px;"
            "  border: 1px solid rgba(231, 76, 60, 0.3);"
            "}"
            ".info-label {"
            "  font-size: 16px;"
            "  color: #3498db;"
            "  text-align: center;"
            "  background: rgba(52, 152, 219, 0.1);"
            "  padding: 20px;"
            "  border-radius: 12px;"
            "  border: 1px solid rgba(52, 152, 219, 0.3);"
            "}";
    }
    
    gtk_css_provider_load_from_string(theme_provider, css);
    gtk_style_context_add_provider_for_display(
        gdk_display_get_default(),
        GTK_STYLE_PROVIDER(theme_provider),
        GTK_STYLE_PROVIDER_PRIORITY_APPLICATION
    );
}

void MainWindow::show() {
    gtk_widget_set_visible(window, TRUE);
}

void MainWindow::refresh_image() {
    load_random_image();
}

void MainWindow::load_random_image() {
    std::cout << "Loading random image..." << std::endl;
    
    // Show loading indicator
    GtkWidget* loading_label = gtk_label_new("Loading Elysia image...\nPlease wait...");
    gtk_widget_set_hexpand(loading_label, TRUE);
    gtk_widget_set_vexpand(loading_label, TRUE);
    gtk_label_set_wrap(GTK_LABEL(loading_label), TRUE);
    gtk_label_set_justify(GTK_LABEL(loading_label), GTK_JUSTIFY_CENTER);
    gtk_widget_add_css_class(loading_label, "loading-label");
    
    // Replace the image widget inside the image container
    GtkWidget* image_container = gtk_widget_get_parent(image_widget);
    if (image_container) {
        gtk_box_remove(GTK_BOX(image_container), image_widget);
    image_widget = loading_label;
        gtk_box_append(GTK_BOX(image_container), image_widget);
    }
    
    try {
        DanbooruClient client;
        std::vector<std::string> tags = {
            "elysia_(honkai_impact)",
            "elysia_(herrscher_of_human:_ego)_(honkai_impact)",
            "elysia_(miss_pink)_(honkai_impact)"
        };
        
        // Try each tag one by one until we find an image
        DanbooruImage image;
        std::string used_tag;
        
        for (const auto& tag : tags) {
            std::cout << "Trying tag: " << tag << std::endl;
            
            // Use Danbooru's built-in tag filtering to exclude videos
            std::vector<std::string> search_tags = {tag, "-video"};
            
            auto images = client.search_images(search_tags, 50);
            
            if (!images.empty()) {
                // Filter for higher quality images and pick the best one
                std::vector<DanbooruImage> quality_images;
                for (const auto& img : images) {
                    // Prefer images with reasonable dimensions (not too small, not too large)
                    if (img.width >= 500 && img.height >= 600 && 
                        img.width <= 4000 && img.height <= 3000) {
                        quality_images.push_back(img);
                    }
                }
                
                if (quality_images.empty()) {
                    // If no quality images found, use all images
                    quality_images = images;
                }
                
                // Pick a random image from quality results
                std::random_device rd;
                std::mt19937 gen(rd());
                std::uniform_int_distribution<> dis(0, quality_images.size() - 1);
                image = quality_images[dis(gen)];
                used_tag = tag;
                std::cout << "Found " << quality_images.size() << " quality images with tag: " << tag << std::endl;
                break;
            } else {
                std::cout << "No images found with tag: " << tag << std::endl;
            }
        }
        
        if (!image.file_url.empty()) {
            std::cout << "Got image: " << image.file_url << " using tag: " << used_tag << std::endl;
            current_image_url = image.file_url;
            current_image_filename = image.filename;
            set_image_from_url(image.file_url);
        } else {
            std::cout << "No image found with any tag!" << std::endl;
            // Show a message that no image was found
            GtkWidget* label = gtk_label_new("No images found with any of the specified tags.\nTry clicking Refresh again.");
            gtk_widget_set_hexpand(label, TRUE);
            gtk_widget_set_vexpand(label, TRUE);
            gtk_label_set_wrap(GTK_LABEL(label), TRUE);
            gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
            gtk_widget_add_css_class(label, "info-label");
            
            // Replace the image widget inside the image container
            GtkWidget* image_container = gtk_widget_get_parent(image_widget);
            if (image_container) {
                gtk_box_remove(GTK_BOX(image_container), image_widget);
            image_widget = label;
                gtk_box_append(GTK_BOX(image_container), image_widget);
            }
        }
    } catch (const std::exception& e) {
        std::cerr << "Error loading image: " << e.what() << std::endl;
        // Show error message in UI
        GtkWidget* label = gtk_label_new(("Error loading image: " + std::string(e.what())).c_str());
        gtk_widget_set_hexpand(label, TRUE);
        gtk_widget_set_vexpand(label, TRUE);
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
        gtk_widget_add_css_class(label, "error-label");
        
        // Replace the image widget inside the image container
        GtkWidget* image_container = gtk_widget_get_parent(image_widget);
        if (image_container) {
            gtk_box_remove(GTK_BOX(image_container), image_widget);
        image_widget = label;
            gtk_box_append(GTK_BOX(image_container), image_widget);
        }
    }
}

void MainWindow::set_image_from_url(const std::string& url) {
    std::cout << "Setting image from URL: " << url << std::endl;
    
    // Create a new picture widget
    GtkWidget* new_image_widget = gtk_picture_new();
    gtk_widget_set_hexpand(new_image_widget, TRUE);
    gtk_widget_set_vexpand(new_image_widget, TRUE);
    gtk_widget_add_css_class(new_image_widget, "main-image");
    
    // Set size request for better image display
    gtk_widget_set_size_request(new_image_widget, 500, 300);
    
    // Download the image first, then display it
    std::string temp_filename = "/tmp/elysia_temp_image.jpg";
    
    try {
        ImageDownloader downloader;
        if (downloader.download_image(url, temp_filename)) {
            std::cout << "Image downloaded successfully to temp file" << std::endl;
            
            // Now load the downloaded image
            gtk_picture_set_filename(GTK_PICTURE(new_image_widget), temp_filename.c_str());
            
            // Check if the picture has content after loading
            GdkPaintable* paintable = gtk_picture_get_paintable(GTK_PICTURE(new_image_widget));
            if (paintable) {
                std::cout << "Image loaded successfully from local file" << std::endl;
            } else {
                std::cout << "Failed to load image from local file" << std::endl;
                // Show placeholder if loading fails
                GtkWidget* label = gtk_label_new(("Image downloaded but failed to display.\nURL: " + url + "\n\nClick Refresh for new image").c_str());
                gtk_widget_set_hexpand(label, TRUE);
                gtk_widget_set_vexpand(label, TRUE);
                gtk_label_set_wrap(GTK_LABEL(label), TRUE);
                gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
                gtk_widget_add_css_class(label, "error-label");
                new_image_widget = label;
            }
        } else {
            std::cout << "Failed to download image from URL" << std::endl;
            // Show placeholder if download fails
            GtkWidget* label = gtk_label_new(("Failed to download image.\nURL: " + url + "\n\nClick Refresh for new image").c_str());
            gtk_widget_set_hexpand(label, TRUE);
            gtk_widget_set_vexpand(label, TRUE);
            gtk_label_set_wrap(GTK_LABEL(label), TRUE);
            gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
            gtk_widget_add_css_class(label, "error-label");
            new_image_widget = label;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error in set_image_from_url: " << e.what() << std::endl;
        // Show error message
        GtkWidget* label = gtk_label_new(("Error loading image: " + std::string(e.what()) + "\n\nClick Refresh for new image").c_str());
        gtk_widget_set_hexpand(label, TRUE);
        gtk_widget_set_vexpand(label, TRUE);
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_label_set_justify(GTK_LABEL(label), GTK_JUSTIFY_CENTER);
        gtk_widget_add_css_class(label, "error-label");
        new_image_widget = label;
    }
    
    // Replace the current image widget inside the image container
    GtkWidget* image_container = gtk_widget_get_parent(image_widget);
    if (image_container) {
        gtk_box_remove(GTK_BOX(image_container), image_widget);
    image_widget = new_image_widget;
        gtk_box_append(GTK_BOX(image_container), image_widget);
    }
}

void MainWindow::download_current_image() {
    if (current_image_url.empty()) {
        // Show simple message that no image is loaded
        GtkWidget* dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "No Image");
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
        gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
        
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
        gtk_widget_set_margin_start(box, 20);
        gtk_widget_set_margin_end(box, 20);
        gtk_widget_set_margin_top(box, 20);
        gtk_widget_set_margin_bottom(box, 20);
        
        GtkWidget* label = gtk_label_new("No image loaded. Please click Refresh first.");
        gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
        gtk_box_append(GTK_BOX(box), label);
        
        GtkWidget* button = gtk_button_new_with_label("OK");
        gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
        g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_box_append(GTK_BOX(box), button);
        
        gtk_window_set_child(GTK_WINDOW(dialog), box);
        gtk_window_present(GTK_WINDOW(dialog));
        return;
    }
    
    std::string download_dir = select_download_directory();
    if (download_dir.empty()) {
        // User cancelled the directory selection
        return;
    }
    
    std::string filepath = download_dir + "/" + current_image_filename;
    
    try {
        ImageDownloader downloader;
        if (downloader.download_image(current_image_url, filepath)) {
            std::cout << "Image downloaded successfully to: " << filepath << std::endl;
            
            // Show simple success message
            GtkWidget* dialog = gtk_window_new();
            gtk_window_set_title(GTK_WINDOW(dialog), "Download Complete");
            gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
            gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
            gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 200);
            gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
            
            GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
            gtk_widget_set_margin_start(box, 20);
            gtk_widget_set_margin_end(box, 20);
            gtk_widget_set_margin_top(box, 20);
            gtk_widget_set_margin_bottom(box, 20);
            
            std::string success_text = "Image downloaded successfully!\n\nSaved to:\n" + filepath;
            GtkWidget* label = gtk_label_new(success_text.c_str());
            gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
            gtk_label_set_wrap(GTK_LABEL(label), TRUE);
            gtk_box_append(GTK_BOX(box), label);
            
            GtkWidget* button = gtk_button_new_with_label("OK");
            gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
            g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
            gtk_box_append(GTK_BOX(box), button);
            
            gtk_window_set_child(GTK_WINDOW(dialog), box);
            gtk_window_present(GTK_WINDOW(dialog));
            
        } else {
            std::cerr << "Failed to download image" << std::endl;
            
            // Show simple error message
            GtkWidget* dialog = gtk_window_new();
            gtk_window_set_title(GTK_WINDOW(dialog), "Download Failed");
            gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
            gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
            gtk_window_set_default_size(GTK_WINDOW(dialog), 300, 150);
            gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
            
            GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
            gtk_widget_set_margin_start(box, 20);
            gtk_widget_set_margin_end(box, 20);
            gtk_widget_set_margin_top(box, 20);
            gtk_widget_set_margin_bottom(box, 20);
            
            GtkWidget* label = gtk_label_new("Failed to download image. Please try again.");
            gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
            gtk_box_append(GTK_BOX(box), label);
            
            GtkWidget* button = gtk_button_new_with_label("OK");
            gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
            g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
            gtk_box_append(GTK_BOX(box), button);
            
            gtk_window_set_child(GTK_WINDOW(dialog), box);
            gtk_window_present(GTK_WINDOW(dialog));
        }
    } catch (const std::exception& e) {
        std::cerr << "Error downloading image: " << e.what() << std::endl;
        
        // Show simple error message
        GtkWidget* dialog = gtk_window_new();
        gtk_window_set_title(GTK_WINDOW(dialog), "Download Error");
        gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(window));
        gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);
        gtk_window_set_default_size(GTK_WINDOW(dialog), 400, 200);
        gtk_window_set_resizable(GTK_WINDOW(dialog), FALSE);
        
        GtkWidget* box = gtk_box_new(GTK_ORIENTATION_VERTICAL, 20);
        gtk_widget_set_margin_start(box, 20);
        gtk_widget_set_margin_end(box, 20);
        gtk_widget_set_margin_top(box, 20);
        gtk_widget_set_margin_bottom(box, 20);
        
        std::string error_text = "Error downloading image:\n" + std::string(e.what());
        GtkWidget* label = gtk_label_new(error_text.c_str());
        gtk_widget_set_halign(label, GTK_ALIGN_CENTER);
        gtk_label_set_wrap(GTK_LABEL(label), TRUE);
        gtk_box_append(GTK_BOX(box), label);
        
        GtkWidget* button = gtk_button_new_with_label("OK");
        gtk_widget_set_halign(button, GTK_ALIGN_CENTER);
        g_signal_connect_swapped(button, "clicked", G_CALLBACK(gtk_window_destroy), dialog);
        gtk_box_append(GTK_BOX(box), button);
        
        gtk_window_set_child(GTK_WINDOW(dialog), box);
        gtk_window_present(GTK_WINDOW(dialog));
    }
}

std::string MainWindow::select_download_directory() {
    // For simplicity, use Downloads directory
    // In a real application, you'd want to use a proper file chooser dialog
    const char* home_dir = g_get_home_dir();
    if (home_dir) {
        char* downloads_path = g_build_filename(home_dir, "Pictures/Elysia", NULL);
        std::string path = downloads_path;
        g_free(downloads_path);
        
        // Create directory if it doesn't exist
        std::error_code ec;
        if (!std::filesystem::exists(path)) {
            if (!std::filesystem::create_directories(path, ec)) {
                // If directory creation fails, fall back to current directory
                g_warning("Failed to create directory %s: %s", path.c_str(), ec.message().c_str());
                return std::filesystem::current_path().string();
            }
        }
        
        return path;
    }
    
    // Fallback to current directory
    return std::filesystem::current_path().string();
}

// Static callback functions
void MainWindow::on_refresh_clicked(GtkButton* button, gpointer user_data) {
    MainWindow* self = static_cast<MainWindow*>(user_data);
    self->refresh_image();
}

void MainWindow::on_download_clicked(GtkButton* button, gpointer user_data) {
    MainWindow* self = static_cast<MainWindow*>(user_data);
    self->download_current_image();
}





void MainWindow::on_window_destroy(GtkWidget* widget, gpointer user_data) {
    // Exit the application
    exit(0);
}
