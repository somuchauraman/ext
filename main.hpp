#pragma once
#include "ui/ui.hpp"
#include "keyauth.hpp"
#include "config.hpp"
#include <d3d11.h>
#include <d3dcompiler.h>
#pragma comment(lib, "d3d11.lib")
#pragma comment(lib, "d3dcompiler.lib")
#pragma comment(lib, "dxgi.lib")
using namespace ui;

#include "imgui/imgui_freetype.h"

inline static ID3D11Device* g_pd3dDevice = nullptr;
inline static ID3D11DeviceContext* g_pd3dDeviceContext = nullptr;
inline static IDXGISwapChain* g_pSwapChain = nullptr;
inline static bool g_SwapChainOccluded = false;
inline static UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
inline static ID3D11RenderTargetView* g_mainRenderTargetView = nullptr;
void CreateRenderTarget( );
void CleanupRenderTarget( );

class c_main {
private:
    KeyAuth keyauth{Config::KEYAUTH_APP_NAME, Config::KEYAUTH_APP_SECRET, Config::KEYAUTH_APP_VERSION};
    bool keyauth_initialized = false;
    KeyAuth::AuthResult current_user;

public:
    void initialize( ImGuiIO& io ) {
        // Initialize KeyAuth
        keyauth_initialized = keyauth.init();
        if (!keyauth_initialized) {
            // Handle initialization error
            MessageBoxA(NULL, "Failed to initialize KeyAuth. Check your internet connection.", "Error", MB_OK | MB_ICONERROR);
        }

        StyleColorsDark( );

        auto style = &ImGui::GetStyle( );
        style->AntiAliasedFill = false;

        auto config = ImFontConfig( );
        config.FontDataOwnedByAtlas = false;
        config.FontBuilderFlags |= ImGuiFreeTypeBuilderFlags::ImGuiFreeTypeBuilderFlags_MonoHinting;

        fonts[font].set_data( Config::FONT_PATH.c_str() );
        fonts[font].set_config( config );
        fonts[font].init( { Config::FONT_SIZE } );

        ImGuiFreeType::BuildFontAtlas( io.Fonts );

        add_page( 0, [this]( ){
            static char username_buf[64] = "";
            static char password_buf[64] = "";
            static char license_buf[128] = "";
            static int login_mode = 0; // 0 = username/password, 1 = license key
            static bool save_credentials = false;
            static std::string error_msg = "";
            static bool is_logging_in = false;

            BeginGroup( ); {
                begin_child( "Authentication" ); {
                    PushStyleVar( ImGuiStyleVar_ItemSpacing, { 11, 9 } );
                    PushStyleVar( ImGuiStyleVar_WindowPadding, { 15, 13 } );
                    BeginChild( "auth wrapper", GetContentRegionAvail( ), 0, ImGuiWindowFlags_AlwaysUseWindowPadding ); {
                        
                        // Login mode selection
                        if (RadioButton("Username/Password", login_mode == 0)) login_mode = 0;
                        SameLine();
                        if (RadioButton("License Key", login_mode == 1)) login_mode = 1;
                        
                        Dummy({ 0, 5 });

                        if (login_mode == 0) {
                            // Username/Password login
                            InputText( "Username", username_buf, sizeof( username_buf ) );
                            InputText( "Password", password_buf, sizeof( password_buf ), ImGuiInputTextFlags_Password );
                        } else {
                            // License key login
                            InputText( "License Key", license_buf, sizeof( license_buf ) );
                        }
                        
                        Dummy({ 0, 0 });

                        // Display error message if any
                        if ( !error_msg.empty() ) {
                            PushStyleColor(ImGuiCol_Text, IM_COL32(255, 100, 100, 255));
                            add_text( font, Config::FONT_SIZE, { GetWindowPos( ).x + GetCursorPosX( ) + CalcItemWidth( ) / 2 - text_size( font, Config::FONT_SIZE, error_msg.c_str() ).x / 2, GetWindowPos( ).y + GetCursorPosY( ) - Config::FONT_SIZE - GImGui->Style.ItemSpacing.y / 2 + 1 }, GetColorU32( ImGuiCol_Text ), error_msg.c_str(), FindRenderedTextEnd( error_msg.c_str() ) );
                            PopStyleColor();
                        }

                        // Login button
                        PushItemFlag(ImGuiItemFlags_Disabled, is_logging_in || !keyauth_initialized);
                        if ( Button( is_logging_in ? "Logging in..." : "Log in", { CalcItemWidth( ), 24 } ) ) {
                            is_logging_in = true;
                            error_msg = "";
                            
                            KeyAuth::AuthResult result;
                            if (login_mode == 0) {
                                // Username/Password login
                                if (strlen(username_buf) == 0 || strlen(password_buf) == 0) {
                                    error_msg = "Please enter username and password";
                                    is_logging_in = false;
                                } else {
                                    result = keyauth.login(username_buf, password_buf);
                                }
                            } else {
                                // License key login
                                if (strlen(license_buf) == 0) {
                                    error_msg = "Please enter license key";
                                    is_logging_in = false;
                                } else {
                                    result = keyauth.license_login(license_buf);
                                }
                            }
                            
                            if (is_logging_in) {
                                if (result.success) {
                                    current_user = result;
                                    cur_page = 1;
                                    error_msg = "";
                                } else {
                                    error_msg = result.message;
                                }
                                is_logging_in = false;
                            }
                        }
                        PopItemFlag();
                        
                        if ( Button( "Exit", { CalcItemWidth( ), 24 } ) ) {
                            exit( 0 );
                        }
                        
                        Checkbox( "Save credentials", &save_credentials );
                        
                        if (!keyauth_initialized) {
                            Dummy({ 0, 5 });
                            PushStyleColor(ImGuiCol_Text, IM_COL32(255, 200, 100, 255));
                            TextWrapped("KeyAuth initialization failed. Please check your internet connection and app configuration.");
                            PopStyleColor();
                        }
                    }
                    EndChild( );
                    PopStyleVar( 2 );
                }
                end_child( );
            }
            EndGroup( );
        } );

        add_page( 1, [this]( ){
            // Validate user session
            if (!keyauth.is_logged_in()) {
                cur_page = 0; // Redirect to login
                return;
            }
            static int cur_game = 0;
            static std::vector< const char* > games = { "ROBLOX" };

            static int percent = 0;
            const int delay_ms = 50;
            static int cur_stage = 0;
            static std::vector< const char* > stages;
            static auto last_update = std::chrono::steady_clock::now( );

            auto CenterText = [&]( const char* text ) {
                SetCursorPosX( GetCursorPosX( ) + CalcItemWidth( ) / 2 - text_size( font, 13, text ).x / 2 );
                TextUnformatted( text );
            };
            
            auto CenterTextF = [&]( const char* fmt, ... ) {
                char buf[256];
                va_list args;
                va_start( args, fmt );
                vsnprintf( buf, sizeof( buf ), fmt, args );
                va_end( args );
                CenterText( buf );
            };

            BeginGroup( ); {
                begin_child( "Cheat selection" ); {
                    PushStyleVar( ImGuiStyleVar_ItemSpacing, { 11, 9 } );
                    PushStyleVar( ImGuiStyleVar_WindowPadding, { 15, 7 } );
                    BeginChild( "cheat selection wrapper", GetContentRegionAvail( ), 0, ImGuiWindowFlags_AlwaysUseWindowPadding ); {
                        PushItemFlag( ImGuiItemFlags_Disabled, stages.size( ) > 0 );

                        BeginChild( "cheat list", { CalcItemWidth( ), 100 }, 0, ImGuiWindowFlags_NoBackground ); {
                            GetWindowDrawList( )->AddRectFilledMultiColor( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), GetColorU32( ImGuiCol_FrameBg ), GetColorU32( ImGuiCol_FrameBg ), GetColorU32( ImGuiCol_FrameBg2 ), GetColorU32( ImGuiCol_FrameBg2 ) );
                            GetWindowDrawList( )->AddRect( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), GetColorU32( ImGuiCol_BorderShadow ) );

                            PushStyleVar( ImGuiStyleVar_ItemSpacing, { 0, 0 } );
                            BeginGroup( ); {
                                for ( int i = 0; i < games.size( ); ++i ) {
                                    if ( Selectable( games[i], cur_game == i, ImGuiSelectableFlags_NoPadWithHalfSpacing, { CalcItemWidth( ), 20 } ) )
                                        cur_game = i;
                                }
                            }
                            EndGroup( );
                            PopStyleVar( );
                        }
                        EndChild( );

                        if ( Button( "Load cheat", { CalcItemWidth( ), 24 } ) ) {
                            percent = 0;
                            cur_stage = 0;
                            last_update = std::chrono::steady_clock::now( );
                            stages = { "Resolving imports", "Retrieving data" };
                        }
                        
                        if ( Button( "Logout", { CalcItemWidth( ), 24 } ) ) {
                            keyauth.logout();
                            cur_page = 0;
                        }

                        PopItemFlag( );
                        
                        if (keyauth.is_logged_in()) {
                            CenterTextF( "Welcome back, %s", current_user.username.c_str() );
                            CenterTextF( "Subscription: %s", current_user.subscription.c_str() );
                            CenterTextF( "Expires: %s", current_user.expiry.c_str() );
                        } else {
                            CenterText( "Not logged in" );
                        }

                        if ( !stages.empty( ) ) {
                            auto now = std::chrono::steady_clock::now( );
                            auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>( now - last_update ).count( );

                            if ( percent < 100 && elapsed >= delay_ms ) {
                                percent++;
                                last_update = now;
                            }

                            if ( percent >= 100 ) {
                                if ( cur_stage < stages.size( ) - 1 ) {
                                    cur_stage++;
                                    percent = 0;
                                    last_update = now;
                                } else {
                                    stages.clear( );
                                    cur_stage = 0;
                                    percent = 0;
                                }
                            }

                            if ( !stages.empty( ) ) {
                                CenterTextF( "%s [%d%%]", stages[cur_stage], percent );
                            }
                        }
                    }
                    EndChild( );
                    PopStyleVar( 2 );
                }
                end_child( );
            }
            EndGroup( );
        } );
    }

    void render( ) {
        SetNextWindowPos({ 0, 0 });
        SetNextWindowSize( ui::size );
        Begin( "imgui base", 0, ImGuiWindowFlags_NoDecoration ); {
            GetWindowDrawList( )->AddRect( GetWindowPos( ), GetWindowPos( ) + GetWindowSize( ), GetColorU32( ImGuiCol_BorderShadow ) );
            GetWindowDrawList( )->AddRect( { GetWindowPos( ).x + 1, GetWindowPos( ).y + 1 }, { GetWindowPos( ).x + GetWindowWidth( ) - 1, GetWindowPos( ).y + GetWindowHeight( ) - 1 }, GetColorU32( ImGuiCol_Border ) );

            SetCursorPos({ 2, 2 });
            BeginChild( "header", { GetWindowWidth( ) - 4, 26 } ); {
                GetWindowDrawList( )->AddQuadFilled( { GetWindowPos( ).x + 60, GetWindowPos( ).y }, { GetWindowPos( ).x + 30, GetWindowPos( ).y }, { GetWindowPos( ).x + 30, GetWindowPos( ).y + GetWindowHeight( ) }, { GetWindowPos( ).x + 50, GetWindowPos( ).y + GetWindowHeight( ) }, GetColorU32( ImGuiCol_Scheme, 0.03f ) );
                GetWindowDrawList( )->AddQuadFilled( { GetWindowPos( ).x + 175, GetWindowPos( ).y }, { GetWindowPos( ).x + 130, GetWindowPos( ).y }, { GetWindowPos( ).x + 120, GetWindowPos( ).y + GetWindowHeight( ) }, { GetWindowPos( ).x + 165, GetWindowPos( ).y + GetWindowHeight( ) }, GetColorU32( ImGuiCol_Scheme, 0.03f ) );

                GetWindowDrawList( )->AddLine( { GetWindowPos( ).x, GetWindowPos( ).y + GetWindowHeight( ) - 3 }, { GetWindowPos( ).x + GetWindowWidth( ), GetWindowPos( ).y + GetWindowHeight( ) - 3 }, GetColorU32( ImGuiCol_BorderShadow ) );
                GetWindowDrawList( )->AddLine( { GetWindowPos( ).x, GetWindowPos( ).y + GetWindowHeight( ) - 2 }, { GetWindowPos( ).x + GetWindowWidth( ), GetWindowPos( ).y + GetWindowHeight( ) - 2 }, GetColorU32( ImGuiCol_Border ) );
                GetWindowDrawList( )->AddRectFilledMultiColor( { GetWindowPos( ).x + GetWindowWidth( ) / 2, GetWindowPos( ).y + GetWindowHeight( ) - 1 }, { GetWindowPos( ).x + GetWindowWidth( ), GetWindowPos( ).y + GetWindowHeight( ) }, GetColorU32( ImGuiCol_Scheme, 0 ), GetColorU32( ImGuiCol_Scheme ), GetColorU32( ImGuiCol_Scheme ), GetColorU32( ImGuiCol_Scheme, 0 ) );

                add_text( font, Config::FONT_SIZE, { GetWindowPos( ).x + 6, GetWindowPos( ).y + GetWindowHeight( ) / 2 - Config::FONT_SIZE / 2 - 2 }, GetColorU32( ImGuiCol_Scheme ), Config::APP_TITLE.c_str() );
            }
            EndChild( );

            SetCursorPos({ 15, 38 });
            PushStyleVar( ImGuiStyleVar_ItemSpacing, { 15, 15 } );
            BeginChild( "main wrapper", { GetWindowWidth( ) - 30, GetWindowHeight( ) - 74 }, 0, ImGuiWindowFlags_NoBackground ); {
                PushStyleVar( ImGuiStyleVar_ItemSpacing, { 3, 3 } );
                PushStyleVar( ImGuiStyleVar_WindowPadding, { 8, 8 } );
                BeginChild( "pages", { GetWindowWidth( ) / 3, GetWindowHeight( ) }, 1, ImGuiWindowFlags_AlwaysUseWindowPadding ); {
                    for ( int i = 0; i < pages.size( ); ++i )
                        page( pages[i], cur_page == i );
                }
                EndChild( );
                PopStyleVar( 2 );

                SameLine( );

                BeginChild( "main", { GetContentRegionAvail( ).x, GetWindowHeight( ) }, 0, ImGuiWindowFlags_NoBackground ); {
                    render_page( );
                }
                EndChild( );
            }
            EndChild( );
            PopStyleVar( );

            SetCursorPos({ 2, GetWindowHeight( ) - 24 });
            BeginChild( "footer", { GetWindowWidth( ) - 4, 22 } ); {
                GetWindowDrawList( )->AddLine( GetWindowPos( ), { GetWindowPos( ).x + GetWindowWidth( ), GetWindowPos( ).y }, GetColorU32( ImGuiCol_Border ) );
                GetWindowDrawList( )->AddLine( { GetWindowPos( ).x, GetWindowPos( ).y + 1 }, { GetWindowPos( ).x + GetWindowWidth( ), GetWindowPos( ).y + 1 }, GetColorU32( ImGuiCol_BorderShadow ) );

                // Get current date
                time_t now = time(0);
                tm* ltm = localtime(&now);
                char date_str[32];
                strftime(date_str, sizeof(date_str), "%b %d %Y", ltm);
                
                add_text( font, Config::FONT_SIZE, { GetWindowPos( ).x + 6, GetWindowPos( ).y + GetWindowHeight( ) / 2 - Config::FONT_SIZE / 2 - 1 }, GetColorU32( ImGuiCol_TextDisabled ), date_str );
            }
            EndChild( );
        }
        End( );
    }
};

