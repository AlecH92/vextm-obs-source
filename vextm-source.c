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

static const char *vextm_source_get_name(void *unused)
{
	UNUSED_PARAMETER(unused);
	return obs_module_text("VexTmInputName");
}

static obs_properties_t *vextm_source_get_properties(void *data)
{
	UNUSED_PARAMETER(data);
	//struct vextm_source_data* s = data;

	obs_properties_t *props = obs_properties_create();

	return props;
}

static void vextm_source_get_defaults(obs_data_t *settings)
{
	obs_data_set_default_bool(settings, "unload", false);
}

static void vextm_source_update(void *data, obs_data_t *settings)
{
	UNUSED_PARAMETER(data);
	UNUSED_PARAMETER(settings);
	//struct vextm_source_data* context = data;
	//const char *file = obs_data_get_string(settings, "file");
}

static PROCESS_INFORMATION pi;

static void *vextm_source_create(obs_data_t *settings, obs_source_t *source)
{
	struct vextm_source_data* context = bzalloc(sizeof(struct vextm_source_data));
	context->source = source;
    context->run_thread = 1;

    // Generate a unique name for the shared memory buffer
    for(int i = 0; i < 8; i++)
    {
        context->shmem[i] = (rand() % 26) + 'a';
    }
    context->shmem[8] = '\0';

    char prg[128];
    snprintf(prg, 128, "C:\\Program Files (x86)\\VEX\\Tournament Manager\\" \
            "Display.exe --shmem %s --checkversion 0", context->shmem);

    STARTUPINFO si;
    ZeroMemory(&si, sizeof(si));
    ZeroMemory(&pi, sizeof(pi));
    if(CreateProcessA(NULL,
            prg,
            NULL,
            NULL,
            FALSE,
            0,
            NULL,
            "C:\\Program Files (x86)\\VEX\\Tournament Manager\\",
            &si,
            &pi) == 0)
    {
        warn("CreateProcess failed: %ld", GetLastError());
    }

    int rc = pthread_mutex_init(&(context->mutex), NULL);
    if(rc != 0)
    {
        warn("Error creating mutex: %d", rc);
    }
    else
    {
        int rc = pthread_create(&(context->thread),
                NULL,
                vextm_source_thread,
                (void*) context);
        if(rc != 0)
        {
            warn("Error creating background thread: %d", rc);
        }
    }

	vextm_source_update(context, settings);
	return context;
}

static void vextm_source_destroy(void *data)
{
	struct vextm_source_data* context = data;

    TerminateProcess(pi.hProcess, 0);

    pthread_mutex_lock(&(context->mutex));
    context->run_thread = 0;
    pthread_mutex_unlock(&(context->mutex));

    int rc = pthread_join(context->thread, NULL);
    if(rc == 0)
    {
        info("Background thread join complete");
    }
    else
    {
        warn("Error joining background thread: %d", rc);
    }

	bfree(context);
}

static struct obs_source_info vextm_source_info = {
	.id             = "vextm_source",
	.type           = OBS_SOURCE_TYPE_INPUT,
	.output_flags   = OBS_SOURCE_ASYNC_VIDEO,
	.get_name       = vextm_source_get_name,
	.create         = vextm_source_create,
	.destroy        = vextm_source_destroy,
	.update         = vextm_source_update,
	.get_defaults   = vextm_source_get_defaults,
	.get_properties = vextm_source_get_properties
};

OBS_DECLARE_MODULE()
OBS_MODULE_USE_DEFAULT_LOCALE("vextm-source", "en-US")

bool obs_module_load(void)
{
	obs_register_source(&vextm_source_info);
	return true;
}
