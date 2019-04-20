/*
** Based on reaper_csurf
** Copyright (C) 2006-2008 Cockos Incorporated
** License: LGPL.
**
** MCU support - Modified for generic controller surfaces such as Korg NanoKontrol 2 support by : Pierre Rousseau (May 2017)
** https://github.com/Pierousseau/reaper_generic_control
*/


#include "control_surface_interface.h"
#include "helpers.h"
#include "sws_imports.h"
#pragma warning (push, 0)
#include <WDL/ptrlist.h>
#pragma warning (pop)

#include <filesystem>
#include <functional>
#include <fstream>
#include <list>
#include <map>
#include <string>
#include <math.h>
#include "reaper_command_ids.h"

#ifdef max
#undef min
#undef max
#endif

#define PLUGIN_NAME "AdvancedMouseController v0.1"
#define PLUGIN_TYPE_NAME "MCCNTRL"
#define CURSOR_ADJUSTMENT_SCALE_FACTOR 0.30
#define VERTICAL_ZOOM_SCALE_FACTOR 0.2

#pragma warning (disable:4996)

//This comes straight from sws/zoom.cpp -> HorizScroll, just changed the input parameters a bit
void HorizScroll2(double pos) {
	HWND hwnd = GetTrackWnd();
	SCROLLINFO si = { sizeof(SCROLLINFO), };
	si.fMask = SIF_ALL;
	CoolSB_GetScrollInfo(hwnd, SB_HORZ, &si);
	si.nPos += (int)(pos * si.nPage / 100.0);
	if (si.nPos < 0) si.nPos = 0;
	else if (si.nPos > si.nMax) si.nPos = si.nMax;
	CoolSB_SetScrollInfo(hwnd, SB_HORZ, &si, true);
	SendMessage(hwnd, WM_HSCROLL, SB_THUMBPOSITION, NULL);
}


inline int signed_int_7bit(uint8_t byte) {
	int b;
	INT8 a = 0;
	a |= byte;
	if (byte & 0x40) {
		a |= 0x80;
	}
	b = (int)a;

	return b;
}

enum MIDI_TYPE {
	NOTE_ON = 0x90,
	NOTE_OFF = 0x80,
	CC_CHANGE = 0xB0,
	END_OF_QUEUE = 0x00,
};

enum MOUSE_KEY {
	KEY_1 = 1, KEY_2, KEY_3, KEY_4, KEY_5, KEY_6, KEY_7, KEY_8, KEY_9, KEY_0, KEY_PLUS, KEY_MINUS,
};

enum CC_KEY {
	CC_X = 1, CC_Y, CC_R, CC_Integrated_Distance,
};

const char* getMidiTypeName(uint8_t b)
{

	switch (b)
	{
	case NOTE_ON: return "NOTE_ON";
	case NOTE_OFF: return "NOTE_OFF";
	case CC_CHANGE: return "CC_CHANGE";
	}
}

void getMidiMessageText(char* text, uint8_t status, uint8_t channel, uint8_t byte1, uint8_t byte2) {
	sprintf(text, "Midi:%s, Channel:%d, Byte1:%d, Byte2:%d\n", (char*)getMidiTypeName(status), channel, byte1, byte2);
}

/*
A control surface plugin needs to override IReaperControlSurface.
This override is a lot simpler than usual, as it just forwards MIDI orders to the active preset,
without trying to understand them.
All code below this point is mostly a heavily simplified version of the usual MCU plugin examples.
*/
class MouseController : public IReaperControlSurface
{
public:
	MouseController(int indev, int outdev, int preset, int *errStats)
		: _midi_in_dev(indev)
		, _midi_out_dev(outdev)
		, _preset(preset)
	{
		_midi_in = _midi_in_dev >= 0 ? CreateMIDIInput(_midi_in_dev) : NULL;
		_midi_out = _midi_out_dev >= 0 ? CreateThreadedMIDIOutput(CreateMIDIOutput(_midi_out_dev, false, NULL)) : NULL;


		if (errStats)
		{
			if (_midi_in_dev >= 0 && !_midi_in) *errStats |= 1;
			if (_midi_out_dev >= 0 && !_midi_out) *errStats |= 2;
		}

		_surfaceInit();

		if (_midi_in)
			_midi_in->start();
	}

