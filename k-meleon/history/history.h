/*
*  Copyright (C) 2001 Jeff Doozan
*
*  This program is free software; you can redistribute it and/or modify
*  it under the terms of the GNU General Public License as published by
*  the Free Software Foundation; either version 2, or (at your option)
*  any later version.
*
*  This program is distributed in the hope that it will be useful,
*  but WITHOUT ANY WARRANTY; without even the implied warranty of
*  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
*  GNU General Public License for more details.
*
*  You should have received a copy of the GNU General Public License
*  along with this program; if not, write to the Free Software
*  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
*/


int Init();
void Create(HWND parent);
void Config(HWND parent);
void Quit();
HGLOBAL GetMenu();
void DoMenu(HMENU menu, char *param);
void DoRebar(HWND rebarWnd);

LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

void CreateBackMenu (UINT button);
void CreateForwardMenu (UINT button);
int MozillaSessionHistory(char **titles[], int *count, int *index);
void UpdateHistoryMenu();
void OnTBLButtonHold(DWORD buttonID, DWORD unused);
void OnTBRButtonDown(WPARAM controlID, LPARAM lParam);

void FreeStringArray(char *array[], int size);
void CondenseMenuText(char *buf, char *title, int index);

int ID_HISTORY_FLAG = -1;
int ID_HISTORY = -1;