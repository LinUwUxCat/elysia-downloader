#pragma once

#include <string>
#include <vector>
#include <curl/curl.h>

struct DanbooruImage {
    std::string id;
    std::string file_url;
    std::string filename;
    std::string tags;
    std::string rating;
    int width;
    int height;
};

class DanbooruClient {
public:
    DanbooruClient();
    ~DanbooruClient();
    
    std::vector<DanbooruImage> search_images(const std::vector<std::string>& tags, int limit = 100);
    DanbooruImage get_random_image(const std::vector<std::string>& tags);
    
private:
    CURL* curl;
    
    static size_t write_callback(void* contents, size_t size, size_t nmemb, std::string* userp);
    std::string make_request(const std::string& url);
    std::vector<DanbooruImage> parse_json_response(const std::string& json);
};