class c_frame {
public:
    c_frame( ) {
        if ( init_fonts ) {
            auto& io = GetIO( );
            io.Fonts->Clear( );

            for ( int i = 0; i < fonts.size( ); ++i ) {
                fonts[i].get_fonts( ).clear( );

                for ( int f = 0; f < fonts[i].should_init.size( ); ++f ) {
                    fonts[i].init( { fonts[i].should_init[f] }, false );
                }
            }

            ImGuiFreeType::BuildFontAtlas( io.Fonts );
            ImGui_ImplDX11_CreateDeviceObjects( );

            init_fonts = false;
        }

        ImGui_ImplDX11_NewFrame( );
        ImGui_ImplWin32_NewFrame( );
        NewFrame( );
    }

    ~c_frame( ) {
        Render( );
        const float clear_c[4] = { GetStyleColorVec4( ImGuiCol_WindowBg ).x, GetStyleColorVec4( ImGuiCol_WindowBg ).y, GetStyleColorVec4( ImGuiCol_WindowBg ).z, GetStyleColorVec4( ImGuiCol_WindowBg ).w };
        g_pd3dDeviceContext->OMSetRenderTargets( 1, &g_mainRenderTargetView, nullptr );
        g_pd3dDeviceContext->ClearRenderTargetView( g_mainRenderTargetView, clear_c );
        ImGui_ImplDX11_RenderDrawData( GetDrawData( ) );

        HRESULT hr = g_pSwapChain->Present( 1, 0 ); // Present with vsync
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }
};

