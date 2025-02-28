/******************************************************************************
/ TimeState.h
/
/ Copyright (c) 2009 Tim Payne (SWS)
/
/
/ Permission is hereby granted, free of charge, to any person obtaining a copy
/ of this software and associated documentation files (the "Software"), to deal
/ in the Software without restriction, including without limitation the rights to
/ use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies
/ of the Software, and to permit persons to whom the Software is furnished to
/ do so, subject to the following conditions:
/ 
/ The above copyright notice and this permission notice shall be included in all
/ copies or substantial portions of the Software.
/ 
/ THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
/ EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES
/ OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
/ NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT
/ HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY,
/ WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
/ FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
/ OTHER DEALINGS IN THE SOFTWARE.
/
******************************************************************************/


#pragma once

class TimeSelection
{
public:
	TimeSelection(int iSlot, bool bIsLoop);
	TimeSelection(LineParser* lp);
	void Restore();
	char* ItemString(char* str, int maxLen);

	double m_dStart;
	double m_dEnd;
	int m_iSlot;
	bool m_bIsLoop;
};

extern SWSProjConfig<WDL_PtrList_DOD<TimeSelection> > g_timeSel;

// "Exported" functions
void SaveTimeSel(COMMAND_T* cs);
void SaveLoopSel(COMMAND_T* cs);
void RestoreTimeSel(COMMAND_T* cs);
void RestoreLoopSel(COMMAND_T* cs);
void RestoreLoopNext(COMMAND_T*);
void RestoreTimeNext(COMMAND_T*);