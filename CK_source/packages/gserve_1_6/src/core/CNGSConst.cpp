// CNGSConst.cpp - per social network name tables of gServe, indexed by eSocialNetworkType
// (0 Glu social, 1 Game Center, 2 Facebook, 3 Twitter, 4 MySpace, 5 OpenFeint).
// Data tables follow the order in the CK 1.5.2 ARMv7 symbol table.
#include "CNGSConst.h"

/** Credentials file keys of the per network user ids. */
const wchar_t* SocialNetworkIDNames[] = {
    L"gluid",
    L"gamecenterid",
    L"facebookid",
    L"twitterid",
    L"myspaceid",
    L"openfeintid"
};

/** Credentials file keys of the per network aliases. */
const wchar_t* SocialNetworkAliasNames[] = {
    L"gluAlias",
    L"gamecenterAlias",
    L"facebookAlias",
    L"twitterAlias",
    L"myspaceAlias",
    L"openfeintAlias"
};

/** Server attribute names of the per network user ids. */
const char* SocialNetwork_NGS_ID_TOKEN[] = {
    "GLUSOCIALID",
    "GAMECENTERID",
    "FACEBOOKID",
    "TWITTERID",
    "MYSPACEID",
    "OPENFEINTID"
};

/** Server attribute names of the per network aliases. */
const char* SocialNetwork_NGS_ALIAS_TOKEN[] = {
    // Confirmed against the six entries in CK 1.5.2 ARMv7.
    "GLUSOCIALALIAS",
    "GAMECENTERALIAS",
    "FACEBOOKALIAS",
    "TWITTERALIAS",
    "MYSPACEALIAS",
    "OPENFEINTALIAS",
};
