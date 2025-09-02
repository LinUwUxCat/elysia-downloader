#include "image_downloader.h"
#include <iostream>
#include <fstream>
#include <filesystem>
#include <iomanip>

ImageDownloader::ImageDownloader() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

ImageDownloader::~ImageDownloader() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

bool ImageDownloader::download_image(const std::string& url, const std::string& filepath) {
    FILE* file = fopen(filepath.c_str(), "wb");
    if (!file) {
        std::cerr << "Failed to open file for writing: " << filepath << std::endl;
        return false;
    }
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_data);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, file);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ElysiaDownloader/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 60L);
    curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0L);
    curl_easy_setopt(curl, CURLOPT_XFERINFOFUNCTION, progress_callback);
    
    CURLcode res = curl_easy_perform(curl);
    fclose(file);
    
    if (res != CURLE_OK) {
        std::cerr << "Download failed: " << curl_easy_strerror(res) << std::endl;
        // Remove the failed download file
        std::filesystem::remove(filepath);
        return false;
    }
    
    return true;
}

size_t ImageDownloader::write_data(void* ptr, size_t size, size_t nmemb, FILE* stream) {
    return fwrite(ptr, size, nmemb, stream);
}

int ImageDownloader::progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow) {
    if (dltotal > 0) {
        double progress = (dlnow * 100.0) / dltotal;
        std::cout << "\rDownload progress: " << std::fixed << std::setprecision(1) << progress << "%" << std::flush;
    }
    return 0;
}
