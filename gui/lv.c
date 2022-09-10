
#pragma warn(disable: 2008 2118 2228 2231 2030 2260)

#include        "main.h"
#include        "lv.h"
#include        <windows.h>
#include        <windowsx.h>
#include        <commctrl.h>

//
// listview management functions
//

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVInsertColumn 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hList                     : Listview item
//    Param.    2: int nCol                       : col. index
//    Param.    3: const WCHAR * lpszColumnHeading: col. title
//    Param.    4: int nFormat                    : indentation
//    Param.    5: int nWidth                     : col. width
//    Param.    6: int nSubItem                   : index of subitem
//                                                  assoc. wth column
//                                                  (-1 if none)
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: Insert a column into a LV control
/*--------------------------------------------------------------------@@-@@-*/
int LVInsertColumn ( HWND hList, int nCol, const WCHAR * lpszColumnHeading, 
    int nFormat, int nWidth, int nSubItem )
/*--------------------------------------------------------------------------*/
{
    LVCOLUMNW column;

    column.mask         = LVCF_TEXT|LVCF_FMT;
    column.pszText      = (WCHAR *)lpszColumnHeading;
    column.cchTextMax   = lstrlenW ( lpszColumnHeading );
    column.fmt          = nFormat;

    if ( nWidth != -1 )
    {
        column.mask |= LVCF_WIDTH;
        column.cx = nWidth;
    }

    if ( nSubItem != -1 )
    {
        column.mask |= LVCF_SUBITEM;
        column.iSubItem = nSubItem;
    }

    return (int)
        SendMessage ( hList, LVM_INSERTCOLUMN, nCol, ( LPARAM )&column );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVInsertItem 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hList             : listview item
//    Param.    2: int nItem              : item index
//    Param.    3: int nImgIndex          : img. list index
//    Param.    4: const WCHAR * lpszItem : item caption
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: insert an item into a LV control
/*--------------------------------------------------------------------@@-@@-*/
int LVInsertItem ( HWND hList, int nItem, 
    int nImgIndex, const WCHAR * lpszItem )
/*--------------------------------------------------------------------------*/
{
    LVITEMW  item;
    
    item.mask       = LVIF_TEXT;

    if ( nImgIndex != -1 )
        item.mask |= LVIF_IMAGE;

    item.iItem      = nItem;
    item.iSubItem   = 0;
    item.pszText    = (WCHAR *)lpszItem;
    item.state      = 0;
    item.stateMask  = 0;
    item.iImage     = nImgIndex;
    item.lParam     = 0;

    return (int)
        SendMessage ( hList, LVM_INSERTITEM, 0, ( LPARAM )&item );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVInsertItemEx 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hList             : listview item
//    Param.    2: int nItem              : item index
//    Param.    3: int nImgIndex          : img. list index
//    Param.    4: const WCHAR * lpszItem : item caption
//    Param.    5: LPARAM lParam          : custom data
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: insert an item into a LV control
/*--------------------------------------------------------------------@@-@@-*/
int LVInsertItemEx ( HWND hList, int nItem, 
    int nImgIndex, const WCHAR * lpszItem, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    LVITEMW  item;
    
    item.mask       = LVIF_TEXT | LVIF_PARAM;

    if ( nImgIndex != -1 )
        item.mask |= LVIF_IMAGE;

    item.iItem      = nItem;
    item.iSubItem   = 0;
    item.pszText    = (WCHAR *)lpszItem;
    item.state      = 0;
    item.stateMask  = 0;
    item.iImage     = nImgIndex;
    item.lParam     = lParam;

    return (int)
        SendMessage ( hList, LVM_INSERTITEM, 0, ( LPARAM )&item );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVSortItems 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hList    : listview control
//    Param.    2: WPARAM wParam : custom data to pass to sorting function
//    Param.    3: LPARAM lParam : sorting function
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: tells the list to sort its items
/*--------------------------------------------------------------------@@-@@-*/
BOOL LVSortItems ( HWND hList, WPARAM wParam, LPARAM lParam )
/*--------------------------------------------------------------------------*/
{
    return (BOOL)
        SendMessage ( hList, LVM_SORTITEMS, wParam, lParam );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVSetItemText 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hList             : listview control
//    Param.    2: int nItem              : item index
//    Param.    3: int nSubItem           : subitem index
//    Param.    4: const WCHAR * lpszText : item text
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: set item text
/*--------------------------------------------------------------------@@-@@-*/
BOOL LVSetItemText ( HWND hList, int nItem, 
    int nSubItem, const WCHAR * lpszText )
/*--------------------------------------------------------------------------*/
{
    LVITEMW  item;

    item.iSubItem   = nSubItem;
    item.pszText    = (WCHAR *)lpszText;

    return (BOOL)
        SendMessage ( hList, LVM_SETITEMTEXT, nItem, ( LPARAM )&item );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVGetItemText 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hList      : listview control
//    Param.    2: int nItem       : item index
//    Param.    3: int nSubItem    : subitem index
//    Param.    4: WCHAR * lpszText: buffer to receive text
//    Param.    5: int size        : buffer size, in chars
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: get the text from a LV item
/*--------------------------------------------------------------------@@-@@-*/
BOOL LVGetItemText ( HWND hList, int nItem, 
    int nSubItem, WCHAR * lpszText, int cchMax )
/*--------------------------------------------------------------------------*/
{
    LVITEMW  item;

    item.iSubItem   = nSubItem;
    item.cchTextMax = cchMax;
    item.pszText    = lpszText;

    return (BOOL)
        SendMessage(hList, LVM_GETITEMTEXT, nItem, ( LPARAM )&item);
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVClear 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hList : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: clear all items
/*--------------------------------------------------------------------@@-@@-*/
BOOL LVClear ( HWND hList )
/*--------------------------------------------------------------------------*/
{
    return (BOOL)SendMessage ( hList, LVM_DELETEALLITEMS, 0, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVGetCount 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hList : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: item count in a LV control
/*--------------------------------------------------------------------@@-@@-*/
int LVGetCount ( HWND hList )
/*--------------------------------------------------------------------------*/
{
    return (int)SendMessage ( hList, LVM_GETITEMCOUNT, 0, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVGetSelIndex 
/*--------------------------------------------------------------------------*/
//           Type: int 
//    Param.    1: HWND hList : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: which item is selected
/*--------------------------------------------------------------------@@-@@-*/
int LVGetSelIndex ( HWND hList )
/*--------------------------------------------------------------------------*/
{
    int         i, count;
    INT_PTR     state;

    count = LVGetCount ( hList );
    
    for ( i = 0; i < count; i++ )
    {
        state = SendMessage ( hList, LVM_GETITEMSTATE, i, LVIS_SELECTED );

        if ( state & LVIS_SELECTED )
            return i;
    }

    return -1;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVDeleteItem 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hList: 
//    Param.    2: int item  : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: remove an item
/*--------------------------------------------------------------------@@-@@-*/
BOOL LVDeleteItem ( HWND hList, int item )
/*--------------------------------------------------------------------------*/
{
    return ( BOOL )SendMessage ( hList, LVM_DELETEITEM, item, 0 );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVDeleteSelection 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hList : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: remove a bunch of selected items
/*--------------------------------------------------------------------@@-@@-*/
void LVDeleteSelection ( HWND hList )
/*--------------------------------------------------------------------------*/
{
    int         i, count;
    INT_PTR     state;

    i           = 0;
    count       = LVGetCount ( hList );

    do
    {
        state   = SendMessage ( hList, LVM_GETITEMSTATE, i, LVIS_SELECTED );

        if ( state & LVIS_SELECTED )
        {
            LVDeleteItem ( hList, i );
            count--;
        }
        else
            i++;

    } 
    while ( i < count );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVSelectAll 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hList : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: select all items
/*--------------------------------------------------------------------@@-@@-*/
void LVSelectAll ( HWND hList )
/*--------------------------------------------------------------------------*/
{
    LVITEM  item;

    item.stateMask  = LVIS_SELECTED;
    item.state      = LVIS_SELECTED;

    SendMessage ( hList, LVM_SETITEMSTATE, (WPARAM)-1, ( LPARAM )&item );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVSelectItem 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hList: 
//    Param.    2: int index : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: select one
/*--------------------------------------------------------------------@@-@@-*/
void LVSelectItem ( HWND hList, int index )
/*--------------------------------------------------------------------------*/
{
    LVITEM  item;

    item.stateMask  = LVIS_SELECTED | LVIS_FOCUSED;
    item.state      = LVIS_SELECTED | LVIS_FOCUSED;

    SendMessage ( hList, LVM_SETITEMSTATE, index, ( LPARAM )&item );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVSetHeaderSortImg 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hList       : listview hwnd
//    Param.    2: int index        : column header index
//    Param.    3: LV_ARROW lvArrow : up, down or none
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 10.09.2022
//    DESCRIPTION: sets the up/down or no arrow on column sort headers
/*--------------------------------------------------------------------@@-@@-*/
BOOL LVSetHeaderSortImg ( HWND hList, int index, LV_ARROW lvArrow )
/*--------------------------------------------------------------------------*/
{
    HWND    hHeader;
    HDITEM  hdrItem;

    hHeader = ListView_GetHeader ( hList );

    if ( hHeader )
    {
        hdrItem.mask = HDI_FORMAT;

        if ( Header_GetItem ( hHeader, index, &hdrItem ) )
        {
            switch ( lvArrow )
            {
                case UP_ARROW:
                    hdrItem.fmt = ( hdrItem.fmt & ~HDF_SORTDOWN ) | HDF_SORTUP;
                    break;

                case DOWN_ARROW:
                    hdrItem.fmt = ( hdrItem.fmt & ~HDF_SORTUP ) | HDF_SORTDOWN;
                    break;

                case NO_ARROW:
                    hdrItem.fmt = hdrItem.fmt & ~( HDF_SORTDOWN | HDF_SORTUP );
                    break;

                default:
                    break;
            }

            return Header_SetItem ( hHeader, index, &hdrItem );
        }
    }

    return FALSE;
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVFocusItem 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hList: 
//    Param.    2: int index : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: focus item
/*--------------------------------------------------------------------@@-@@-*/
void LVFocusItem ( HWND hList, int index )
/*--------------------------------------------------------------------------*/
{
    LVITEM  item;

    item.stateMask  = LVIS_FOCUSED;
    item.state      = LVIS_FOCUSED;

    SendMessage ( hList, LVM_SETITEMSTATE, index, ( LPARAM )&item );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVUnselectItem 
/*--------------------------------------------------------------------------*/
//           Type: void 
//    Param.    1: HWND hList: 
//    Param.    2: int index : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: unselect one
/*--------------------------------------------------------------------@@-@@-*/
void LVUnselectItem ( HWND hList, int index )
/*--------------------------------------------------------------------------*/
{
    LVITEM  item;

    item.stateMask  =  LVIS_SELECTED;
    item.state      &= ~LVIS_SELECTED;

    SendMessage ( hList, LVM_SETITEMSTATE, index, ( LPARAM )&item );
}

/*-@@+@@--------------------------------------------------------------------*/
//       Function: LVEnsureVisible 
/*--------------------------------------------------------------------------*/
//           Type: BOOL 
//    Param.    1: HWND hList: 
//    Param.    2: int index : 
/*--------------------------------------------------------------------------*/
//         AUTHOR: Adrian Petrila, YO3GFH
//           DATE: 03.10.2020
//    DESCRIPTION: scroll a item into view
/*--------------------------------------------------------------------@@-@@-*/
BOOL LVEnsureVisible ( HWND hList, int index )
/*--------------------------------------------------------------------------*/
{
    return (BOOL)SendMessage ( hList, LVM_ENSUREVISIBLE, index, (LPARAM)FALSE);
}