class c_context {
public:
    c_context( HWND hwnd ) {
        CreateContext( );
        ImGui_ImplWin32_Init( hwnd );
        ImGui_ImplDX11_Init( g_pd3dDevice, g_pd3dDeviceContext );
    }

    ~c_context( ) {
        ImGui_ImplDX11_Shutdown( );
        ImGui_ImplWin32_Shutdown( );
        DestroyContext( );
    }
};

bool CreateDeviceD3D( HWND hWnd ) {
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory( &sd, sizeof( sd ) );
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext );
    if ( res == DXGI_ERROR_UNSUPPORTED )
        res = D3D11CreateDeviceAndSwapChain( nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext );
    if ( res != S_OK )
        return false;

    CreateRenderTarget( );
    return true;
}

void CleanupDeviceD3D( ) {
    CleanupRenderTarget( );
    if ( g_pSwapChain ) { g_pSwapChain->Release( ); g_pSwapChain = nullptr; }
    if ( g_pd3dDeviceContext ) { g_pd3dDeviceContext->Release( ); g_pd3dDeviceContext = nullptr; }
    if ( g_pd3dDevice ) { g_pd3dDevice->Release( ); g_pd3dDevice = nullptr; }
}

void CreateRenderTarget( ) {
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer( 0, IID_PPV_ARGS( &pBackBuffer ) );
    g_pd3dDevice->CreateRenderTargetView( pBackBuffer, nullptr, &g_mainRenderTargetView );
    pBackBuffer->Release( );
}

void CleanupRenderTarget( ) {
    if ( g_mainRenderTargetView ) { g_mainRenderTargetView->Release( ); g_mainRenderTargetView = nullptr; }
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam );

LRESULT WINAPI WndProc( HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam ) {
    if ( ImGui_ImplWin32_WndProcHandler( hWnd, msg, wParam, lParam ) )
        return true;

    switch ( msg ) {
    case WM_SIZE:
        if ( wParam == SIZE_MINIMIZED )
            return 0;
        g_ResizeWidth = (UINT)LOWORD( lParam );
        g_ResizeHeight = (UINT)HIWORD( lParam );
        return 0;
    case WM_SYSCOMMAND:
        if ( ( wParam & 0xfff0 ) == SC_KEYMENU )
            return 0;
        break;
    case WM_DESTROY:
        ::PostQuitMessage( 0 );
        return 0;
    }
    return ::DefWindowProcW( hWnd, msg, wParam, lParam );
}
