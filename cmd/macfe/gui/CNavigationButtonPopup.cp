/* -*- Mode: C++; tab-width: 4; indent-tabs-mode: nil; c-basic-offset: 2 -*-
 *
 * The contents of this file are subject to the Netscape Public License
 * Version 1.0 (the "NPL"); you may not use this file except in
 * compliance with the NPL.  You may obtain a copy of the NPL at
 * http://www.mozilla.org/NPL/
 *
 * Software distributed under the NPL is distributed on an "AS IS" basis,
 * WITHOUT WARRANTY OF ANY KIND, either express or implied. See the NPL
 * for the specific language governing rights and limitations under the
 * NPL.
 *
 * The Initial Developer of this code under the NPL is Netscape
 * Communications Corporation.  Portions created by Netscape are
 * Copyright (C) 1998 Netscape Communications Corporation.  All Rights
 * Reserved.
 */

// ===========================================================================
//	CNavigationButtonPopup.cp
//
// ===========================================================================

#include "CNavigationButtonPopup.h"
		
#include "prefapi.h"		
#include "xp_mem.h"
#include "shist.h"
#include "Netscape_Constants.h"
#include "CWindowMediator.h"
#include "CBrowserContext.h"
#include "CBrowserWindow.h"
#include "UMenuUtils.h"
#include "CAutoPtr.h"
#include "macutil.h"
#include "net.h"

#include <string>
		
// ---------------------------------------------------------------------------
//		� CNavigationButtonPopup
// ---------------------------------------------------------------------------

CNavigationButtonPopup::CNavigationButtonPopup(
	LStream* inStream)

	:	mBrowserContext(nil),
		mBrowserWindow(nil),
		mHistory(nil),
		mCurrentEntry(nil),
		mCurrentEntryIndex(0),
		mNumItemsInHistory(0),
	
		super(inStream)
{
}

// ---------------------------------------------------------------------------
//		� ~CNavigationButtonPopup
// ---------------------------------------------------------------------------

CNavigationButtonPopup::~CNavigationButtonPopup()
{
}


//
// FinishCreateSelf
//
void
CNavigationButtonPopup :: FinishCreateSelf ( )
{
	CToolbarBevelButton::FinishCreateSelf();
	
	// LBevelButton will broadcast when an item in the popup menu is picked or when
	// the button is pressed. We want to handle that here instead of elsewhere
	AddListener(this);

} // FinishCreateSelf


#pragma mark -

// ---------------------------------------------------------------------------
//		� AdjustMenuContents
// ---------------------------------------------------------------------------

void
CNavigationButtonPopup::AdjustMenuContents()
{
	if (!GetMenuHandle())
		return;
	
	if (!AssertPreconditions())
		return;
		
	// Purge the menu

	UMenuUtils::PurgeMenuItems(GetMenuHandle());
	
	// Fill the menu
	
	if (GetCommandNumber() == cmd_GoBack)
	{
		for (int insertAfterItem = 0, i = mCurrentEntryIndex - 1; i >= 1; i--, insertAfterItem++)
		{
			InsertHistoryItemIntoMenu(i, insertAfterItem);
		}
	}
	else if (GetCommandNumber() == cmd_GoForward)
	{
		for (int insertAfterItem = 0, i = mCurrentEntryIndex + 1; i <= mNumItemsInHistory; i++, insertAfterItem++)
		{
			InsertHistoryItemIntoMenu(i, insertAfterItem);
		}
	}
	
	// Set the min/max values of the control since we populated the menu
	
//	SetPopupMinMaxValues();
}

// ---------------------------------------------------------------------------
//		� InsertHistoryItemIntoMenu
// ---------------------------------------------------------------------------

void
CNavigationButtonPopup::InsertHistoryItemIntoMenu(
	Int32				inHistoryItemIndex,
	Int16				inAfterItem)
{
	Assert_(GetMenuHandle());
	Assert_(mBrowserContext);

	CAutoPtr<cstring> theTitle = mBrowserContext->GetHistoryEntryTitleByIndex(inHistoryItemIndex);
	
	if (!theTitle.get() || !theTitle->length())
	{
		theTitle.reset(new cstring);
		mBrowserContext->GetHistoryURLByIndex(*theTitle, inHistoryItemIndex);
	}
	
	UMenuUtils::AdjustStringForMenuTitle(*theTitle);
	
	LStr255 thePString(*theTitle);

	// Insert a "blank" item first...
	
	::InsertMenuItem(GetMenuHandle(), "\p ", inAfterItem + 1);
	
	// Then change it. We do this so that no interpretation of metacharacters will occur.
	
	::SetMenuItemText(GetMenuHandle(), inAfterItem + 1, thePString);
}
	
#pragma mark -


//
// ListenToMessage
//
// The message sent will have
//
void
CNavigationButtonPopup :: ListenToMessage ( MessageT inMessage, void* ioParam )
{
	Uint32 menuValue = *reinterpret_cast<Uint32*>(ioParam);
	
	if ( AssertPreconditions() ) {
		if ( menuValue ) {
			Int32 historyIndex = 0;

			if (inMessage == cmd_GoBack)
				historyIndex = SHIST_GetIndex(mHistory, mCurrentEntry) - menuValue;
			else if (inMessage == cmd_GoForward)
				historyIndex = SHIST_GetIndex(mHistory, mCurrentEntry) + menuValue;

			if (historyIndex)
				mBrowserContext->LoadHistoryEntry(historyIndex);
		}
		else {
			if (inMessage == cmd_GoBack)
				mBrowserWindow->SendAEGo(kAEPrevious);
			else if (inMessage == cmd_GoForward)
				mBrowserWindow->SendAEGo(kAENext);		
		}
	}
}

// ---------------------------------------------------------------------------
//		� AssertPreconditions
// ---------------------------------------------------------------------------
//	Assert preconditions and fill in interesting member data

Boolean
CNavigationButtonPopup::AssertPreconditions()
{	
	CMediatedWindow* topWindow = CWindowMediator::GetWindowMediator()->FetchTopWindow(WindowType_Any, regularLayerType);	
	if (!topWindow || topWindow->GetWindowType() != WindowType_Browser)
		return false;
	
	mBrowserWindow = dynamic_cast<CBrowserWindow*>(topWindow);
	if (!mBrowserWindow)
		return false;
	
	if (!(mBrowserContext = (CBrowserContext*)mBrowserWindow->GetWindowContext()))
		return false;

	if (!(mHistory = &((MWContext*)(*mBrowserContext))->hist))
		return false;
	
	if (!(mCurrentEntry = mBrowserContext->GetCurrentHistoryEntry()))
		return false;

	mCurrentEntryIndex = SHIST_GetIndex(mHistory, mCurrentEntry);
	
	mNumItemsInHistory = mBrowserContext->GetHistoryListCount();
	
	return true;
}


//
// ClickSelf
//
// Override to fixup the menus before we show them
//
void
CNavigationButtonPopup :: ClickSelf ( const SMouseDownEvent & inEvent )
{
	AdjustMenuContents();
	super::ClickSelf(inEvent);
}