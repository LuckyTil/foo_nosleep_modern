// foo_nosleep_modern - foobar2000 component
// Disable automatic system sleep while foobar2000 playback is running

#include "pch.h"
#include <Resources.h>
#include <windows.h>
#include <foobar2000.h>
#include <SDK/cfg_var.h>
#include <SDK/preferences_page.h>
#include <helpers/atl-misc.h>
#include <helpers\DarkMode.h>

constexpr GUID guid_cfg_prevent_display_off = {
    0x778054fd, 0x6ecf, 0x4835, { 0x90, 0x7f, 0x82, 0x18, 0x70, 0x6d, 0x7b, 0x9a }
};
constexpr bool default_prevent_display_off = false;

cfg_bool cfg_prevent_display_off( guid_cfg_prevent_display_off, default_prevent_display_off );

class nosleep_runtime {
    static bool g_active_SR;
    static bool g_active_ER;
    static bool g_active_DR;
    static HANDLE g_power_request;

    static bool ensure_power_request() {
        if( g_power_request != INVALID_HANDLE_VALUE ) {
            return true;
        }

        REASON_CONTEXT rc = {};
        rc.Version = POWER_REQUEST_CONTEXT_VERSION;
        rc.Flags = POWER_REQUEST_CONTEXT_SIMPLE_STRING;
        rc.Reason.SimpleReasonString = const_cast<LPWSTR>(L"foobar2000 playback is active");

        g_power_request = PowerCreateRequest( &rc );
        return g_power_request != INVALID_HANDLE_VALUE;
    }

    static void refresh_main_thread( bool playback_active, bool prevent_display_off ) {
        const bool have_power_request = g_power_request != INVALID_HANDLE_VALUE || (playback_active &&
            ensure_power_request());

        if( playback_active ) {
            if( have_power_request && !g_active_SR
                && PowerSetRequest( g_power_request, PowerRequestSystemRequired ) ) {
                g_active_SR = true;
            }

            if( have_power_request && !g_active_ER
                && PowerSetRequest( g_power_request, PowerRequestExecutionRequired ) ) {
                g_active_ER = true;
            }

            if( prevent_display_off ) {
                if( have_power_request && !g_active_DR
                    && PowerSetRequest( g_power_request, PowerRequestDisplayRequired ) ) {
                    g_active_DR = true;
                }
            }
            else if( have_power_request && g_active_DR
                && PowerClearRequest( g_power_request, PowerRequestDisplayRequired ) ) {
                g_active_DR = false;
            }
        }
        else {
            if( g_power_request == INVALID_HANDLE_VALUE ) {
                g_active_SR = false;
                g_active_ER = false;
                g_active_DR = false;
            }
            else {
                if( g_active_SR && PowerClearRequest( g_power_request, PowerRequestSystemRequired ) ) {
                    g_active_SR = false;
                }

                if( g_active_ER && PowerClearRequest( g_power_request, PowerRequestExecutionRequired ) ) {
                    g_active_ER = false;
                }

                if( g_active_DR && PowerClearRequest( g_power_request, PowerRequestDisplayRequired ) ) {
                    g_active_DR = false;
                }
            }
        }

        // Keep display idle and screensaver timers from expiring while playback is active.
        SetThreadExecutionState( playback_active && prevent_display_off
                                     ? ES_CONTINUOUS | ES_DISPLAY_REQUIRED
                                     : ES_CONTINUOUS );
    }

public:
    static void refresh( bool playback_active ) {
        const bool prevent_display_off = cfg_prevent_display_off.get();
        fb2k::inMainThread2( [playback_active, prevent_display_off]
        {
            refresh_main_thread( playback_active, prevent_display_off );
        } );
    }

    static void refresh_from_playback_state() {
        static_api_ptr_t<playback_control> pc;
        refresh( pc->is_playing() && !pc->is_paused() );
    }

    static void shutdown() {
        fb2k::inMainThreadSynchronous2( []
        {
            refresh_main_thread( false, false );

            if( g_power_request != INVALID_HANDLE_VALUE ) {
                CloseHandle( g_power_request );
                g_power_request = INVALID_HANDLE_VALUE;
            }

            g_active_SR = false;
            g_active_ER = false;
            g_active_DR = false;
        } );
    }
};

bool nosleep_runtime::g_active_SR = false;
bool nosleep_runtime::g_active_ER = false;
bool nosleep_runtime::g_active_DR = false;
HANDLE nosleep_runtime::g_power_request = INVALID_HANDLE_VALUE;

class nosleep_callback : public play_callback_static {
public:
    static void refresh( bool active ) {
        nosleep_runtime::refresh( active );
    }

    static void shutdown() {
        nosleep_runtime::shutdown();
    }

    virtual ~nosleep_callback() noexcept {
        refresh( false );
    }

    unsigned get_flags() override {
        return flag_on_playback_starting
            | flag_on_playback_new_track
            | flag_on_playback_pause
            | flag_on_playback_stop;
    }

    void on_playback_starting( play_control::t_track_command, bool paused ) override {
        refresh( !paused );
    }

    void on_playback_new_track( metadb_handle_ptr ) override {
        refresh( true );
    }

    void on_playback_stop( play_control::t_stop_reason ) override {
        refresh( false );
    }

    void on_playback_seek( double ) override {
    }

