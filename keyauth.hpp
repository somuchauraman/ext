#pragma once
#include <string>
#include <iostream>
#include <vector>
#include <map>
#include <curl/curl.h>
#include <json/json.h>
#include <ctime>

#pragma comment(lib, "libcurl.lib")

class KeyAuth {
private:
    std::string app_name;
    std::string app_secret;
    std::string app_version;
    std::string api_url = "https://keyauth.win/api/1.2/";
    
    std::string session_id;
    std::string username;
    std::string subscription;
    std::string expiry;
    std::string hwid;
    
    bool initialized = false;
    bool logged_in = false;
    
    struct APIResponse {
        std::string data;
        long response_code;
    };
    
    static size_t WriteCallback(void* contents, size_t size, size_t nmemb, APIResponse* response) {
        size_t total_size = size * nmemb;
        response->data.append((char*)contents, total_size);
        return total_size;
    }
    
    APIResponse make_request(const std::string& endpoint, const std::map<std::string, std::string>& data) {
        CURL* curl;
        CURLcode res;
        APIResponse response;
        
        curl = curl_easy_init();
        if (curl) {
            std::string url = api_url + endpoint;
            std::string post_data = "";
            
            for (auto const& pair : data) {
                if (!post_data.empty()) post_data += "&";
                post_data += pair.first + "=" + pair.second;
            }
            
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, post_data.c_str());
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &response);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "KeyAuth");
            curl_easy_setopt(curl, CURLOPT_TIMEOUT, 30L);
            
            res = curl_easy_perform(curl);
            curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &response.response_code);
            curl_easy_cleanup(curl);
        }
        
        return response;
    }
    
    std::string get_hwid() {
        // Simple HWID generation based on system info
        // In production, use more sophisticated methods
        return "HWID-" + std::to_string(std::hash<std::string>{}("system_unique_id"));
    }
    
public:
    KeyAuth(const std::string& name, const std::string& secret, const std::string& version) 
        : app_name(name), app_secret(secret), app_version(version) {
        hwid = get_hwid();
    }
    
    struct AuthResult {
        bool success;
        std::string message;
        std::string username;
        std::string subscription;
        std::string expiry;
    };
    
    bool init() {
        std::map<std::string, std::string> data = {
            {"type", "init"},
            {"name", app_name},
            {"ownerid", app_secret},
            {"ver", app_version}
        };
        
        auto response = make_request("", data);
        
        if (response.response_code != 200) {
            return false;
        }
        
        try {
            Json::Value json_response;
            Json::Reader reader;
            
            if (!reader.parse(response.data, json_response)) {
                return false;
            }
            
            if (json_response["success"].asBool()) {
                session_id = json_response["sessionid"].asString();
                initialized = true;
                return true;
            }
        } catch (...) {
            return false;
        }
        
        return false;
    }
    
    AuthResult login(const std::string& user, const std::string& pass) {
        AuthResult result = {false, "", "", "", ""};
        
        if (!initialized) {
            result.message = "KeyAuth not initialized";
            return result;
        }
        
        std::map<std::string, std::string> data = {
            {"type", "login"},
            {"username", user},
            {"pass", pass},
            {"hwid", hwid},
            {"sessionid", session_id}
        };
        
        auto response = make_request("", data);
        
        if (response.response_code != 200) {
            result.message = "Network error";
            return result;
        }
        
        try {
            Json::Value json_response;
            Json::Reader reader;
            
            if (!reader.parse(response.data, json_response)) {
                result.message = "Invalid response format";
                return result;
            }
            
            if (json_response["success"].asBool()) {
                username = json_response["info"]["username"].asString();
                subscription = json_response["info"]["subscriptions"][0]["subscription"].asString();
                expiry = json_response["info"]["subscriptions"][0]["expiry"].asString();
                
                logged_in = true;
                result.success = true;
                result.username = username;
                result.subscription = subscription;
                result.expiry = expiry;
                result.message = "Login successful";
            } else {
                result.message = json_response["message"].asString();
            }
        } catch (...) {
            result.message = "Error parsing response";
        }
        
        return result;
    }
    
    AuthResult register_user(const std::string& user, const std::string& pass, const std::string& license) {
        AuthResult result = {false, "", "", "", ""};
        
        if (!initialized) {
            result.message = "KeyAuth not initialized";
            return result;
        }
        
        std::map<std::string, std::string> data = {
            {"type", "register"},
            {"username", user},
            {"pass", pass},
            {"key", license},
            {"hwid", hwid},
            {"sessionid", session_id}
        };
        
        auto response = make_request("", data);
        
        if (response.response_code != 200) {
            result.message = "Network error";
            return result;
        }
        
        try {
            Json::Value json_response;
            Json::Reader reader;
            
            if (!reader.parse(response.data, json_response)) {
                result.message = "Invalid response format";
                return result;
            }
            
            if (json_response["success"].asBool()) {
                result.success = true;
                result.message = "Registration successful";
            } else {
                result.message = json_response["message"].asString();
            }
        } catch (...) {
            result.message = "Error parsing response";
        }
        
        return result;
    }
    
    AuthResult license_login(const std::string& license) {
        AuthResult result = {false, "", "", "", ""};
        
        if (!initialized) {
            result.message = "KeyAuth not initialized";
            return result;
        }
        
        std::map<std::string, std::string> data = {
            {"type", "license"},
            {"key", license},
            {"hwid", hwid},
            {"sessionid", session_id}
        };
        
        auto response = make_request("", data);
        
        if (response.response_code != 200) {
            result.message = "Network error";
            return result;
        }
        
        try {
            Json::Value json_response;
            Json::Reader reader;
            
            if (!reader.parse(response.data, json_response)) {
                result.message = "Invalid response format";
                return result;
            }
            
            if (json_response["success"].asBool()) {
                username = json_response["info"]["username"].asString();
                subscription = json_response["info"]["subscriptions"][0]["subscription"].asString();
                expiry = json_response["info"]["subscriptions"][0]["expiry"].asString();
                
                logged_in = true;
                result.success = true;
                result.username = username;
                result.subscription = subscription;
                result.expiry = expiry;
                result.message = "License login successful";
            } else {
                result.message = json_response["message"].asString();
            }
        } catch (...) {
            result.message = "Error parsing response";
        }
        
        return result;
    }
    
    bool is_logged_in() const {
        return logged_in;
    }
    
    std::string get_username() const {
        return username;
    }
    
    std::string get_subscription() const {
        return subscription;
    }
    
    std::string get_expiry() const {
        return expiry;
    }
    
    std::string get_hwid() const {
        return hwid;
    }
    
    void logout() {
        logged_in = false;
        username.clear();
        subscription.clear();
        expiry.clear();
    }
};