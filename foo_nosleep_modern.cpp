// foo_nosleep_modern - modern foobar2000 component
// Prevents Windows system sleep while playback is active.

#include "pch.h"
#include <Resources.h>
#include <windows.h>
#include <foobar2000.h>

// foo_nosleep_modern - modern foobar2000 component
// Disable automatic system sleep while foobar2000 playback is running

class nosleep_callback : public play_callback_static {
    static bool g_active;

public:
    static void refresh( bool active ) {
        if( g_active == active ) return;

        g_active = active;

        if( active ) {
            ::SetThreadExecutionState( ES_CONTINUOUS | ES_SYSTEM_REQUIRED );
        }
        else {
            ::SetThreadExecutionState( ES_CONTINUOUS );
        }
    }

    ~nosleep_callback() noexcept {
        refresh( false );
    }

    unsigned get_flags() override {
        return
            flag_on_playback_starting |
            flag_on_playback_new_track |
            flag_on_playback_pause |
            flag_on_playback_stop;
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

    void on_playback_seek( double ) override {}

    void on_playback_pause( bool state ) override {
        refresh( !state );
    }

    void on_playback_edited( metadb_handle_ptr ) override {}

    void on_playback_dynamic_info( const file_info& ) override {}

    void on_playback_dynamic_info_track( const file_info& ) override {}

    void on_playback_time( double ) override {}

    void on_volume_change( float ) override {}
};

bool nosleep_callback::g_active = false;

static play_callback_static_factory_t<nosleep_callback> g_nosleep_callback_factory;

DECLARE_COMPONENT_VERSION(
    "NoSleep (modern)",
    STR_COMPONENT_VERSION_SHORT,
    "Disable automatic system sleep while foobar2000 playback is running"
);

VALIDATE_COMPONENT_FILENAME( "foo_nosleep_modern.dll" );

FOOBAR2000_IMPLEMENT_CFG_VAR_DOWNGRADE;
