/*
    WFSIZE, a small folder/subfolder size calculator for Windows  
    -------------------------------------------------------------------
    Copyright (c) 2022 Adrian Petrila, YO3GFH
    
    Born from the need to check a "folder with many subfolders" size, 
    and see which subfolder takes more space. Something like the Linux
    du command, roughly. Built with Pelle's C compiler.

                                * * *
                                
    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program.  If not, see <http://www.gnu.org/licenses/>.

                                * * *

    Features
    ---------
    
    It's designed to take a single folder path from command line and
    go recursively from there to calculate total and subfolder size,
    presenting a list with all of them. If no start folder is specified,
    it uses the folder it was executed from.

    The console version takes a maximum "depth" (folder-in-folder) as
    a second optional parameter.

    The gui version is intended to work with the included Explorer shell 
    extension which starts the app with the selected folder. Multiple  
    or file selection is ignored. The folder crawling process starts in a
    separate thread and works reasonably fast. If so desired, the process
    can be interrupted. The resulting list can be sorted ascending or
    descending. 

    Nothing fancy, but gets the job done reasonably fast,
    in under 100 KBytes :-)
    
    You may build as 32 or 64 bit, but UNICODE is mandatory. 

    It's taylored to my own needs, modify it to suit your own. 
    I'm not a professional programmer, so this isn't the best code you'll find
    on the web, you have been warned :-))

    All the bugs are guaranteed to be genuine, and are exclusively mine =)
*/

#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include "main.h"
#include "lv.h"
#include "mem.h"
#include <windows.h>
#include <windowsx.h>
#include <process.h>
#include <commctrl.h>
#include <strsafe.h>

// structure to pass to thread functions
typedef struct _fsize_thread_data
{
    WPARAM      wParam;     // unused
    LPARAM      lParam;     // unused
    HWND        hList;      // listview hwnd
    HWND        hParent;    // dlg hwnd
    WCHAR       * fpath;    // root path
    WCHAR       * crtDir;   // current path
    __int64     size;       // crt. folder size
    __int64     * fsizes;   // always holds the correct fsize table address
    UINT        errcode;    // unused
    UINT_PTR    subfolders; // how many subfolders processed          
} THREAD_DATA;

// function prototypes

typedef HRESULT (WINAPI *PGetDpiForMonitor)(HMONITOR hmonitor, 
    int dpiType, UINT* dpiX, UINT* dpiY);

WORD GetWindowDPI ( HWND hWnd );
WCHAR ** FILE_CommandLineToArgv ( WCHAR * CmdLine, int * _argc );
INT_PTR CALLBACK MainDlgProc ( HWND, UINT, WPARAM, LPARAM );
BOOL IsDotOrTwoDots ( const WCHAR * src );

__int64 FolderSize ( const WCHAR * fpath, THREAD_DATA * ptd );

UINT __stdcall Thread_FolderSize ( void * thData );
BOOL CALLBACK EnumChildProc ( HWND hwndChild, LPARAM lParam );
BOOL ContextMenu ( HWND hWnd, int menuId );
BOOL SaveFolderListToCSV ( HWND hWnd );
BOOL LVItemsToCSV ( HWND hList, const WCHAR * fname);

int CALLBACK CompareListItems
( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort );

// message handlers
BOOL MainDLG_OnCOMMAND ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainDLG_OnSIZING ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainDLG_OnSIZE ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainDLG_OnUPDFSIZE ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainDLG_OnENDFSIZE ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainDLG_OnINITDIALOG ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainDLG_OnNOTIFY ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL MainDLG_OnDPICHANGED ( HWND hWnd, WPARAM wParam, LPARAM lParam );
BOOL PathFromModule ( WCHAR * buf, DWORD cchDest );

//
// globals
//

HANDLE      ghInstance;
HWND        ghList;                     // listview hwnd
WCHAR       grootDir[1024];             // passed from cmdline
UINT        gTid;                       // worker thread id
UINT_PTR    gThandle;                   // worker thread handle
BOOL        gThreadWorking = FALSE;     // flag for the worker thread
BOOL        gAscending = TRUE;          // for sorting
WORD        gDpi;                       // holds current monitor DPI

THREAD_DATA gTtd;                       // structure to pass data to and
                                        // from the worker thread

size_t      gfSizesCapacity;            // will hold gfSizes current capacity
__int64     * gfSizes;                  // pointer to int64 table that 
                                        // will hold ALL processed 
                                        // folders (gTtd.crtDir) size;
                                        // pointers to each element from
                                        // this table are stored in the  
                                        // coresponding list items data
                                        // struct

SYSTEMTIME  gTimeStart;                 // for calculating elapsed time

// some constants

const WCHAR * opn_filter  = L"CSV files (*.csv)\0*.csv\0"
                            "All Files (*.*)\0*.*\0";

const WCHAR * opn_defext  = L"csv";
const WCHAR * sav_title   = L"Save folder list to CSV...";
const WCHAR * app_name    = L"WFSize 1.1";
const WCHAR * app_name_ex = L"WFsize v1.1 - Windows folder size calculator,"
                            L" copyright (c) 2022 Adrian Petrila, YO3GFH";

/*-@@+@@--------------------------------------------------------------------*/
//       Function: wWinMain 
/*--------------------------------------------------------------------------*/
//           Type: int APIENTRY 
//    Param.    1: HINSTANCE hInstance    : 
//    Param.    2: HINSTANCE hPrevInstance: 
//    Param.    3: WCHAR *pszCmdLine      : 
//    Param.    4: int nCmdShow           : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 09.09.2022
//    DESCRIPTION: main program loop
/*--------------------------------------------------------------------@@-@@-*/
int APIENTRY wWinMain ( HINSTANCE hInstance, HINSTANCE hPrevInstance, 
    WCHAR *pszCmdLine, int nCmdShow )
