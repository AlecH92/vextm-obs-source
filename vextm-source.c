#include <obs-module.h>
#include <graphics/image-file.h>
#include <util/platform.h>
#include <util/dstr.h>
#include <sys/stat.h>
#include <pthread.h>
#include <windows.h>
#include "vextm-log.h"
#include "vextm-thread.h"
#include "vextm-source.h"

#define DEFAULT_DISPLAY_PATH "C:\\Program Files (x86)\\VEX\\" \
    "Tournament Manager\\Display.exe"

// Some custom screen IDs that represent combos (i.e. an instance of the
// plugin cofigured for one of these IDs will show any of the combo screens
// associated with the ID but no other screens).
#define INTRO_TIMER_COMBO 1000
#define RANKINGS_ALLIANCE_BRACKET_COMBO 1001

// Static helper methods defined at the end of the file
static char* get_dirname(char* path);
static void start_display(struct vextm_source_data* context, char* cmd);
static void stop_display(struct vextm_source_data* context);

// Function get_name returns the user-visible name for this input source type.
static const char* vextm_source_get_name(void* unused) {
    UNUSED_PARAMETER(unused);
    return obs_module_text("VexTmInputName");
}

// Function get_properties returns a list of configurable properties that the
// user can configure when setting up a source which uses this plugin.
static obs_properties_t* vextm_source_get_properties(void* data) {
    UNUSED_PARAMETER(data);
    //struct vextm_source_data* s = data;

    obs_properties_t* props = obs_properties_create();

    // Display executable path
    obs_properties_add_path(props, "display",
            obs_module_text("VexTmDisplayPath"),
            OBS_PATH_FILE, NULL, DEFAULT_DISPLAY_PATH);

    // Server address
    obs_properties_add_text(props, "server",
            obs_module_text("VexTmServerAddr"),
            OBS_TEXT_DEFAULT);

    // Password
    obs_properties_add_text(props, "password",
            obs_module_text("VexTmPassword"),
            OBS_TEXT_PASSWORD);

    // Screen type selection for this display instance
    obs_property_t* screen;
    screen = obs_properties_add_list(props, "screen",
            obs_module_text("VexTmScreenToShow"),
            OBS_COMBO_TYPE_LIST, OBS_COMBO_FORMAT_INT);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenAll"), 0);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenMatchIntroduction"), 2);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenMatchTimer"), 3);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenMatchResults"), 4);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenMatchIntroTimer"),
            INTRO_TIMER_COMBO);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenRankings"), 5);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenLogo"), 6);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenAllianceSelection"), 7);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenEliminationBracket"), 8);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenRankingsASBracket"),
            RANKINGS_ALLIANCE_BRACKET_COMBO);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenSkillsRankings"), 9);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenSkillsResults"), 10);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenSlidesAwards"), 12);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenSchedule"), 13);
    obs_property_list_add_int(screen,
            obs_module_text("VexTmScreenInspection"), 15);

    // Field set
    obs_properties_add_int(props, "fieldset",
            obs_module_text("VexTmFieldSet"),
            0, 20, 1);

    return props;
}

// Function get_defaults configures any default values in settings that make
// sense for the plugin.
static void vextm_source_get_defaults(obs_data_t* settings) {
    obs_data_set_default_string(settings, "display", DEFAULT_DISPLAY_PATH);
    obs_data_set_default_string(settings, "server", "");
    obs_data_set_default_string(settings, "password", "");
    obs_data_set_default_int(settings, "screen", 0);
    obs_data_set_default_int(settings, "fieldset", 0);
}

// Function update is called when any source settings are updated. For this
// plugin, vextm_source_create also calls this with the initial settings.
// When the plugin is starting, vextm_source_create is called first before
// update.
static void vextm_source_update(void* data, obs_data_t* settings) {
    struct vextm_source_data* context = data;
    long screen = obs_data_get_int(settings, "screen");
    const char* server = (char*) obs_data_get_string(settings, "server");
    const char* password = (char*) obs_data_get_string(settings, "password");
    long fieldset = obs_data_get_int(settings, "fieldset");

    char cmd[256];
    snprintf(cmd, 256, "%s --shmem %s --checkversion 0 --preview 0 --kiosk 1",
            obs_data_get_string(settings, "display"),
            context->shmem);

    if(strlen(server) > 0) {
        snprintf(cmd, 256, "%s --server %s", cmd, server);
    }

    if(strlen(password) > 0) {
        snprintf(cmd, 256, "%s --pw %s", cmd, password);
    }

    if(screen == INTRO_TIMER_COMBO) {
        // Special case for intro + in match
        snprintf(cmd, 256, "%s --onlyscreen 2 --onlyscreen 3", cmd);
    } else if(screen == RANKINGS_ALLIANCE_BRACKET_COMBO) {
        // Special case for rankings + alliance selection + elim bracket
        snprintf(cmd, 256, "%s --onlyscreen 5 --onlyscreen 7 " \
                "--onlyscreen 8 --onlyscreen 13", cmd);
    } else if(screen != 0) {
        // Pin to one screen only
        snprintf(cmd, 256, "%s --onlyscreen %ld", cmd, screen);
    }

    // Field set
    if(fieldset > 0) {
        snprintf(cmd, 256, "%s --fieldsetid %ld", cmd, fieldset);
    }

    info("Starting display with command: %s", cmd);

    if(context->run_thread != 0) {
        stop_display(context);
    }
    start_display(context, cmd);
}

