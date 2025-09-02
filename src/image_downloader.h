#pragma once

#include <string>
#include <curl/curl.h>

class ImageDownloader {
public:
    ImageDownloader();
    ~ImageDownloader();
    
    bool download_image(const std::string& url, const std::string& filepath);
    
private:
    CURL* curl;
    
    static size_t write_data(void* ptr, size_t size, size_t nmemb, FILE* stream);
    static int progress_callback(void* clientp, curl_off_t dltotal, curl_off_t dlnow, curl_off_t ultotal, curl_off_t ulnow);
};