	~MouseController()
	{
		CloseNoReset();
	}

	const char *GetTypeString()
	{
		return PLUGIN_TYPE_NAME;
	}

	/*
	The plugin description string, as seen in Reaper's preferences dialog
	*/
	const char *GetDescString()
	{
		_desc_string.Set(PLUGIN_NAME);
		char tmp[512];
		sprintf(tmp, " (dev %d,%d)", _midi_in_dev, _midi_out_dev);
		_desc_string.Append(tmp);
		return _desc_string.Get();
	}

	/*
	The plugin configuration string, made of three numerals which are the ID of the input device, the ID of the output device, and the preset ID
	*/
	const char *GetConfigString()
	{
		sprintf(_cfg_string, "%d %d", _midi_in_dev, _midi_out_dev);
		return _cfg_string;
	}

	virtual void CloseNoReset() override
	{
		if (_midi_in)
			delete _midi_in;
		if (_midi_out)
			delete _midi_out;
		_midi_out = 0;
		_midi_in = 0;
	}

	virtual void Run() override
	{
		MIDI_event_t *evt;
		uint8_t status;
		uint8_t channel;
		uint8_t byte1;
		uint8_t byte2;

		if (_midi_in)
		{
			_midi_in->SwapBufs(timeGetTime());
			int l = 0;
			MIDI_eventlist *list = _midi_in->GetReadBuf();

			/*
			if (list->GetSize() > 0) {
				sprintf(s, "Midi Event Queue Size:%d\n", list->GetSize());
				ShowConsoleMsg(s);
			}
			*/

			while ((evt = list->EnumItems(&l)))
			{
				status = evt->midi_message[0] & 0xF0;
				if ((status == NOTE_ON) || (status == NOTE_OFF) || (status == CC_CHANGE)) {
					channel = evt->midi_message[0] & 0x0F;
					byte1 = evt->midi_message[1] & 0xFF;
					byte2 = evt->midi_message[2] & 0xFF;
					_midiEvent2(status,channel,byte1,byte2);
				}
			}

			//Send a zero message to flush out whatever remaining accumulated cc
			_midiEvent2(0,0,0,0);
		}
	}


private:
	typedef void (MouseController::*cc_function)(uint8_t status, uint8_t channel, int del_X, int del_Y);
	typedef void (MouseController::*button_function)();

	//void _midiEvent2(uint8_t status, uint8_t channel, uint8_t byte1, uint8_t byte2);

	uint8_t last_note_pressed;
	uint8_t cc_values_received_while_note_pressed;
	cc_function cc_actions[15];
	button_function button_actions[15];

	char s[128];

	//These accumulators are used to consolidate multiple CC_x messages in the midi queue, so that 
	//cc_action functions do not get called multiple times in one run() cycle.
	int cc_accumulator_x = 0, cc_accumulator_y = 0;
	int accumulated_cc_messages = 0;

	//These accumulators are used by functions like select next region/marker 
	//to keep the residual movement of the mouse until it becomes enough to jump
	//to the next region/marker
	int secondary_accumulator_x = 0, secondary_accunmulator_y = 0;

	void button_do_nothing() {
		// do nothing;
	}

	void button_toggle_snap() {
		Main_OnCommand(TOGGLE_SNAPPING, 0);
	}

	void button_toggle_selected_tracks_mute() {
		Main_OnCommand(TOGGLE_MUTE_ON_SELECTED_TRACKS, 0);
	}

	void button_toggle_selected_tracks_solo() {
		Main_OnCommand(TOGGLE_SOLO_ON_SELECTED_TRACKS, 0);
	}

	void button_toggle_selected_items_mute() {
		Main_OnCommand(TOGGLE_MUTE_ON_SELECTED_ITEMS, 0);
	}

	void button_toggle_selected_items_lock() {
		Main_OnCommand(TOGGLE_LOCK_ON_SELECTED_ITEMS, 0);
	}


	/* This is the beginning of the action functions */
	void do_nothing(uint8_t a, uint8_t b, int c, int d) {
		// do nothing;
	}

