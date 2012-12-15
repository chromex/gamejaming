#ifndef LAWLJSON_SETTINGS_H
#define LAWLJSON_SETTINGS_H

#define LJ_NUMBER_TYPE double

#define LJ_NAMESPACE LawlJSON

// Set to one if you want LawlJSON to be inside the LawlJSON namespace
// and 0 if not.
//
#if 0
    #define BEGIN_LAWLJSON namespace LJ_NAMESPACE {
    #define END_LAWLJSON }
#else
    #define BEGIN_LAWLJSON
    #define END_LAWLJSON
#endif

#endif