#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#define MAX_DEPTH 96    // max. level of "folder in folder" :-)
#define MAX_LEN 55      // max. path length that is displayed
                        // takes effect in console window only, not if
                        // the output is redirected to a text file 
#define UNICODE

#ifndef UNICODE
    #error UNICODE symbol must be defined!
#endif

#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <strsafe.h>

BOOL IsDotOrTwoDots ( const WCHAR * src );
BOOL IsConsoleRedirected ( void );
__int64 FolderSize  ( const WCHAR * fpath, UINT_PTR max_depth );

/*-@@+@@--------------------------------------------------------------------*/
//       Function: wmain 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: int argc      : 
//    Param.    2: WCHAR ** argv : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.08.2022
//    DESCRIPTION: main program loop
//
/*--------------------------------------------------------------------@@-@@-*/
int wmain ( int argc, WCHAR ** argv )
/*--------------------------------------------------------------------------*/
{
    UINT_PTR                    barlen;
    INT_PTR                     iterations, idx;
    WCHAR                       bar[128];
    HANDLE                      hStdout;
    CONSOLE_SCREEN_BUFFER_INFO  csbiInfo;
    WORD                        wOldColorAttrs;

    if ( argc < 2 )
    {
        fwprintf ( stderr, 
            L"\n*** fsize v1.0, copyright (c) 2022"
                L" by Adrian Petrila, YO3GFH ***\n\n"
            L"Prints folder size, along with each subfolder, "
                L"if any. By default, recurses %u folder levels "
            L"unless told otherwise by the second parameter. "
                L"A value of 1 disables recursion\n\n"
            L"\tUsage: fsize <full folder path> [max. recursions]\n\n", 
            MAX_DEPTH );

        return 1;
    }

    // argc = 2, default case 
    iterations = MAX_DEPTH;

    if ( argc > 2 )
    {
        iterations = wcstoul ( argv[2], NULL, 10 );

        // some bogus value for argv[2]
        if ( ( iterations < 1 ) || ( iterations > MAX_DEPTH ) )
            iterations = MAX_DEPTH;  
    }

    barlen = 78; // console width
    bar[barlen] = L'\0';

    // make a nice top line
    while ( barlen-- )
        bar[barlen] = L'-';

    // useless code to paint passed params bright green
    hStdout = GetStdHandle ( STD_OUTPUT_HANDLE );
    GetConsoleScreenBufferInfo ( hStdout, &csbiInfo );

    wOldColorAttrs = csbiInfo.wAttributes;

    fwprintf ( stdout, L"%ls\n", bar );
    fwprintf ( stdout, L" Getting data for " );

    SetConsoleTextAttribute ( hStdout, FOREGROUND_GREEN |
        FOREGROUND_INTENSITY );

    fwprintf ( stdout, L"[%ls]", argv[1] );
    
    SetConsoleTextAttribute ( hStdout, wOldColorAttrs );

    fwprintf ( stdout, L" with a maximum of " );

    SetConsoleTextAttribute ( hStdout, FOREGROUND_GREEN |
        FOREGROUND_INTENSITY );

    fwprintf ( stdout, L"[%zd]", iterations );

    SetConsoleTextAttribute ( hStdout, wOldColorAttrs );

    fwprintf ( stdout, L" folder depth levels...\n" );
    fwprintf ( stdout, L"%ls\n", bar );

    idx = lstrlenW ( argv[1] );

    if ( argv[1][idx-1] == L'\\' )
        argv[1][idx-1] = L'\0';

    FolderSize ( argv[1], iterations );

    return 0;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: FolderSize 
/*--------------------------------------------------------------------------*/
//           Type: __int64 
//    Param.    1: const WCHAR * fpath: path to folder to calculate size
//    Param.    2: UINT_PTR max_depth : max. number of recursions
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 04.09.2022
//    DESCRIPTION: takes a folder path as an argument and recursively goes 
//                 from there calculating each subfolder size, up to a
//                 maximum of max_depth iterations (folder in folder depth).
//                 Returns folder size, in bytes, or 0 on error.
/*--------------------------------------------------------------------@@-@@-*/
__int64 FolderSize ( const WCHAR * fpath, UINT_PTR max_depth )
/*--------------------------------------------------------------------------*/
{
    WIN32_FIND_DATA ffData;
    HANDLE          hFind;
    __int64         size;
    static UINT_PTR depth;
    WCHAR           buf[1024];
    WCHAR           tmp[1024];
    WCHAR           s[128], f[128];
    LARGE_INTEGER   li;

    if ( fpath == NULL )
        return 0;

    size = 0;

    StringCchCopyW ( buf, ARRAYSIZE(buf), fpath );
    StringCchCopyW ( tmp, ARRAYSIZE(tmp), fpath );
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
                    // do recursion!
                    size += FolderSize ( buf, max_depth );
                    depth--;
                }
            }
            else // just files
            {
                li.u.HighPart = ffData.nFileSizeHigh;
                li.u.LowPart = ffData.nFileSizeLow;
                size += li.QuadPart;
            }
        }

        // stop if max. level of folder imbrication reached
        while ( ( FindNextFile ( hFind, &ffData ) != 0 ) &&
                  ( depth < max_depth ) );
    }

    FindClose ( hFind );

    // if we're not redirected to text, chop path length so
    // it will fit in the console
    if ( !IsConsoleRedirected() )
    {
        tmp[MAX_LEN] = L'\0';
        tmp[MAX_LEN-1] = L'.';
        tmp[MAX_LEN-2] = L'.';
        tmp[MAX_LEN-3] = L'.';
    }
    
    StringCchPrintfW ( f, ARRAYSIZE(f), L"%.2f", ((float)size)/1024);

    GetNumberFormatW ( LOCALE_SYSTEM_DEFAULT, LOCALE_NOUSEROVERRIDE, 
        f, NULL, s, ARRAYSIZE(s) );

    fwprintf ( stdout, L"%-*ls %*ls KB\n", 
        MAX_LEN, tmp, 18, s );

    return size;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: IsDotOrTwoDots 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: const WCHAR * src : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 29.08.2022
//    DESCRIPTION: <lol>
//
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
//       Function: IsConsoleRedirected 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: void : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 30.08.2022
//    DESCRIPTION: <lol>
//
/*--------------------------------------------------------------------@@-@@-*/
BOOL IsConsoleRedirected ( void )
/*--------------------------------------------------------------------------*/
{
    DWORD foType;

    foType = GetFileType ( GetStdHandle ( STD_OUTPUT_HANDLE ) );

    return ( foType == FILE_TYPE_DISK );
}
