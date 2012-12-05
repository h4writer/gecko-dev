/* -*- Mode: C++; tab-width: 2; indent-tabs-mode: nil; c-basic-offset: 2 -*- */
/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/. */

#include "nsHttpHandler.h"
#include "nsHttpChannel.h"
#include "nsHttpAuthManager.h"
#include "nsReadableUtils.h"
#include "nsNetUtil.h"
#include "nsIPrincipal.h"

NS_IMPL_ISUPPORTS1(nsHttpAuthManager, nsIHttpAuthManager)

nsHttpAuthManager::nsHttpAuthManager()
{
}

nsresult nsHttpAuthManager::Init()
{
  // get reference to the auth cache.  we assume that we will live
  // as long as gHttpHandler.  instantiate it if necessary.

  if (!gHttpHandler) {
    nsresult rv;
    nsCOMPtr<nsIIOService> ios = do_GetIOService(&rv);
    if (NS_FAILED(rv))
      return rv;

    nsCOMPtr<nsIProtocolHandler> handler;
    rv = ios->GetProtocolHandler("http", getter_AddRefs(handler));
    if (NS_FAILED(rv))
      return rv;

    // maybe someone is overriding our HTTP handler implementation?
    NS_ENSURE_TRUE(gHttpHandler, NS_ERROR_UNEXPECTED);
  }
	
  mAuthCache = gHttpHandler->AuthCache();
  NS_ENSURE_TRUE(mAuthCache, NS_ERROR_FAILURE);
  return NS_OK;
}

nsHttpAuthManager::~nsHttpAuthManager()
{
}

NS_IMETHODIMP
nsHttpAuthManager::GetAuthIdentity(const nsACString & aScheme,
                                   const nsACString & aHost,
                                   int32_t aPort,
                                   const nsACString & aAuthType,
                                   const nsACString & aRealm,
                                   const nsACString & aPath,
                                   nsAString & aUserDomain,
                                   nsAString & aUserName,
                                   nsAString & aUserPassword,
                                   nsIPrincipal* aPrincipal)
{
  nsHttpAuthEntry * entry = nullptr;
  nsresult rv;
  uint32_t appId = NECKO_NO_APP_ID;
  bool inBrowserElement = false;
  if (aPrincipal) {
    appId = aPrincipal->GetAppId();
    inBrowserElement = aPrincipal->GetIsInBrowserElement();
  }

  if (!aPath.IsEmpty())
    rv = mAuthCache->GetAuthEntryForPath(PromiseFlatCString(aScheme).get(),
                                         PromiseFlatCString(aHost).get(),
                                         aPort,
                                         PromiseFlatCString(aPath).get(),
                                         appId, inBrowserElement,
                                         &entry);
  else
    rv = mAuthCache->GetAuthEntryForDomain(PromiseFlatCString(aScheme).get(),
                                           PromiseFlatCString(aHost).get(),
                                           aPort,
                                           PromiseFlatCString(aRealm).get(),
                                           appId, inBrowserElement,
                                           &entry);

  if (NS_FAILED(rv))
    return rv;
  if (!entry)
    return NS_ERROR_UNEXPECTED;

  aUserDomain.Assign(entry->Domain());
  aUserName.Assign(entry->User());
  aUserPassword.Assign(entry->Pass());
  return NS_OK;
}

NS_IMETHODIMP
nsHttpAuthManager::SetAuthIdentity(const nsACString & aScheme,
                                   const nsACString & aHost,
                                   int32_t aPort,
                                   const nsACString & aAuthType,
                                   const nsACString & aRealm,
                                   const nsACString & aPath,
                                   const nsAString & aUserDomain,
                                   const nsAString & aUserName,
                                   const nsAString & aUserPassword,
                                   nsIPrincipal* aPrincipal)
{
  nsHttpAuthIdentity ident(PromiseFlatString(aUserDomain).get(),
                           PromiseFlatString(aUserName).get(),
                           PromiseFlatString(aUserPassword).get());

  uint32_t appId = NECKO_NO_APP_ID;
  bool inBrowserElement = false;
  if (aPrincipal) {
    appId = aPrincipal->GetAppId();
    inBrowserElement = aPrincipal->GetIsInBrowserElement();
  }

  return mAuthCache->SetAuthEntry(PromiseFlatCString(aScheme).get(),
                                  PromiseFlatCString(aHost).get(),
                                  aPort,
                                  PromiseFlatCString(aPath).get(),
                                  PromiseFlatCString(aRealm).get(),
                                  nullptr,  // credentials
                                  nullptr,  // challenge
                                  appId, inBrowserElement,
                                  &ident,
                                  nullptr); // metadata
}

NS_IMETHODIMP
nsHttpAuthManager::ClearAll()
{
  return mAuthCache->ClearAll();
}