/*--------------------------------------------------------------------------*/
{
    INITCOMMONCONTROLSEX    icc;
    WNDCLASSEXW             wcx;
    WCHAR                   ** cmdLine;
    int                     argc, result;
    INT_PTR                 idx;

    ghInstance          = hInstance;

    icc.dwSize          = sizeof ( icc );
    icc.dwICC           = ICC_LISTVIEW_CLASSES;
    InitCommonControlsEx ( &icc );

    // get system dialog information
    wcx.cbSize          = sizeof ( wcx );

    if ( !GetClassInfoExW ( NULL, MAKEINTRESOURCE(32770), &wcx) )
        return 0;

    // add our own stuff
    wcx.hInstance       = hInstance;
    wcx.hIcon           = LoadIconW ( hInstance, MAKEINTRESOURCE(IDR_ICO_MAIN) );
    wcx.lpszClassName   = L"wfsizeClass";

    if ( !RegisterClassExW (&wcx) )
        return 0;

    grootDir[0]         = L'\0';
    argc                = 0;
    cmdLine             = NULL;

    cmdLine             = FILE_CommandLineToArgv ( GetCommandLineW(), &argc );

    // do we have something to do from cmd line?
    if ( argc >= 2 && cmdLine != NULL )
    {
        StringCchCopyW ( grootDir, ARRAYSIZE(grootDir), cmdLine[1] );
        idx = lstrlenW ( grootDir );

        // erase any trailing backslash :-)
        if ( grootDir[idx-1] == L'\\' )
            grootDir[idx-1] = L'\0';
    }
    // no, just use the crt dir
    else
    {
        if ( !PathFromModule ( grootDir, ARRAYSIZE(grootDir)) )
        {
            MessageBoxW ( NULL, L"Please pass a folder path to wfsize.exe"
                " through the command line", app_name, 
                    MB_OK | MB_ICONEXCLAMATION );

            if ( cmdLine != NULL )
                GlobalFree ( cmdLine );

            return 0;
        }
    }

    // make initial capacity equal to list capacity
    gfSizesCapacity = LV_DEFAULT_CAPACITY;

    gfSizes = alloc_and_zero_mem (gfSizesCapacity*sizeof(__int64));

    if ( gfSizes == NULL )
    {
        MessageBoxW ( NULL, L"Unable to allocate memory for folders"
            " size table!", app_name, 
                MB_OK | MB_ICONEXCLAMATION );

        if ( cmdLine != NULL )
            GlobalFree ( cmdLine );

        return 0;
    }

    result = (int)DialogBoxW ( hInstance, MAKEINTRESOURCE(DLG_MAIN), 
        NULL, (DLGPROC)MainDlgProc );

    if ( cmdLine != NULL )
        GlobalFree ( cmdLine );

    if ( gfSizes != NULL )
        free_mem ( gfSizes );

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainDlgProc 
/*--------------------------------------------------------------------------*/
//           Type: INT_PTR CALLBACK 
//    Param.    1: HWND hwndDlg  : 
//    Param.    2: UINT uMsg     : 
//    Param.    3: WPARAM wParam : 
//    Param.    4: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 09.09.2022
//    DESCRIPTION: main dialog message handling
/*--------------------------------------------------------------------@@-@@-*/
INT_PTR CALLBACK MainDlgProc ( HWND hwndDlg, UINT uMsg, 
    WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    switch ( uMsg )
    {
        // react to monitor DPI change
        case WM_DPICHANGED:
            MainDLG_OnDPICHANGED ( hwndDlg, wParam, lParam );
            return TRUE;

        // pass WM_NOTIFY for further processing (column header click?)
        case WM_NOTIFY:

            if ( !gThreadWorking ) // do not disturb :-)
                MainDLG_OnNOTIFY ( hwndDlg, wParam, lParam );

            return TRUE;

        // WM_UPDFSIZE is received when the working thread is done
        // with a folder
        case WM_UPDFSIZE:

            if ( !MainDLG_OnUPDFSIZE ( hwndDlg, wParam, lParam ) )
            {
                MessageBoxW ( hwndDlg, 
                    L"Error adding items to list", app_name, 
                        MB_OK|MB_ICONERROR );

                // signal that we're not in the mood anymore
                if ( gThreadWorking == TRUE )
                {
                    gThreadWorking = FALSE;

                    PostThreadMessage ( gTid, WM_ABTFSIZE, 0, 0 );
                    EnableWindow ( GetDlgItem ( hwndDlg, IDC_BREAKOP ), FALSE);
                    LVEnsureVisible ( ghList, LVGetCount ( ghList ) - 1 );
                    LVSelectItem ( ghList, LVGetCount ( ghList ) - 1 );
                    SetFocus ( ghList );
                }
            }

            return TRUE;

        // WM_ENDFSIZE is received when the whole operation is finished
        case WM_ENDFSIZE:
            MainDLG_OnENDFSIZE ( hwndDlg, wParam, lParam );
            return TRUE;

        case WM_INITDIALOG:
            MainDLG_OnINITDIALOG ( hwndDlg, wParam, lParam );
            return FALSE;

        // process WM_SIZE in order to resize/move all child controls
        case WM_SIZE:
            MainDLG_OnSIZE ( hwndDlg, wParam, lParam );
            return TRUE;

        // finished resizing, scroll list into view
        case WM_EXITSIZEMOVE:

            if ( !gThreadWorking )
                LVEnsureVisible ( ghList, LVGetSelIndex ( ghList ) );

            return TRUE;

        // process WM_SIZING in order to restrict window resize
        case WM_SIZING:
            MainDLG_OnSIZING ( hwndDlg, wParam, lParam );
            return TRUE;

        case WM_COMMAND:
            MainDLG_OnCOMMAND ( hwndDlg, wParam, lParam );
            break;

        case WM_DESTROY:
            // signal the working thread that we've gone fishing :-)
            PostThreadMessage ( gTid, WM_RAGEQUIT, 0, 0 );
            return TRUE;

        case WM_CLOSE:
            EndDialog ( hwndDlg, 0 );
            return TRUE;

        default:
            break;
    }

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: IsDotOrTwoDots 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: const WCHAR * src : whatever string
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: see if src is . or ..
/*--------------------------------------------------------------------@@-@@-*/
BOOL IsDotOrTwoDots ( const WCHAR * src )
/*--------------------------------------------------------------------------*/
{
    INT_PTR len;

    if ( src == NULL )
        return FALSE;

    len = lstrlenW ( src );

    if ( (len == 1 && src[0] == L'.') || 
         (len == 2 && src[0] == L'.' && src[1] == L'.') )
            return TRUE;

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FolderSize 
/*--------------------------------------------------------------------------*/
//           Type: __int64 
//    Param.    1: const WCHAR * fpath: path to folder to calculate its size
//    Param.    2: THREAD_DATA * ptd  : pointer to data struct passed by the 
//                                      thread function
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: this will compute the size of the folder specified by 
//                 fpath, recursing through subfolders, with a 
//                 "folder in folder" limit of MAX_DEPTH. Returns total size
//                 or 0 on error.
/*--------------------------------------------------------------------@@-@@-*/
__int64 FolderSize ( const WCHAR * fpath, THREAD_DATA * ptd )
/*--------------------------------------------------------------------------*/
{
    WIN32_FIND_DATAW    ffData;
    HANDLE              hFind;
    __int64             size;
    __int64             * tmpptr;
    static UINT_PTR     depth;
    static UINT_PTR     subfolders;
    WCHAR               buf[1024];
    WCHAR               tmp[1024];
    LARGE_INTEGER       li;
    MSG                 msg;

    if ( fpath == NULL )
        return 0;

    size = 0;

    StringCchCopyW ( buf, ARRAYSIZE(buf), fpath );
    StringCchCopyW ( tmp, ARRAYSIZE(tmp), fpath ); // save a pristine copy
    StringCchCatW ( buf, ARRAYSIZE(buf), L"\\*" );

    // start enumerating
    hFind = FindFirstFileExW ( buf, FindExInfoStandard, &ffData, 
        FindExSearchNameMatch, NULL, 0 );

    if ( hFind != INVALID_HANDLE_VALUE )
    {
        do
        {
            // do we have a folder?
            if ( ffData.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY )
            {
                // skip . and .. (crt and parent folder)
                if ( !IsDotOrTwoDots ( ffData.cFileName ) )
                {
                    // complete path with folder name
                    StringCchCopyW ( buf+lstrlenW(fpath)+1, 
                        ARRAYSIZE(buf), ffData.cFileName );

                    depth++;
                    subfolders++;
                    // do recursion!
                    size += FolderSize ( buf, ptd );
                    depth--;
                }
            }
            else // just files, add to total size
            {
                li.u.HighPart = ffData.nFileSizeHigh;
                li.u.LowPart = ffData.nFileSizeLow;
                size += li.QuadPart;
            }
        }

        // stop if max. level of folder imbrication reached
        while ( ( FindNextFile ( hFind, &ffData ) != 0 ) &&
                  ( depth < MAX_DEPTH ) );
    }

    FindClose ( hFind );

    // see if abort or sudden app quit happened (WM_ABTFSIZE
    // or WM_RAGEQUIT messages) and simulate a MAX_DEPTH
    // overflow to stop recursion
    if ( PeekMessage ( &msg, (HWND)-1, WM_APP+100, WM_APP+200, PM_REMOVE ) )
        depth = MAX_DEPTH+10;

    // see if we reached the end of our current table, calculate the
    // new size and realloc accordingly
    if ( subfolders >= gfSizesCapacity - 1 )
    {
        gfSizesCapacity += (LV_DEFAULT_CAPACITY + 
                (subfolders - gfSizesCapacity - 1));

        tmpptr = realloc_and_zero_mem ( ptd->fsizes, 
            gfSizesCapacity*sizeof (__int64));

        if ( tmpptr != NULL )
            ptd->fsizes = tmpptr;
        else
            depth = MAX_DEPTH+10; // can't go further so force eject
    }

    // put results in our structure...
    ptd->size       = size;
    ptd->crtDir     = tmp;
    ptd->subfolders = subfolders;

    // ...and signal main thread that we have data
    // please don't use PostMessage, unless you worship Satan 8-)
    SendMessageW ( ptd->hParent, WM_UPDFSIZE, 0, (LPARAM)ptd );

    return size;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: Thread_FolderSize 
/*--------------------------------------------------------------------------*/
//           Type: UINT __stdcall 
//    Param.    1: void * thData : pointer to THREAD_DATA struct to pass
//                                 to FolderSize
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: thread function for _beginthreadex
/*--------------------------------------------------------------------@@-@@-*/
UINT __stdcall Thread_FolderSize ( void * thData )
/*--------------------------------------------------------------------------*/
{
    THREAD_DATA * ptd;

    if ( thData == NULL )
        return FALSE;

    ptd = (THREAD_DATA *)thData;

    // do actual work
    ptd->size = FolderSize ( ptd->fpath, ptd );

    // signal end of work an return ASAP, hence PostMessage
    return (UINT) PostMessageW ( ptd->hParent, 
        WM_ENDFSIZE, 1, (LPARAM)thData );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FILE_CommandLineToArgv
/*--------------------------------------------------------------------------*/
//           Type: TCHAR **
//    Param.    1: TCHAR * CmdLine: pointer to app commandline
//    Param.    2: int * _argc    : pointer to var to receive arg. count
/*--------------------------------------------------------------------------*/
//         AUTHOR: Thanks to Alexander A. Telyatnikov,
//                 http://alter.org.ua/en/docs/win/args/
//           DATE: 28.09.2020
//    DESCRIPTION: Nice little function! Takes a command line and breaks it
//                 into null term. strings (_argv) indexed by an array of
//                 pointers (argv). All necessary mem. is allocated
//                 dynamically, _argv and argv being stored one after the
//                 other. It returns the pointers table, which you must free
//                 with GlobalFree after getting the job done.
/*--------------------------------------------------------------------@@-@@-*/
WCHAR ** FILE_CommandLineToArgv ( WCHAR * CmdLine, int * _argc )
/*--------------------------------------------------------------------------*/
{
    WCHAR       ** argv;
    WCHAR       *  _argv;
    ULONG       len;
    ULONG       argc;
    WCHAR       a;
    ULONG       i, j;

    BOOLEAN     in_QM;
    BOOLEAN     in_TEXT;
    BOOLEAN     in_SPACE;

    if ( CmdLine == NULL || _argc == NULL )
        return NULL;

    len         = lstrlenW ( CmdLine );
    i           = ((len+2)/2)*sizeof(PVOID) + sizeof(PVOID);

    argv        = (WCHAR**)GlobalAlloc(GMEM_FIXED,
                    (i + (len+2)*sizeof(WCHAR) + 4096 + 1) & (~4095) );

    if ( argv == NULL )
        return NULL;

    _argv       = (WCHAR*)(((UCHAR*)argv)+i);

    argc        = 0;
    argv[argc]  = _argv;
    in_QM       = FALSE;
    in_TEXT     = FALSE;
    in_SPACE    = TRUE;
    i           = 0;
    j           = 0;

    while ( a = CmdLine[i] )
    {
        if ( in_QM )
        {
            if ( a == L'\"' )
            {
                in_QM = FALSE;
            }
            else
            {
                _argv[j] = a;
                j++;
            }
        }
        else
        {
            switch ( a )
            {
                case L'\"':
                    in_QM   = TRUE;
                    in_TEXT = TRUE;

                    if ( in_SPACE )
                    {
                        argv[argc] = _argv+j;
                        argc++;
                    }

                    in_SPACE = FALSE;
                    break;

                case L' ':
                case L'\t':
                case L'\n':
                case L'\r':

                    if ( in_TEXT )
                    {
                        _argv[j] = L'\0';
                        j++;
                    }

                    in_TEXT  = FALSE;
                    in_SPACE = TRUE;
                    break;

                default:
                    in_TEXT  = TRUE;

                    if ( in_SPACE )
                    {
                        argv[argc] = _argv+j;
                        argc++;
                    }

                    _argv[j] = a;
                    j++;
                    in_SPACE = FALSE;
                    break;
            }
        }

        i++;
    }

    _argv[j]    = L'\0';
    argv[argc]  = NULL;

    (*_argc)    = argc;

    return argv;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: EnumChildProc 
/*--------------------------------------------------------------------------*/
//           Type: BOOL CALLBACK 
//    Param.    1: HWND hwndChild: child control HWND
//    Param.    2: LPARAM lParam : custom data (pointer to RECT with parent
//                                 coord.)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: called by EnumChildWindows on WM_SIZE, in order to 
//                 reposition and resize controls. Values are scaled 
//                 with respect to current display dpi. 
/*--------------------------------------------------------------------@@-@@-*/
BOOL CALLBACK EnumChildProc ( HWND hwndChild, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    LONG    idChild;
    LONG    newLeft;
    LONG    newTop;
    LONG    newWidth;
    LONG    newHeight;
    RECT    * rcParent;

	idChild = GetWindowLongW ( hwndChild, GWL_ID );

	rcParent = (RECT *)lParam;

    switch ( idChild )
    {
        // top status label
        case IDC_FLABEL:
            newWidth = rcParent->right - MulDiv((8+70),gDpi,DEFAULT_DPI);
            newHeight = MulDiv(50,gDpi,DEFAULT_DPI);

            MoveWindow ( hwndChild, MulDiv(70,gDpi,DEFAULT_DPI), 
                MulDiv(8,gDpi,DEFAULT_DPI), newWidth, newHeight, TRUE );

            ShowWindow ( hwndChild, SW_SHOW );
            break;

        // break op. button
        case IDC_BREAKOP:
            newLeft = rcParent->right - MulDiv((120 + BNWIDTH),
                gDpi,DEFAULT_DPI);

            newTop = rcParent->bottom - MulDiv((13 + BNHEIGHT),
                gDpi,DEFAULT_DPI);

            MoveWindow ( hwndChild, newLeft, newTop, 
                MulDiv(BNWIDTH,gDpi,DEFAULT_DPI), 
                    MulDiv(BNHEIGHT,gDpi,DEFAULT_DPI), TRUE );

            ShowWindow ( hwndChild, SW_SHOW );
            break;

        // close btn.
        case IDOK:
            newLeft = rcParent->right - MulDiv((13 + BNWIDTH),
                gDpi,DEFAULT_DPI);

            newTop = rcParent->bottom - MulDiv((13 + BNHEIGHT),
                gDpi,DEFAULT_DPI);

            MoveWindow ( hwndChild, newLeft, newTop, 
                MulDiv(BNWIDTH,gDpi,DEFAULT_DPI), MulDiv(BNHEIGHT,
                    gDpi,DEFAULT_DPI), TRUE );

            ShowWindow ( hwndChild, SW_SHOW );
            break;

        // listview
        case IDC_FLIST:
            newWidth = rcParent->right - (2*MulDiv(8,gDpi,DEFAULT_DPI));
            newHeight = rcParent->bottom - MulDiv((65+50),gDpi,DEFAULT_DPI);

            MoveWindow ( hwndChild, MulDiv(8,gDpi,DEFAULT_DPI), 
                MulDiv(65,gDpi,DEFAULT_DPI), newWidth, newHeight, TRUE );

            ShowWindow ( hwndChild, SW_SHOW );
            break;
        
        default:
            break;
    }

	return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainDLG_OnCOMMAND 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: WM_COMMAND handler for our dialog
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainDLG_OnCOMMAND ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    switch ( GET_WM_COMMAND_ID(wParam, lParam) )
    {
        case IDM_SAVECSV:

            if ( !SaveFolderListToCSV ( hWnd ) )
                MessageBoxW ( hWnd, L"Unable to save folder "
                    "list to CSV file!", app_name, MB_OK|MB_ICONEXCLAMATION);

            break;

        case IDOK:
            EndDialog ( hWnd, TRUE );
            return TRUE;

        case IDCANCEL:
            EndDialog ( hWnd, FALSE );
            return TRUE;

        case IDC_BREAKOP:
            // user pressed the abort button, so signal to our thread,
            // disable the panic button and scroll list into view
            if ( gThreadWorking == TRUE )
            {
                gThreadWorking = FALSE;

                PostThreadMessage ( gTid, WM_ABTFSIZE, 0, 0 );
                EnableWindow ( GetDlgItem ( hWnd, IDC_BREAKOP ), FALSE );
                LVEnsureVisible ( ghList, LVGetCount ( ghList ) - 1 );
                LVSelectItem ( ghList, LVGetCount ( ghList ) - 1 );
                SetFocus ( ghList );
            }

            return TRUE;

        default:
            break;
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainDLG_OnSIZING 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: WM_SIZING handler for our dialog, called when drag-sizing
//                 the window with the mouse. Used for restricting the 
//                 min. accepted size for the window. 
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainDLG_OnSIZING ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    RECT    * wr;
    int     ww, wh;

    wr = (RECT *)lParam;

    if ( wr != NULL )
    {
        ww = MulDiv(WWIDTH,gDpi,DEFAULT_DPI);
        wh = MulDiv(WHEIGHT,gDpi,DEFAULT_DPI);

        switch ( wParam )
        {
            // sides
            case WMSZ_BOTTOM:
                if ( wr->bottom - wr->top < wh )
                    wr->bottom = wr->top + wh;
                break;

            case WMSZ_TOP:
                if ( wr->bottom - wr->top < wh )
                    wr->top = wr->bottom - wh;
                break;

            case WMSZ_LEFT:
                if ( wr->right - wr->left < ww )
                    wr->left = wr->right - ww;
                break;

            case WMSZ_RIGHT:
                if ( wr->right - wr->left < ww )
                    wr->right = wr->left + ww;
                break;

            // corners
            case WMSZ_BOTTOMLEFT:
                if ( wr->bottom - wr->top < wh )
                    wr->bottom = wr->top + wh;
                if ( wr->right - wr->left < ww )
                    wr->left = wr->right - ww;
                break;

            case WMSZ_BOTTOMRIGHT:
                if ( wr->bottom - wr->top < wh )
                    wr->bottom = wr->top + wh;
                if ( wr->right - wr->left < ww )
                    wr->right = wr->left + ww;
                break;

            case WMSZ_TOPLEFT:
                if ( wr->bottom - wr->top < wh )
                    wr->top = wr->bottom - wh;
                if ( wr->right - wr->left < ww )
                    wr->left = wr->right - ww;
                break;

            case WMSZ_TOPRIGHT:
                if ( wr->bottom - wr->top < wh )
                    wr->top = wr->bottom - wh;
                if ( wr->right - wr->left < ww )
                    wr->right = wr->left + ww;
                break;

            default:
                break;
        }
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainDLG_OnSIZE 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 11.09.2022
//    DESCRIPTION: WM_SIZE message handler for our dlg
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainDLG_OnSIZE ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    RECT    r;

    // retrieve window coords and visit every children
    // moving them as needed
    GetClientRect ( hWnd, &r );
    EnumChildWindows ( hWnd, EnumChildProc, (LPARAM)&r );

    RedrawWindow ( GetDlgItem ( hWnd, IDC_BREAKOP ), 
        NULL, NULL, RDW_ERASE|RDW_INVALIDATE );

    RedrawWindow ( GetDlgItem ( hWnd, IDOK ), 
        NULL, NULL, RDW_ERASE|RDW_INVALIDATE );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainDLG_OnUPDFSIZE 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: message handler for the WM_UPDFSIZE, sent by our thread
//                 when data is available to be inserted into the list.
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainDLG_OnUPDFSIZE ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    THREAD_DATA     * ptd;
    WCHAR           f[1024];
    WCHAR           s[128];
    UINT_PTR        index;

    ptd = (THREAD_DATA *)lParam;

    if ( ptd != NULL )
    {
        StringCchPrintfW ( f, ARRAYSIZE(f), L"%.2f", 
            ((float)(ptd->size))/1024);

        // format the number with the default thousand separator
        GetNumberFormatW ( LOCALE_SYSTEM_DEFAULT, 
            LOCALE_NOUSEROVERRIDE, f, NULL, s, ARRAYSIZE(s) );

        index = LVGetCount ( ptd->hList );

        // add result to the list and store index to folder size 
        // in the list as lParam member of LVITEM struct so we can sort 
        // the list at a later time. NOTE: don't store pointers!
        // this mem will be realloc'd and any ptr will be invalid!
        gfSizes = ptd->fsizes;

        __try
        {
            gfSizes[index] = ptd->size;
        }
        __except ( EXCEPTION_EXECUTE_HANDLER )
        {
            return FALSE; // great booboo happened
        }
    
        LVInsertItemEx ( ptd->hList, index, -1, ptd->crtDir, 
            (LPARAM)index );

        LVSetItemText ( ptd->hList, index, 1, s );

        // from time to time, update total folders and scroll list
        // into view
        if ( ptd->subfolders % 256 == 0 )
        {
            StringCchPrintfW ( f, ARRAYSIZE(f), L"%ls (%zu subfolders "
                "processed)", grootDir, ptd->subfolders );

            SetDlgItemTextW ( hWnd, IDC_FLABEL, f );
            LVEnsureVisible ( ptd->hList, index );
        }
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainDLG_OnENDFSIZE 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: handler for the WM_ENDFSIZE private message, sent when 
//                 the worker thread is done.
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainDLG_OnENDFSIZE ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    WCHAR           f[1024];
    WCHAR           s[128];
    THREAD_DATA     * ptd;
    SYSTEMTIME      st_stop, st_result;
    FILETIME        ft_start, ft_stop, fl_result;
    ULARGE_INTEGER  ul_start, ul_stop, ul_result;
    WORD            hr, min, sec, msec;

    gThreadWorking = FALSE;

    if ( gThandle )
    {
        CloseHandle ( (HANDLE)gThandle );
        gThandle = 0;
    }

    ptd = (THREAD_DATA *)lParam;

    // scroll list into view, update totals and disable panic button :-)
    if ( ptd != NULL )
    {
        GetLocalTime ( &st_stop );

        SystemTimeToFileTime ( &gTimeStart, &ft_start );
        SystemTimeToFileTime ( &st_stop, &ft_stop );

        ul_stop.u.HighPart = ft_stop.dwHighDateTime;
        ul_stop.u.LowPart = ft_stop.dwLowDateTime;

        ul_start.u.HighPart = ft_start.dwHighDateTime;
        ul_start.u.LowPart = ft_start.dwLowDateTime;

        ul_result.QuadPart = ul_stop.QuadPart - ul_start.QuadPart;
        fl_result.dwHighDateTime = ul_result.u.HighPart;
        fl_result.dwLowDateTime = ul_result.u.LowPart;

        FileTimeToSystemTime ( &fl_result, &st_result );

        hr   = st_result.wHour;
        min  = st_result.wMinute;
        sec  = st_result.wSecond;
        msec = st_result.wMilliseconds;

        LVEnsureVisible ( ptd->hList, LVGetCount ( ptd->hList ) - 1 );
        LVSelectItem ( ptd->hList, LVGetCount ( ptd->hList ) - 1 );
        SetFocus ( ptd->hList );

        StringCchPrintfW ( f, ARRAYSIZE(f), L"%.2f", 
            ((float)(ptd->size))/1024 );

        GetNumberFormatW ( LOCALE_SYSTEM_DEFAULT, 
            LOCALE_NOUSEROVERRIDE, f, NULL, s, ARRAYSIZE(s) );

        StringCchPrintfW ( f, ARRAYSIZE(f), L"%ls (%zu subfolders,"
            " %02uh:%02um:%02us:%03ums), %ls KBytes total size", grootDir, 
                ptd->subfolders, hr, min, sec, msec, s );

        SetDlgItemTextW ( hWnd, IDC_FLABEL, f );
    }

    EnableWindow ( GetDlgItem ( hWnd, IDC_BREAKOP ), FALSE );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainDLG_OnINITDIALOG 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: handler for WM_INITDIALOG. Initialize the ListView ctrl.
//                 prepare data structure and start the worker thread.
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainDLG_OnINITDIALOG ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    SetWindowTextW ( hWnd, app_name_ex );

    ghList  = GetDlgItem ( hWnd, IDC_FLIST );
    
    // save current display DPI for later
    gDpi    = GetWindowDPI ( hWnd ); 

    if ( ghList != NULL )
    {
        // set additional styles for listview
        // (dbl. buffering avoids flicker)
        ListView_SetExtendedListViewStyle ( ghList,
            LVS_EX_FULLROWSELECT | LVS_EX_INFOTIP | LVS_EX_DOUBLEBUFFER);

        LVInsertColumn ( ghList, 0, L"Full path to folder", 
            LVCFMT_LEFT, 750, -1 );
        LVInsertColumn ( ghList, 1, L"Folder size (KBytes)", 
            LVCFMT_LEFT, 130, -1 );

        if ( grootDir[0] != L'\0' )
        {
            gTtd.fpath      = grootDir;
            gTtd.hList      = ghList;
            gTtd.hParent    = hWnd;
            gTtd.fsizes     = gfSizes;
            
            gThreadWorking  = TRUE;

            SendMessage ( ghList, LVM_SETITEMCOUNT, LV_DEFAULT_CAPACITY, 0 );
            EnableWindow ( GetDlgItem ( hWnd, IDC_BREAKOP ), TRUE );

            // punch the clock
            GetLocalTime ( &gTimeStart );

            // start the working thread
            gThandle = _beginthreadex ( NULL, 0, 
                Thread_FolderSize, &gTtd, 0, &gTid );
        }
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainDLG_OnNOTIFY 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: handle the WM_NOTIFY message for our dialog. Resort  
//                 strictly to messages from our listview only.
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainDLG_OnNOTIFY ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    NMHDR           * lpnm;
    NMLVKEYDOWN     * lpvk;
    HWND            hList;
    static BOOL     ctrl_pressed, s_pressed;

    lpnm = (NMHDR *)lParam;

    if ( lpnm == NULL )
        return FALSE;

    // is it from the list?
    if ( lpnm->idFrom == IDC_FLIST )
    {
        switch (lpnm->code)
        {
            // is it a column header clicky?
            case LVN_COLUMNCLICK:
                hList = lpnm->hwndFrom;

                // sort list items and set the column header arrow
                LVSortItems ( hList, gAscending, (LPARAM)CompareListItems );

                if ( gAscending )
                    LVSetHeaderSortImg ( hList, 1, UP_ARROW );
                else
                    LVSetHeaderSortImg ( hList, 1, DOWN_ARROW );

                gAscending = !gAscending;

                // select first item and scroll into view
                LVSelectItem ( hList, 0 );
                LVEnsureVisible ( hList, 0 );
                break;

            // mouse right-clicky
            case NM_RCLICK:
                ContextMenu ( hWnd, IDR_LPOP );
                break;

            // a crude ctrl+s simulation
            case LVN_KEYDOWN:
                lpvk = (NMLVKEYDOWN *)lParam;
                switch ( lpvk->wVKey )
                {
                    case VK_CONTROL:
                        ctrl_pressed = TRUE;
                        break;

                    case VK_S:
                        if ( ctrl_pressed )
                            s_pressed = TRUE;
                        break;

                    default:
                        ctrl_pressed = FALSE;
                        s_pressed = FALSE;
                        break;
                } 

                if ( ctrl_pressed && s_pressed )
                {
                    if ( !SaveFolderListToCSV ( hWnd ) )
                        MessageBoxW ( hWnd, L"Unable to save folder "
                            "list to CSV file!", app_name, 
                                MB_OK|MB_ICONEXCLAMATION);
                    
                    ctrl_pressed = FALSE;
                    s_pressed = FALSE;
                }
                break;

            default:
                break;
        }
    }

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: CompareListItems 
/*--------------------------------------------------------------------------*/
//           Type: int CALLBACK 
//    Param.    1: LPARAM lParam1    : LVITEM.lParam
//    Param.    2: LPARAM lParam2    : LVITEM.lParam
//    Param.    3: LPARAM lParamSort : custom data that was passed to
//                                     LVM_SORTITEMS message as wParam
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: callback function called by the LVM_SORTITEMS message.
//                 Compares what's on LVITEM.lParam (previously stored 
//                 by the list when item was created) for two adjacent items.  
//                 Returns -, + or 0, according to <, > or = relation between
//                 lParam1 and lParam2.
/*--------------------------------------------------------------------@@-@@-*/
int CALLBACK CompareListItems
    ( LPARAM lParam1, LPARAM lParam2, LPARAM lParamSort )
/*--------------------------------------------------------------------------*/
{
    __int64 i1, i2;

    __try
    {
        i1 = gfSizes[lParam1];
        i2 = gfSizes[lParam2];
    }
    __except ( EXCEPTION_EXECUTE_HANDLER )
    {
        return 0;
    }

    if ( i1 > i2 )
        return (lParamSort == TRUE) ? 1 : -1;

    if ( i1 < i2 )
        return (lParamSort == TRUE) ? -1 : 1;

    return 0;

    // do not be tempted to optimize by replacing the whole shebang with
    // "return (lParamSort == TRUE) ? i1 - i2 : i2 - i1;", it WILL return
    // truncated values :-)
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: ContextMenu 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd  : parent hwnd
//    Param.    2: int menuId : resource ID of the manu that pops
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 16.09.2022
//    DESCRIPTION: make a pop!
/*--------------------------------------------------------------------@@-@@-*/
BOOL ContextMenu ( HWND hWnd, int menuId )
/*--------------------------------------------------------------------------*/
{
    static HMENU    hPop, hSub;
    POINT           pt;
    DWORD           states[2] = { MF_GRAYED, MF_ENABLED };
    BOOL            state;

    hPop = LoadMenuW ( GetModuleHandleW ( NULL ),
        MAKEINTRESOURCEW ( menuId ));

    if ( !hPop )
        return FALSE;

    hSub    = GetSubMenu ( hPop, 0 );
    state   = ( LVGetCount ( ghList ) != 0 );

    EnableMenuItem ( hSub, IDM_SAVECSV, states[state] );
    GetCursorPos ( &pt );

    TrackPopupMenuEx ( hSub, 
        TPM_LEFTALIGN|TPM_LEFTBUTTON|TPM_RIGHTBUTTON, 
            pt.x, pt.y, hWnd, NULL );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: SaveFolderListToCSV 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd : parent hwnd
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 16.09.2022
//    DESCRIPTION: display the Save dialog and call LVItemsToCSV
/*--------------------------------------------------------------------@@-@@-*/
BOOL SaveFolderListToCSV ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    OPENFILENAMEW   ofn;
    DWORD           dlgstyle;
    WCHAR           csvname[MAX_PATH];

    RtlZeroMemory ( &ofn, sizeof ( ofn ) );

    dlgstyle = OFN_EXPLORER|OFN_PATHMUSTEXIST|
        OFN_HIDEREADONLY|OFN_OVERWRITEPROMPT;

    StringCchCopyW ( csvname, ARRAYSIZE(csvname), L"folderlist" );

    ofn.lStructSize     = sizeof ( ofn );
    ofn.hInstance       = GetModuleHandleW ( NULL );
    ofn.hwndOwner       = hWnd;
    ofn.Flags           = dlgstyle;
    ofn.lpstrFilter     = opn_filter;
    ofn.nFilterIndex    = 1;
    ofn.lpstrFile       = csvname;
    ofn.nMaxFile        = ARRAYSIZE ( csvname );
    ofn.lpstrTitle      = sav_title;
    ofn.lpstrDefExt     = opn_defext;
    ofn.lpstrInitialDir = grootDir; // would be nice to pass this 
                                    // as a param, not use a global :-)

    if ( GetSaveFileNameW ( &ofn ) )
        return LVItemsToCSV ( ghList, ofn.lpstrFile );

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVItemsToCSV 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hList          : ListView ctrl. handle
//    Param.    2: const WCHAR * fname : path to csv file to save to
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 16.09.2022
//    DESCRIPTION: this saves all the items and the first level of subitems 
//                 from a listview control into a CSV file that you can open 
//                 in Excel. 
/*--------------------------------------------------------------------@@-@@-*/
BOOL LVItemsToCSV ( HWND hList, const WCHAR * fname )
/*--------------------------------------------------------------------------*/
{
    HANDLE  hFile;
    size_t  i, items;
    WCHAR   item[1024];     // holds item text
    WCHAR   subitem[64];    // holds subitem text
    WCHAR   s[1280];        // holds a csv line, in unicode
    CHAR    b[1280];        // holds the ansi version
    DWORD   towrite, written;
    BOOL    result = TRUE;

    if ( hList == NULL || fname == NULL )
        return FALSE;

    items = LVGetCount ( hList );

    if ( items == 0 ) // nothing to do
        return FALSE;

    hFile = CreateFile ( fname, GENERIC_READ|GENERIC_WRITE,
        FILE_SHARE_READ|FILE_SHARE_WRITE, NULL, CREATE_ALWAYS, 0, 0 );
    
    if ( hFile == INVALID_HANDLE_VALUE )
        return FALSE;

    for ( i = 0; i < items; i++ )
    {
        item[0]         = L'\0';
        subitem[0]      = L'\0';

        LVGetItemText ( hList, i, 0, item, ARRAYSIZE(item) );
        LVGetItemText ( hList, i, 1, subitem, ARRAYSIZE(subitem) );

        StringCchPrintfW ( s, ARRAYSIZE(s), L"\"%ls\",\"%ls\"\r\n",
            item, subitem );

        // make a regular text line 
        towrite = WideCharToMultiByte ( GetACP(), 0, s, -1, b,
            ARRAYSIZE(b), NULL, NULL );

        // write to file and accumulate errors, if any :-)
        result &= WriteFile ( hFile, b, towrite-1, &written, NULL );
    }
    
    CloseHandle ( hFile );

    return result;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: PathFromModule 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: WCHAR * buf   : buffer to hold the result
//    Param.    2: DWORD cchDest : buf size, in (w)chars
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 16.09.2022
//    DESCRIPTION: copies the path from which the program was started in the
//                 buffer buf. Returns TRUE on success, FALSE on failure
/*--------------------------------------------------------------------@@-@@-*/
BOOL PathFromModule ( WCHAR * buf, DWORD cchDest )
/*--------------------------------------------------------------------------*/
{
    HMODULE     hMod;
    DWORD       len;

    if ( buf == NULL || cchDest == 0 )
        return FALSE;

    hMod    = GetModuleHandleW ( NULL );
    len     = GetModuleFileNameW ( hMod, buf, cchDest );

    if ( len == 0 )
    {
        buf[0] = L'\0';
        return FALSE;
    }

    // hit the first path separator (from end to start)
    while ( buf[len] != L'\\' )
        len--;

    // and end the string here
    buf[len] = L'\0';

    return TRUE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: GetWindowDPI 
/*--------------------------------------------------------------------------*/
//           Type: WORD 
//    Param.    1: HWND hWnd : window to get DPI for
/*--------------------------------------------------------------------------*/
//         AUTHOR: Colin Finck, Adrian Petrila, YO3GFH
//           DATE: 18.09.2022
//    DESCRIPTION: thanks to Colin Finck for the blog post, very nice article
// https://building.enlyze.com/posts/writing-win32-apps-like-its-2020-part-3/
/*--------------------------------------------------------------------@@-@@-*/
WORD GetWindowDPI ( HWND hWnd )
/*--------------------------------------------------------------------------*/
{
    HMODULE             hShcore;
    PGetDpiForMonitor   pGetDpiForMonitor;
    HMONITOR            hMonitor;
    UINT                uiDpiX;
    UINT                uiDpiY;
    HRESULT             hr;
    HDC                 hScreenDC;
    int                 iDpiX;

    // Try to get the DPI setting for the monitor where the given 
    // window is located. This API is Windows 8.1+.
    hShcore = LoadLibraryW ( L"shcore" );

    if ( hShcore )
    {
        pGetDpiForMonitor = ( PGetDpiForMonitor )
            ( GetProcAddress ( hShcore, "GetDpiForMonitor" ) );

        if ( pGetDpiForMonitor )
        {
            hMonitor = MonitorFromWindow ( hWnd, MONITOR_DEFAULTTOPRIMARY );
            hr = pGetDpiForMonitor ( hMonitor, 0, &uiDpiX, &uiDpiY );

            if (SUCCEEDED(hr))
            {
                return (WORD)(uiDpiX);
            }
        }
    }

    // We couldn't get the window's DPI above, so get the DPI of the primary 
    // monitor using an API that is available in all Windows versions.
    hScreenDC = GetDC(0);
    iDpiX = GetDeviceCaps ( hScreenDC, LOGPIXELSX );
    ReleaseDC ( 0, hScreenDC );

    return (WORD)(iDpiX);
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: MainDLG_OnDPICHANGED 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hWnd     : 
//    Param.    2: WPARAM wParam : 
//    Param.    3: LPARAM lParam : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 18.09.2022
//    DESCRIPTION: handler for WM_DPICHANGED - sent when display res has 
//                 changed. Update our global var and resize window to 
//                 recommended values.
/*--------------------------------------------------------------------@@-@@-*/
BOOL MainDLG_OnDPICHANGED ( HWND hWnd, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    // window coords
    int     wx, wy, ww, wh;
    RECT    * r;

    gDpi = LOWORD ( wParam );
    r = (RECT *)lParam;    

    if ( r == NULL )
        return FALSE;

    wx = r->left;
    wy = r->top;
    ww = r->right - r->left;
    wh = r->bottom - r->top;

    // make window dimensions bigger by 1 px to
    // trigger a WM_SIZE and scale all children
    return SetWindowPos ( hWnd, NULL, wx, wy, ww+1, wh+1, 
        SWP_NOZORDER | SWP_NOACTIVATE);
}