    void on_playback_pause( bool state ) override {
        refresh( !state );
    }

    void on_playback_edited( metadb_handle_ptr ) override {
    }

    void on_playback_dynamic_info( const file_info& ) override {
    }

    void on_playback_dynamic_info_track( const file_info& ) override {
    }

    void on_playback_time( double ) override {
    }

    void on_volume_change( float ) override {
    }
};

#pragma warning(push)
#pragma warning(disable: 4265)

class nosleep_initquit : public initquit {
public:
    nosleep_initquit() = default;
    nosleep_initquit( const nosleep_initquit& ) = delete;
    nosleep_initquit& operator=( const nosleep_initquit& ) = delete;
    nosleep_initquit( nosleep_initquit&& ) = delete;
    nosleep_initquit& operator=( nosleep_initquit&& ) = delete;

    void on_init() override {
        nosleep_runtime::refresh_from_playback_state();
    }

    void on_quit() override {
        nosleep_callback::shutdown();
    }
};

#pragma warning(pop)

class nosleep_preferences_page_instance : public CDialogImpl<nosleep_preferences_page_instance>,
                                          public preferences_page_instance {
public:
    nosleep_preferences_page_instance() = default;
    nosleep_preferences_page_instance( const nosleep_preferences_page_instance& ) = delete;
    nosleep_preferences_page_instance& operator=( const nosleep_preferences_page_instance& ) = delete;
    nosleep_preferences_page_instance( nosleep_preferences_page_instance&& ) = delete;
    nosleep_preferences_page_instance& operator=( nosleep_preferences_page_instance&& ) = delete;

    nosleep_preferences_page_instance( HWND parent, preferences_page_callback::ptr callback )
        : m_callback( std::move( callback ) ) {
        PFC_ASSERT( Create( parent ) != NULL );
    }

    enum { IDD = IDD_PREFERENCES };

    BEGIN_MSG_MAP_EX( nosleep_preferences_page_instance )
        MSG_WM_INITDIALOG( OnInitDialog )
        COMMAND_HANDLER_EX( IDC_PREVENT_DISPLAY_OFF, BN_CLICKED, OnCheckboxClicked )
    END_MSG_MAP()

    fb2k::hwnd_t get_wnd() override {
        return m_hWnd;
    }

    t_uint32 get_state() override {
        t_uint32 state = preferences_state::resettable | preferences_state::dark_mode_supported;
        if( has_changed() ) {
            state |= preferences_state::changed;
        }
        return state;
    }

    void apply() override {
        cfg_prevent_display_off = is_prevent_display_off_checked();
        nosleep_runtime::refresh_from_playback_state();
        on_changed();
    }

    void reset() override {
        CheckDlgButton( IDC_PREVENT_DISPLAY_OFF, default_prevent_display_off ? BST_CHECKED : BST_UNCHECKED );
        on_changed();
    }

private:
    BOOL OnInitDialog( CWindow, LPARAM ) {
        CheckDlgButton( IDC_PREVENT_DISPLAY_OFF, cfg_prevent_display_off ? BST_CHECKED : BST_UNCHECKED );
        return FALSE;
    }

    void OnCheckboxClicked( UINT, int, CWindow ) {
        on_changed();
    }

    bool is_prevent_display_off_checked() {
        return IsDlgButtonChecked( IDC_PREVENT_DISPLAY_OFF ) == BST_CHECKED;
    }

    bool has_changed() {
        return is_prevent_display_off_checked() != static_cast<bool>(cfg_prevent_display_off);
    }

    void on_changed() {
        m_callback->on_state_changed();
    }

    const preferences_page_callback::ptr m_callback;
};

#pragma warning(push)
#pragma warning(disable: 4265)

class nosleep_preferences_page : public preferences_page_v4 {
public:
    nosleep_preferences_page() = default;
    nosleep_preferences_page( const nosleep_preferences_page& ) = delete;
    nosleep_preferences_page& operator=( const nosleep_preferences_page& ) = delete;
    nosleep_preferences_page( nosleep_preferences_page&& ) = delete;
    nosleep_preferences_page& operator=( nosleep_preferences_page&& ) = delete;

    preferences_page_instance::ptr
    instantiate( fb2k::hwnd_t parent, preferences_page_callback::ptr callback ) override {
        return fb2k::service_new_window<nosleep_preferences_page_instance>( parent, callback );
    }

    const char* get_name() override {
        return STR_COMPONENT_NAME;
    }

    GUID get_guid() override {
        return GUID_PREFERENCES;
    }

    GUID get_parent_guid() override {
        return guid_tools;
    }
};

#pragma warning(pop)

static play_callback_static_factory_t<nosleep_callback> g_nosleep_callback_factory;
static initquit_factory_t<nosleep_initquit> g_nosleep_initquit_factory;
static preferences_page_factory_t<nosleep_preferences_page> g_nosleep_preferences_page_factory;

#pragma warning(push)
#pragma warning(disable: 4265 5026 5027)

DECLARE_COMPONENT_VERSION(
    "NoSleep (modern)",
    STR_COMPONENT_VERSION_SHORT,
    "Disable automatic system sleep while foobar2000 playback is running."
);

#pragma warning(pop)

VALIDATE_COMPONENT_FILENAME( "foo_nosleep_modern.dll" );

FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