// Function source_create runs once when the source instance is first created.
static void* vextm_source_create(obs_data_t* settings, obs_source_t* source) {
    struct vextm_source_data* context = bzalloc(sizeof(struct vextm_source_data));
    context->source = source;

    // Generate a unique name for the shared memory buffer
    for(int i = 0; i < 8; i++) {
        context->shmem[i] = (rand() % 26) + 'a';
    }
    context->shmem[8] = '\0';

    vextm_source_update(context, settings);

    return context;
}

// Function source_destroy runs once when the source instance is being
// destroyed (either the source was removed from OBS or the whole program is
// shutting down).
static void vextm_source_destroy(void* data) {
    struct vextm_source_data* context = data;

    stop_display(context);

    bfree(context);
}

// Struct obs_source_info provides function pointers to the various methods
// that are implemented by this plugin so they can be found by OBS.
static struct obs_source_info vextm_source_info = {
    .id             = "vextm_source",
    .type           = OBS_SOURCE_TYPE_INPUT,
    .output_flags   = OBS_SOURCE_ASYNC_VIDEO,
    .icon_type      = OBS_ICON_TYPE_DESKTOP_CAPTURE,
    .get_name       = vextm_source_get_name,
    .create         = vextm_source_create,
    .destroy        = vextm_source_destroy,
    .update         = vextm_source_update,
    .get_defaults   = vextm_source_get_defaults,
    .get_properties = vextm_source_get_properties
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("vextm-source", "en-US")

// Function obs_module_load is the entrypoint to the DLL that is called when
// the plugin is initialized.
bool obs_module_load(void) {
    obs_register_source(&vextm_source_info);
    return true;
}

// Function get_dirname strips the last component off the path in order to
// provide the directory name for the specified resource. This is a basic
// implementation that fails to handle a number of edge cases.
static char* get_dirname(char* path) {
    for(int i = strlen(path) - 1; i >= 0; i--) {
        if((path[i] == '\\') || (path[i] == '/')) {
            path[i] = '\0';
            return path;
        }
    }

    snprintf(path, strlen(path), ".");
    return path;
}

// Function start_display executes the display process and starts the
// background thread that will manage it.
static void start_display(struct vextm_source_data* context, char* cmd) {
    char* dir;
    char path[256];
    strncpy(path, cmd, 256);
    dir = get_dirname(path);

    // Start the TM display process
    context->run_thread = 1;
    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&(context->pi), sizeof(context->pi));
    if(CreateProcessA(NULL,
                cmd,
                NULL,
                NULL,
                FALSE,
                0,
                NULL,
                dir,
                &si,
                &(context->pi)) == 0) {
        warn("CreateProcess failed: %ld", GetLastError());
    }

    // Init a mutex and start a background thread that will manage the display
    // process just created
    int rc = pthread_mutex_init(&(context->mutex), NULL);
    if(rc != 0) {
        warn("Error creating mutex: %d", rc);
    } else {
        int rc = pthread_create(&(context->thread),
                NULL,
                vextm_source_thread,
                (void*) context);
        if(rc != 0) {
            warn("Error creating background thread: %d", rc);
        }
    }
}

// Function stop_display terminates the process and the background management
// thread.
static void stop_display(struct vextm_source_data* context) {
    TerminateProcess(context->pi.hProcess, 0);

    pthread_mutex_lock(&(context->mutex));
    context->run_thread = 0;
    pthread_mutex_unlock(&(context->mutex));

    int rc = pthread_join(context->thread, NULL);
    if(rc == 0) {
        info("Background thread join complete");
    } else {
        warn("Error joining background thread: %d", rc);
    }
}