	void cc_action_based_on_note_pressed(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		//char s[128];
		//sprintf(s, "Last Note Pressed:%d\n", last_note_pressed);
		//ShowConsoleMsg(s);
		cc_function func = cc_actions[last_note_pressed];
		(this->*func)(status, channel, del_X, del_Y);
		cc_values_received_while_note_pressed += 1;
	}

	void take_note_action() {
		button_function func = button_actions[last_note_pressed];
		(this->*func)();
	}

	void adjust_selected_tracks_height(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		int adj = (int)((double)del_Y * VERTICAL_ZOOM_SCALE_FACTOR);

		int num_selected_tracks = CountSelectedTracks(0);
		int i = 0;
		int track_height = 0;
		int new_track_height = 0;
		MediaTrack* j = NULL;
		for (i = 0; i < num_selected_tracks; i++) {
			j = GetSelectedTrack(0, i);
			track_height = GetTrackHeight(j, NULL);
			new_track_height = track_height + adj;
			new_track_height = SetToBounds(new_track_height, GetTcpTrackMinHeight(), GetCurrentTcpMaxHeight());
			SetTrackHeight(j, new_track_height);
		}
	}

	void adjust_all_tracks_height(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		int adj = (int)((double)del_Y * VERTICAL_ZOOM_SCALE_FACTOR);

		int num_tracks = CountTracks(0);
		int i = 0;
		int track_height = 0;
		int new_track_height = 0;
		MediaTrack* j = NULL;
		for (i = 0; i < num_tracks; i++) {
			j = GetTrack(0, i);
			track_height = GetTrackHeight(j, NULL);
			new_track_height = track_height + adj;
			new_track_height = SetToBounds(new_track_height, GetTcpTrackMinHeight(), GetCurrentTcpMaxHeight());
			SetTrackHeight(j, new_track_height);
		}
	}

	void adjust_last_touched_tracks_height(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		int adj = (int)((double)del_Y * VERTICAL_ZOOM_SCALE_FACTOR);

		int i = 0;
		int track_height = 0;
		int new_track_height = 0;
		MediaTrack* j = GetLastTouchedTrack();
		track_height = GetTrackHeight(j, NULL);
		new_track_height = track_height + adj;
		new_track_height = SetToBounds(new_track_height, GetTcpTrackMinHeight(), GetCurrentTcpMaxHeight());
		SetTrackHeight(j, new_track_height);
	}

	void adjust_cursor_position(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		double zoom_level = GetHZoomLevel();
		double cursor_position = GetCursorPosition();
		double adjustment = del_X * (CURSOR_ADJUSTMENT_SCALE_FACTOR / zoom_level);
		cursor_position = cursor_position + adjustment;
		if (cursor_position < 0.0)
			cursor_position = 0.0;
		SetEditCurPos(cursor_position, 1, 0);
	}


	void adjust_horizontal_zoom_level(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		double min_zoom_value = 0.1;
		// Zoom value is a fraction that represents song length / display, this would 
		// compress the whole song such that you could fit two side by side, enough?

		double zoom_level = GetHZoomLevel();
		double adj = 0.0;

		adj = (del_X * 0.005 * zoom_level);
		
		double zoom_adjusted = zoom_level + adj;
		if (zoom_adjusted < min_zoom_value)
			zoom_adjusted = min_zoom_value;
			
		adjustZoom(zoom_adjusted, 1, TRUE, -1);

		/*
		getMidiMessageText(s, status, channel, byte1, byte2);
		ShowConsoleMsg(s);
		sprintf(s, "Adjust Horizontal Zoom: Signed Int value:%d\n", del_X);
		ShowConsoleMsg(s);
		*/
	}

	void adjust_horizontal_scroll(uint8_t status, uint8_t channel, int del_X, int del_Y) {

		HorizScroll2((double)del_X);
		
		/*
		getMidiMessageText(s, status, channel, byte1, byte2);
		ShowConsoleMsg(s);
		sprintf(s, "Adjust Horizontal Scroll: Signed Int value:%d\n", del_X);
		ShowConsoleMsg(s);
		*/
	}

