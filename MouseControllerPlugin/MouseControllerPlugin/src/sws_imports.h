#pragma once

typedef struct COMMAND_T
{
	gaccel_register_t accel;
	const char* id;
	void(*doCommand)(COMMAND_T*);
	const char* menuText;
	INT_PTR user;
	int(*getEnabled)(COMMAND_T*);
	int uniqueSectionId;
	void(*onAction)(COMMAND_T*, int, int, int, HWND);
	bool fakeToggle;
} COMMAND_T;

//These are symbols brought in from sws extensions
HWND GetTrackWnd();
HWND GetRulerWnd();
int GetTrackHeight(MediaTrack* track, int* offsetY, int* topGap = NULL, int* bottomGap = NULL);
void SetTrackHeight(MediaTrack* track, int height, bool useChunk = false);
template <typename T> T SetToBounds(T val, T min, T max);
int GetTcpTrackMinHeight();
int GetCurrentTcpMaxHeight();
void SelNextRegion(COMMAND_T*);
void SelPrevRegion(COMMAND_T*);
void SelNextMarkerOrRegion(COMMAND_T*);
void SelPrevMarkerOrRegion(COMMAND_T*);
