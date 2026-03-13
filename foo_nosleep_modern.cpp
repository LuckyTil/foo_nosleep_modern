// foo_nosleep_modern - foobar2000 component
// Disable automatic system sleep while foobar2000 playback is running

#include "pch.h"
#include <Resources.h>
#include <windows.h>
#include <foobar2000.h>

class nosleep_callback : public play_callback_static {
    static bool g_active_SR;
    static bool g_active_ER;
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

public:
    static void refresh( bool active ) {
        if( active ) {
            if( !ensure_power_request() ) {
                return;
            }

            if( !g_active_SR ) {
                if( PowerSetRequest( g_power_request, PowerRequestSystemRequired ) ) {
                    g_active_SR = true;
                }
            }

            if( !g_active_ER ) {
                if( PowerSetRequest( g_power_request, PowerRequestExecutionRequired ) ) {
                    g_active_ER = true;
                }
            }
        }
        else {
            if( g_power_request == INVALID_HANDLE_VALUE ) {
                g_active_SR = false;
                g_active_ER = false;
                return;
            }

            if( g_active_SR ) {
                if( PowerClearRequest( g_power_request, PowerRequestSystemRequired ) ) {
                    g_active_SR = false;
                }
            }

            if( g_active_ER ) {
                if( PowerClearRequest( g_power_request, PowerRequestExecutionRequired ) ) {
                    g_active_ER = false;
                }
            }
        }
    }

    static void shutdown() {
        refresh( false );

        if( g_power_request != INVALID_HANDLE_VALUE ) {
            CloseHandle( g_power_request );
            g_power_request = INVALID_HANDLE_VALUE;
        }

        g_active_SR = false;
        g_active_ER = false;
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

bool nosleep_callback::g_active_SR = false;
bool nosleep_callback::g_active_ER = false;
HANDLE nosleep_callback::g_power_request = INVALID_HANDLE_VALUE;

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
        static_api_ptr_t<playback_control> pc;
        nosleep_callback::refresh( pc->is_playing() && !pc->is_paused() );
    }

    void on_quit() override {
        nosleep_callback::shutdown();
    }
};

#pragma warning(pop)

static play_callback_static_factory_t<nosleep_callback> g_nosleep_callback_factory;
static initquit_factory_t<nosleep_initquit> g_nosleep_initquit_factory;

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