	void move_left_selection_edge(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		double startPos;
		double endPos;
		double zoom_level = GetHZoomLevel();
		double adjustment = del_X * (CURSOR_ADJUSTMENT_SCALE_FACTOR / zoom_level);
		
		GetSet_LoopTimeRange(false, true, &startPos, &endPos, false);
		startPos = startPos + adjustment;
		GetSet_LoopTimeRange(true, true, &startPos, &endPos, false);
	}


	void move_right_selection_edge(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		double startPos;
		double endPos;
		double zoom_level = GetHZoomLevel();
		double adjustment = del_X * (CURSOR_ADJUSTMENT_SCALE_FACTOR / zoom_level);

		GetSet_LoopTimeRange(false, true, &startPos, &endPos, false);
		endPos = endPos + adjustment;
		GetSet_LoopTimeRange(true, true, &startPos, &endPos, false);
	}

	void select_next_prev_region(uint8_t status, uint8_t channel, int del_X, int del_Y) {
#define THRESHOLD 300
		secondary_accumulator_x += del_X;
		if (abs(secondary_accumulator_x) > THRESHOLD) {
			if (secondary_accumulator_x > 0) {
				SelNextRegion(NULL);
				secondary_accumulator_x -= THRESHOLD;
			}
			else {
				SelPrevRegion(NULL);
				secondary_accumulator_x += THRESHOLD;
			}	
		}
	}

	void select_and_move_to_next_prev_item(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		secondary_accumulator_x += del_X;
		if (abs(secondary_accumulator_x) > THRESHOLD) {
			if (secondary_accumulator_x > 0) {
				//SelNextMarkerOrRegion(NULL);
				//Reaper Markers: GoToNextMarker/Project End
				Main_OnCommand(SELECT_AND_MOVE_TO_NEXT_ITEM, 0);
				secondary_accumulator_x -= THRESHOLD;
			}
			else {
				//SelPrevMarkerOrRegion(NULL);
				//Reaper Markers: GoToPrevMarker/Project Start
				Main_OnCommand(SELECT_AND_MOVE_TO_PREV_ITEM, 0);
				secondary_accumulator_x += THRESHOLD;
			}
		}
	}

	void select_next_prev_marker(uint8_t status, uint8_t channel, int del_X, int del_Y) {
		secondary_accumulator_x += del_X;
		if (abs(secondary_accumulator_x) > THRESHOLD) {
			if (secondary_accumulator_x > 0) {
				//SelNextMarkerOrRegion(NULL);
				//Reaper Markers: GoToNextMarker/Project End
				Main_OnCommand(GO_TO_NEXT_MARKER_OR_PROJECT_END, 0);
				secondary_accumulator_x -= THRESHOLD;
			}
			else {
				//SelPrevMarkerOrRegion(NULL);
				//Reaper Markers: GoToPrevMarker/Project Start
				Main_OnCommand(GO_TO_PREV_MARKER_OR_PROJECT_START, 0);
				secondary_accumulator_x += THRESHOLD;
			}
		}
#undef THRESHOLD
	}
	

	void _surfaceInit()
	{
		last_note_pressed = 0;
		for (int i = 0; i < 15; i++) {
			cc_actions[i] = &MouseController::do_nothing;
			button_actions[i] = &MouseController::button_do_nothing;
		}

		cc_actions[1] = &MouseController::adjust_cursor_position;
		cc_actions[2] = &MouseController::adjust_horizontal_scroll;
		cc_actions[3] = &MouseController::adjust_horizontal_zoom_level;
		cc_actions[4] = &MouseController::adjust_selected_tracks_height;
		cc_actions[5] = &MouseController::adjust_last_touched_tracks_height;
		cc_actions[6] = &MouseController::adjust_all_tracks_height;
		cc_actions[7] = &MouseController::move_left_selection_edge;
		cc_actions[8] = &MouseController::move_right_selection_edge;
		cc_actions[9] = &MouseController::select_next_prev_region;
		cc_actions[0] = &MouseController::select_next_prev_marker;	//Button#10
		cc_actions[11] = &MouseController::select_and_move_to_next_prev_item;

		// Keys 1,2,3 for global editing type operations
		button_actions[1] = &MouseController::button_toggle_snap;

		// Keys 4,5,6 for selected track type operations
		button_actions[4] = &MouseController::button_toggle_selected_tracks_mute;
		button_actions[5] = &MouseController::button_toggle_selected_tracks_solo;

		// Keys 7,8,9 for selected items type operations
		button_actions[7] = &MouseController::button_toggle_selected_items_mute;
		button_actions[8] = &MouseController::button_toggle_selected_items_lock;

	}

