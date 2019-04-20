/*
** Based on reaper_csurf
** Copyright (C) 2006-2008 Cockos Incorporated
** License: LGPL.
**
** MCU support - Modified for generic controller surfaces such as Korg NanoKontrol 2 support by : Pierre Rousseau (May 2017)
** https://github.com/Pierousseau/reaper_generic_control
**
** Code in this file is basically identical to "ThirdParties\ReaperExtensionsSdk\jmde\csurf\csurf_main.cpp" in the reaper SDK,
** except for the plugin registration part at the end of REAPER_PLUGIN_ENTRYPOINT function.
*/

#define REAPERAPI_IMPLEMENT
#include "control_surface_interface.h"
#define IMPAPI(x)       if (!errcnt && !((*(void **)&(x)) = (void *)rec->GetFunc(#x))) errcnt++;

extern reaper_csurf_reg_t generic_surface_control_reg;

REAPER_PLUGIN_HINSTANCE g_hInst; // used for dialogs, if any
HWND g_hwnd;
HWND g_hwndParent = NULL;

int *g_config_csurf_rate, *g_config_zoommode;

int __g_projectconfig_timemode2, __g_projectconfig_timemode;
int __g_projectconfig_measoffs;
int __g_projectconfig_timeoffs; // double

extern "C"
{

  REAPER_PLUGIN_DLL_EXPORT int REAPER_PLUGIN_ENTRYPOINT(REAPER_PLUGIN_HINSTANCE hInstance, reaper_plugin_info_t *rec)
  {
    g_hInst = hInstance;

    if (!rec || rec->caller_version != REAPER_PLUGIN_VERSION || !rec->GetFunc)
      return 0;
	
    g_hwnd = rec->hwnd_main;
	g_hwndParent = rec->hwnd_main;

    int errcnt = 0;
	errcnt = REAPERAPI_LoadAPI(rec->GetFunc);
	
	//Including extra reaper api functions from sws_rpf_wrapper

	IMPAPI(AttachWindowTopmostButton);
	IMPAPI(AttachWindowResizeGrip);
	IMPAPI(RemoveXPStyle);
	IMPAPI(CoolSB_GetScrollInfo);
	IMPAPI(CoolSB_SetScrollInfo);
	IMPAPI(MainThread_LockTracks);
	IMPAPI(MainThread_UnlockTracks);
	IMPAPI(OnColorThemeOpenFile);

	if (errcnt > 0)
	{
		char errbuf[256];
		sprintf_s(errbuf, "Failed to load %d expected API function(s)", errcnt);
		MessageBox(g_hwnd, errbuf, "MRP extension error", MB_OK);
		return 0;
	}
	/* else {
		MessageBox(g_hwnd, "Pragun's MIDI CSurf", "Loaded Successfully", MB_OK);
	} */

    if (errcnt) return 0;

    rec->Register("csurf", &generic_surface_control_reg);
	//ShowConsoleMsg("Loaded MouseController.");

    return 1;
  }

};

