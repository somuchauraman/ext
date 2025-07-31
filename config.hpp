#pragma once
#include <string>

namespace Config {
    // KeyAuth Application Settings
    // Replace these with your actual KeyAuth application details
    inline const std::string KEYAUTH_APP_NAME = "YourAppName";
    inline const std::string KEYAUTH_APP_SECRET = "YourAppSecret"; // This is the Owner ID from KeyAuth
    inline const std::string KEYAUTH_APP_VERSION = "1.0";
    
    // Application Settings
    inline const std::string APP_TITLE = "pooron.solutions";
    inline const int WINDOW_WIDTH = 800;
    inline const int WINDOW_HEIGHT = 600;
    
    // UI Settings
    inline const std::string FONT_PATH = "C:\\Windows\\Fonts\\tahoma.ttf";
    inline const int FONT_SIZE = 13;
    
    // Session Settings
    inline const bool REMEMBER_CREDENTIALS = true;
    inline const std::string CREDENTIALS_FILE = "saved_creds.dat";
}