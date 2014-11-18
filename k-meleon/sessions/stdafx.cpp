// stdafx.cpp : fichier source incluant simplement les fichiers Include standard 
// sessions.pch représente l'en-tête précompilé
// stdafx.obj contient les informations de type précompilées

#include "stdafx.h"

std::string itos(int i) {
	char s[35];
	::itoa(i, s, 10);
	return s;
}

void setIntPref(const char* prefname, int value, bool flush) {
	kPlugin.kFuncs->SetPreference(PREF_INT, prefname, &value, flush);
}

void setStrPref(const char* prefname, char* value, bool flush) {
	kPlugin.kFuncs->SetPreference(PREF_STRING, prefname, value, flush);
}

int getIntPref(const char* prefname, int defvalue) {
	int value = defvalue;
	kPlugin.kFuncs->GetPreference(PREF_INT, prefname, &value, &value);
	return value;
}

std::string getStrPref(const char* prefname, const char* defvalue) {
	int len = kPlugin.kFuncs->GetPreference(PREF_STRING, prefname, 0, (void*)defvalue);
	char* str = new char[len+1];
	kPlugin.kFuncs->GetPreference(PREF_STRING, prefname, (void*)str, (void*)defvalue);
	std::string ret = str;
	delete [] str;
	return ret;
}