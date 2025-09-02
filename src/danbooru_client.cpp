#include "danbooru_client.h"
#include <iostream>
#include <random>
#include <sstream>
#include <regex>
#include <algorithm>

DanbooruClient::DanbooruClient() {
    curl_global_init(CURL_GLOBAL_DEFAULT);
    curl = curl_easy_init();
    if (!curl) {
        throw std::runtime_error("Failed to initialize CURL");
    }
}

DanbooruClient::~DanbooruClient() {
    if (curl) {
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

std::vector<DanbooruImage> DanbooruClient::search_images(const std::vector<std::string>& tags, int limit) {
    std::ostringstream url_stream;
    url_stream << "https://danbooru.donmai.us/posts.json?tags=";
    
    for (size_t i = 0; i < tags.size(); ++i) {
        if (i > 0) url_stream << "+";
        url_stream << tags[i];
    }
    url_stream << "&limit=" << limit;
    
    std::string response = make_request(url_stream.str());
    return parse_json_response(response);
}

DanbooruImage DanbooruClient::get_random_image(const std::vector<std::string>& tags) {
    auto images = search_images(tags, 100);
    
    if (images.empty()) {
        return DanbooruImage{};
    }
    
    std::random_device rd;
    std::mt19937 gen(rd());
    std::uniform_int_distribution<> dis(0, images.size() - 1);
    
    return images[dis(gen)];
}

size_t DanbooruClient::write_callback(void* contents, size_t size, size_t nmemb, std::string* userp) {
    userp->append((char*)contents, size * nmemb);
    return size * nmemb;
}

std::string DanbooruClient::make_request(const std::string& url) {
    std::string response;
    
    std::cout << "Making request to: " << url << std::endl;
    
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(curl, CURLOPT_USERAGENT, "ElysiaDownloader/1.0");
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
    
    CURLcode res = curl_easy_perform(curl);
    if (res != CURLE_OK) {
        throw std::runtime_error("CURL request failed: " + std::string(curl_easy_strerror(res)));
    }
    
    std::cout << "Response length: " << response.length() << " characters" << std::endl;
    
    return response;
}

std::vector<DanbooruImage> DanbooruClient::parse_json_response(const std::string& json_str) {
    std::vector<DanbooruImage> images;
    
    // Debug: Print the response to see what we're getting
    std::cout << "Received JSON response: " << json_str.substr(0, 500) << "..." << std::endl;
    
    // Simpler regex patterns for individual fields
    std::regex id_regex("\"id\"\\s*:\\s*(\\d+)");
    std::regex file_url_regex("\"file_url\"\\s*:\\s*\"([^\"]*)\"");
    std::regex file_ext_regex("\"file_ext\"\\s*:\\s*\"([^\"]*)\"");
    std::regex tag_string_regex("\"tag_string\"\\s*:\\s*\"([^\"]*)\"");
    std::regex rating_regex("\"rating\"\\s*:\\s*\"([^\"]*)\"");
    std::regex width_regex("\"image_width\"\\s*:\\s*(\\d+)");
    std::regex height_regex("\"image_height\"\\s*:\\s*(\\d+)");
    
    // Find all matches for each field
    std::sregex_iterator id_iter(json_str.begin(), json_str.end(), id_regex);
    std::sregex_iterator file_url_iter(json_str.begin(), json_str.end(), file_url_regex);
    std::sregex_iterator file_ext_iter(json_str.begin(), json_str.end(), file_ext_regex);
    std::sregex_iterator tag_string_iter(json_str.begin(), json_str.end(), tag_string_regex);
    std::sregex_iterator rating_iter(json_str.begin(), json_str.end(), rating_regex);
    std::sregex_iterator width_iter(json_str.begin(), json_str.end(), width_regex);
    std::sregex_iterator height_iter(json_str.begin(), json_str.end(), height_regex);
    
    std::sregex_iterator end;
    
    // Collect all matches
    std::vector<std::string> ids, file_urls, file_exts, tag_strings, ratings, widths, heights;
    
    for (; id_iter != end; ++id_iter) ids.push_back((*id_iter)[1].str());
    for (; file_url_iter != end; ++file_url_iter) file_urls.push_back((*file_url_iter)[1].str());
    for (; file_ext_iter != end; ++file_ext_iter) file_exts.push_back((*file_ext_iter)[1].str());
    for (; tag_string_iter != end; ++tag_string_iter) tag_strings.push_back((*tag_string_iter)[1].str());
    for (; rating_iter != end; ++rating_iter) ratings.push_back((*rating_iter)[1].str());
    for (; width_iter != end; ++width_iter) widths.push_back((*width_iter)[1].str());
    for (; height_iter != end; ++height_iter) heights.push_back((*height_iter)[1].str());
    
    // Create images from matched data
    size_t min_size = std::min({ids.size(), file_urls.size(), file_exts.size(), 
                                tag_strings.size(), ratings.size(), widths.size(), heights.size()});
    
    std::cout << "Found " << min_size << " images" << std::endl;
    
    for (size_t i = 0; i < min_size; ++i) {
        DanbooruImage image;
        image.id = ids[i];
        image.file_url = file_urls[i];
        
        // Extract filename from URL and add proper extension
        std::string url = file_urls[i];
        size_t last_slash = url.find_last_of('/');
        if (last_slash != std::string::npos) {
            std::string url_filename = url.substr(last_slash + 1);
            // Remove any query parameters
            size_t question_mark = url_filename.find('?');
            if (question_mark != std::string::npos) {
                url_filename = url_filename.substr(0, question_mark);
            }
            image.filename = url_filename;
        } else {
            // Fallback: use ID + extension
            image.filename = "elysia_" + ids[i] + "." + file_exts[i];
        }
        
        image.tags = tag_strings[i];
        image.rating = ratings[i];
        image.width = std::stoi(widths[i]);
        image.height = std::stoi(heights[i]);
        
        // Add all images since we're filtering at the API level with -video tag
        if (!image.file_url.empty()) {
            images.push_back(image);
            std::cout << "Added image: " << image.file_url << " (format: " << file_exts[i] << ")" << std::endl;
        }
    }
    
    return images;
}


