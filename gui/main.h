#ifndef MAIN_H
#define MAIN_H

// define LV_FAST_UPDATE to make folder crawling work faster
// (about 3 times) on the expense of realtime list updating 
//#define LV_FAST_UPDATE
#define UNICODE

#ifndef UNICODE
    #error "You need to define UNICODE for this app to compile correctly"
#endif

#if !defined(WM_DPICHANGED)
    #define WM_DPICHANGED 0x02E0
#endif

// default settings for display res
#define DEFAULT_DPI     96

// make a "reservation" for this # of items, to make it easier
// for the list
#define LV_DEFAULT_CAPACITY 50000

// how many "folder in folder" levels
#define MAX_DEPTH       100

// private thread messages
#define WM_ENDFSIZE     WM_APP + 1      // end op.
#define WM_UPDFSIZE     WM_APP + 2      // data available, update the list
#define WM_ABTFSIZE     WM_APP + 100    // abort op.
#define WM_RAGEQUIT     WM_APP + 199    // user depressed :-)

#define DLG_MAIN        1001
#define IDC_BREAKOP     4001
#define IDC_FLIST       4007
#define IDC_FLABEL      4008

#define IDR_LPOP        2001
#define IDM_SAVECSV     6001

#define IDR_ICO_MAIN    8001

#endif

