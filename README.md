# ImGui KeyAuth Integration

This project integrates KeyAuth authentication system into an ImGui-based application, removing hardcoded authentication and implementing proper user management.

## Dependencies

Before building, ensure you have the following dependencies installed:

### Required Libraries
- **libcurl** - For HTTP requests to KeyAuth API
- **jsoncpp** - For JSON parsing of API responses
- **ImGui** - GUI framework (already included)
- **DirectX 11** - Graphics API (Windows)

### Windows Setup (Visual Studio)

1. **Install vcpkg** (if not already installed):
   ```bash
   git clone https://github.com/Microsoft/vcpkg.git
   cd vcpkg
   .\bootstrap-vcpkg.bat
   .\vcpkg integrate install
   ```

2. **Install required packages**:
   ```bash
   .\vcpkg install curl:x64-windows
   .\vcpkg install jsoncpp:x64-windows
   ```

3. **Add to your project**:
   - Link libraries: `libcurl.lib`, `jsoncpp.lib`
   - Include directories for curl and jsoncpp headers

## Configuration

1. **Update KeyAuth Settings** in `config.hpp`:
   ```cpp
   namespace Config {
       // Replace with your actual KeyAuth application details
       inline const std::string KEYAUTH_APP_NAME = "YourAppName";
       inline const std::string KEYAUTH_APP_SECRET = "YourOwnerID"; 
       inline const std::string KEYAUTH_APP_VERSION = "1.0";
   }
   ```

2. **Get your KeyAuth credentials**:
   - Create an account at [KeyAuth.win](https://keyauth.win)
   - Create a new application
   - Copy your application name and owner ID
   - Set up your licensing system

## Features

### Authentication
- Username/Password login
- License key authentication
- Session management
- Automatic session validation
- Logout functionality

### Security
- Hardware ID binding
- Subscription validation
- Expiry date checking
- Network error handling

### User Interface
- Clean login interface
- Real-time authentication status
- Error message display
- User information display

## Usage

1. **First Run**: Configure your KeyAuth credentials in `config.hpp`
2. **Login**: Users can authenticate using either:
   - Username and password (if registered)
   - License key (for instant access)
3. **Session**: Once authenticated, user data is displayed and validated
4. **Logout**: Users can safely logout and return to login screen

## Security Notes

- Never hardcode credentials in your source code
- Always validate user sessions before accessing protected features
- Use HTTPS for all KeyAuth API communications (handled automatically)
- Consider implementing additional security measures for sensitive applications

## Troubleshooting

### Common Issues

1. **"Failed to initialize KeyAuth"**:
   - Check internet connection
   - Verify KeyAuth credentials in config.hpp
   - Ensure libcurl is properly linked

2. **"Network error"**:
   - Check firewall settings
   - Verify SSL certificates are up to date
   - Ensure KeyAuth servers are accessible

3. **"Invalid response format"**:
   - Check if jsoncpp is properly linked
   - Verify KeyAuth API is responding correctly

### Build Errors

1. **Missing curl headers**:
   ```
   Solution: Install libcurl development package
   vcpkg install curl:x64-windows
   ```

2. **Missing json headers**:
   ```
   Solution: Install jsoncpp development package
   vcpkg install jsoncpp:x64-windows
   ```

## File Structure

```
├── main.hpp          # Main application with KeyAuth integration
├── keyauth.hpp       # KeyAuth API wrapper class
├── config.hpp        # Configuration settings
├── ui/              # UI framework files
└── README.md        # This file
```

## License

This integration code is provided as-is for educational purposes. Make sure to comply with KeyAuth's terms of service and your own application's licensing requirements.