	void _midiEvent2(uint8_t status, uint8_t channel, uint8_t byte1, uint8_t byte2)
	{
		char midi_dump[128];
		
		if ((status == END_OF_QUEUE) || (status == NOTE_OFF)) {
			if (accumulated_cc_messages > 0) {
				//sprintf(midi_dump, "Number of CC_Messages Accumulated :%d", accumulated_cc_messages);
				//ShowConsoleMsg(midi_dump);
				cc_action_based_on_note_pressed(status, channel, cc_accumulator_x, cc_accumulator_y);
				accumulated_cc_messages = 0;
				cc_accumulator_x = 0;
				cc_accumulator_y = 0;
			}
		}

		switch (status) {
		case NOTE_ON:
		{
			//ShowConsoleMsg("Inside NOTE_ON\n");
			last_note_pressed = byte1;
			cc_values_received_while_note_pressed = 0;
			break;
		}

		case CC_CHANGE:
		{
			//ShowConsoleMsg("Inside CC_CHANGE\n");
			//cc_action_based_on_note_pressed(status, channel, byte1, byte2);
			if (byte1 == CC_X) {
				cc_accumulator_x += (int)signed_int_7bit(byte2);
				accumulated_cc_messages += 1;
			}
			if (byte1 == CC_Y){
				cc_accumulator_y += (int)signed_int_7bit(byte2);
				accumulated_cc_messages += 1;
			}
			break;
		}

		case NOTE_OFF:
		{
			//ShowConsoleMsg("Inside NOTE_OFF\n");
			if (cc_values_received_while_note_pressed == 0) {
				take_note_action();
			}
			last_note_pressed = 0;
			break;
		}
		}

		//Debug crap
		/*
		if ((status == NOTE_ON) || (status == NOTE_OFF)) {
			sprintf(midi_dump, "last_note_pressed:%d", last_note_pressed);
			ShowConsoleMsg(midi_dump);
		}

		sprintf_s(midi_hex, "%02x %02x %02x %02x\n", status, channel, byte1, byte2);
		int cmdid = NamedCommandLookup("_RS33a01e04b9804e7d6ca4089b1fdad880b95eb6a5");
		SetExtState("HyperMouse", "MIDI", midi_dump, false);
		Main_OnCommand(cmdid, false);
		*/
	}


private:
	int _midi_in_dev;
	int _midi_out_dev;
	int _preset;

	midi_Output * _midi_out;
	midi_Input * _midi_in;

	WDL_String _desc_string;
	char _cfg_string[1024];
};


/*
Parse the plugin configuration string (input device ID, output device ID, preset ID)
*/
static void parseParameters(const char *str, int params[3])
{
	params[0] = params[1] = params[2] = -1;

	const char *p = str;
	if (p)
	{
		int x = 0;
		while (x < 3)
		{
			while (*p == ' ') p++;
			if ((*p < '0' || *p > '9') && *p != '-') break;
			params[x++] = atoi(p);
			while (*p && *p != ' ') p++;
		}
	}
}


/*
Instantiate the plugin
*/
static IReaperControlSurface *createFunc(const char *type_string, const char *configString, int *errStats)
{
	int params[3];
	parseParameters(configString, params);

	return new MouseController(params[0], params[1], params[2], errStats);
}

