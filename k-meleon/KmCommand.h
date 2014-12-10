/*
*  Copyright (C) 2008 Dorian Boissonnade
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


#pragma once

#include "KmeleonConst.h"
struct _kmeleonCommand;

const UINT kPluginIdRangeNumber = 2;
const UINT kPluginIdRange[kPluginIdRangeNumber][2] = { { 0x1800, 0x6FFF }, {0x9000, 0xDFFF} };
#define MAX_ID_NUMBER (0x7000 - 0x1800) + (0xE000 - 0x9000)
#define MAX_DESC_LENGTH 256

class CMfcEmbedApp;

struct KmCommand
{
	UINT id;
	CString desc;

	KmCommand() : id(0) {}
	KmCommand(UINT aId) : id(aId) {}
	KmCommand(UINT aId, LPCTSTR aDesc) : id(aId), desc(aDesc) {}
	BOOL IsPlugin();
	CString GetDesc() {
		return desc;
	}
};

class KmCmdService
{
public:

	KmCmdService()
	{
		InitializeDefineMap();
		InitPluginCmdList();
		InitDefaultCmd();
	}

	void InitPluginCmdList()
	{
		UINT i=0;
		for (int j=0;j<kPluginIdRangeNumber;j++)
			for (UINT k=kPluginIdRange[j][0]; k<=kPluginIdRange[j][1]; k++)
				mIdList[i++] = k;
		mCurrentId = 0;
		mBottomId = i-1;
	}

	BOOL IsPluginCommand(UINT id)
	{
		for (int j=0;j<kPluginIdRangeNumber;j++)
			if (id>=kPluginIdRange[j][0] && id <= kPluginIdRange[j][1])
				return TRUE;
		return FALSE;
	}

	UINT AllocateId(UINT num)
	{
		if (num == 1) return AllocateId();
		
		// For the sake of simplicity and compatibility, group of ids are allocated
		// from the end.
		ASSERT(mCurrentId + num < mBottomId);
		if (mCurrentId + num >= mBottomId)
			return 0;
		mBottomId -= num;
		return mIdList[mBottomId];
	}

	UINT AllocateId()
	{
		ASSERT(mCurrentId<mBottomId);
		if (mCurrentId>=mBottomId) return 0;
		return mIdList[mCurrentId++];
	}

	void ReleaseId(UINT id)
	{
		ASSERT(mCurrentId>0);
		if (mCurrentId>0) 
			mIdList[--mCurrentId] = id;
	}

	void UnregisterCommand(LPCTSTR plugin, LPCTSTR command)
	{
		KmCommand kcommand;
		if (!mCommandList.Lookup(command, kcommand))
			return;

		ReleaseId(kcommand.id);
		mCommandList.RemoveKey(command);
	}

	UINT RegisterCommand(LPCTSTR name, LPCTSTR desc = 0, LPCTSTR icon = 0);

	BOOL ParseCommand(char *pszCommand, char** plugin, char **parameter)
	{
		if (!pszCommand)
			return FALSE;

		*plugin = pszCommand;
		*parameter = strchr(pszCommand, '(');
		if (!*parameter)
			return FALSE;

		char *close = strrchr(*parameter, ')');
		if (!close)
			return FALSE;

		*(*parameter)++ = 0;
		*close = 0;
		return TRUE;
	}
#ifdef _UNICODE
	UINT GetId(LPCSTR command)
	{
		USES_CONVERSION;
		return GetId(A2CT(command));
	}
#endif
	UINT GetCount()
	{
		return mCommandList.GetCount();	
	}

	UINT GetDefCount()
	{
		return mDefCount;
	}

	UINT GetPluginCount()
	{
		return GetCount() - GetDefCount();
	}

	UINT    GetId(LPCTSTR command);
	bool GetCommand(UINT id, KmCommand&);
	bool GetCommand(LPCTSTR command, KmCommand&);

	CString GetDescription(LPCTSTR command);
	CString GetDescription(UINT id);
	UINT    GetList(_kmeleonCommand* cmdList, UINT size, BOOL def = FALSE);

protected:

	void InitializeDefineMap();
	void InitDefaultCmd();
	typedef CMap<CString, LPCTSTR, KmCommand, KmCommand&> CCmdMap;

	CMap<CString, LPCTSTR, UINT, UINT &> defineMap;
	CCmdMap mCommandList;
	UINT mIdList[MAX_ID_NUMBER];
	UINT mCurrentId;
	UINT mDefCount;
	UINT mBottomId;
	//CList<KmCommand, KmCommand&> mCommandList;
};


