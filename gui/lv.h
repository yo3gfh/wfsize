

#ifndef _LV_H
#define _LV_H

#include <windows.h>

typedef enum {
    NO_ARROW,
    UP_ARROW,
    DOWN_ARROW
} LV_ARROW;

BOOL LVSetItemText 
    ( HWND hList, int nItem, int nSubItem, const WCHAR * lpszText );

BOOL LVGetItemText 
    ( HWND hList, int nItem, int nSubItem, WCHAR * lpszText, int cchMax );

BOOL    LVClear             ( HWND hList );
BOOL    LVDeleteItem        ( HWND hList, int item );
void    LVDeleteSelection   ( HWND hList );
void    LVSelectAll         ( HWND hList );
void    LVSelectItem        ( HWND hList, int index );
void    LVFocusItem         ( HWND hList, int index );
void    LVUnselectItem      ( HWND hList, int index );
BOOL    LVEnsureVisible     ( HWND hList, int index );
BOOL    LVSortItems         ( HWND hList, WPARAM wParam, LPARAM lParam );
BOOL    LVSetHeaderSortImg  ( HWND hList, int index, LV_ARROW lvArrow );
void    BeginDraw           ( HWND hList );
void    EndDraw             ( HWND hList );

int LVInsertColumn 
    ( HWND hList, int nCol, const WCHAR * lpszColumnHeading, 
    int nFormat, int nWidth, int nSubItem );

int LVInsertItem 
    ( HWND hList, int nItem, int nImgIndex, const WCHAR * lpszItem );

int LVInsertItemEx
    ( HWND hList, int nItem, int nImgIndex, const WCHAR * lpszItem,
    LPARAM lParam );

int     LVGetSelIndex       ( HWND hList );
int     LVGetCount          ( HWND hList );

#endif // _LV_H