static WDL_DLGRET dlgProc(HWND hwndDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg)
	{
	case WM_INITDIALOG:
	{
		/*
		Add comboboxes to the plugin's configuration dialog
		*/
		int params[3];
		parseParameters((const char *)lParam, params);

		int count = GetNumMIDIInputs();
		LRESULT entry_none = SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_ADDSTRING, 0, (LPARAM)"None");
		SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_SETITEMDATA, entry_none, -1);
		for (int i = 0; i < count; i++)
		{
			char buf[512];
			if (GetMIDIInputName(i, buf, sizeof(buf)))
			{
				LRESULT entry_midi = SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_ADDSTRING, 0, (LPARAM)buf);
				SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_SETITEMDATA, entry_midi, i);
				if (i == params[0])
					SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_SETCURSEL, entry_midi, 0);
			}
		}


		/*
		entry_none = SendDlgItemMessage(hwndDlg, IDC_COMBO3, CB_ADDSTRING, 0, (LPARAM)"None");
		SendDlgItemMessage(hwndDlg, IDC_COMBO3, CB_SETITEMDATA, entry_none, -1);
		count = GetNumMIDIOutputs();
		for (int i = 0; i < count; i++)
		{
			char buf[512];
			if (GetMIDIOutputName(i, buf, sizeof(buf)))
			{
				LRESULT entry_midi = SendDlgItemMessage(hwndDlg, IDC_COMBO3, CB_ADDSTRING, 0, (LPARAM)buf);
				SendDlgItemMessage(hwndDlg, IDC_COMBO3, CB_SETITEMDATA, entry_midi, i);
				if (i == params[1])
					SendDlgItemMessage(hwndDlg, IDC_COMBO3, CB_SETCURSEL, entry_midi, 0);
			}
		}*/

		/*
		entry_none = SendDlgItemMessage(hwndDlg, IDC_COMBO4, CB_ADDSTRING, 0, (LPARAM)"None");
		SendDlgItemMessage(hwndDlg, IDC_COMBO4, CB_SETITEMDATA, entry_none, -1);
		for (const auto & preset : surface_presets)
		{
		  int preset_key = preset.first;
		  LRESULT entry_preset = SendDlgItemMessage(hwndDlg, IDC_COMBO4, CB_ADDSTRING, 0, (LPARAM)preset.second->name().c_str());
		  SendDlgItemMessage(hwndDlg, IDC_COMBO4, CB_SETITEMDATA, entry_preset, preset_key);
		  if (preset_key == params[2])
			SendDlgItemMessage(hwndDlg, IDC_COMBO4, CB_SETCURSEL, entry_preset, 0);
		} */
	}
	break;

	case WM_USER + 1024:
	{
		/*
		Handle user input in the plugin's configuration dialog
		*/
		if (wParam > 1 && lParam)
		{
			char tmp[512];
			int midi_in = -1, midi_out = -1;

			LRESULT r = SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_GETCURSEL, 0, 0);
			if (r != CB_ERR)
				midi_in = (int)SendDlgItemMessage(hwndDlg, IDC_COMBO2, CB_GETITEMDATA, r, 0);

			/* r = SendDlgItemMessage(hwndDlg, IDC_COMBO3, CB_GETCURSEL, 0, 0);
			if (r != CB_ERR)
				midi_out = (int)SendDlgItemMessage(hwndDlg, IDC_COMBO3, CB_GETITEMDATA, r, 0); */

			/* r = SendDlgItemMessage(hwndDlg, IDC_COMBO4, CB_GETCURSEL, 0, 0);
			if (r != CB_ERR)
			  preset = (int)SendDlgItemMessage(hwndDlg, IDC_COMBO4, CB_GETITEMDATA, r, 0); */

			sprintf(tmp, "%d %d", midi_in, midi_out);
			lstrcpyn((char *)lParam, tmp, (int)wParam);
		}
	}
	break;
	}
	return 0;
}

static HWND configFunc(const char *type_string, HWND parent, const char *initConfigString)
{
	return CreateDialogParam(g_hInst, MAKEINTRESOURCE(IDD_SURFACEEDIT_MCU), parent, dlgProc, (LPARAM)initConfigString);
}

/*
Global variable, of type struct reaper_csurf_reg_t, containing the plugin's name, creation and configuration functions.
Used in plugin_main.cpp : REAPER_PLUGIN_ENTRYPOINT :
	rec->Register("csurf", &generic_surface_control_reg);
*/
reaper_csurf_reg_t generic_surface_control_reg =
{
  PLUGIN_TYPE_NAME,
  PLUGIN_NAME,
  createFunc,
  configFunc,
};
