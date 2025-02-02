#pragma once
/*
 *      Copyright (C) 2005-2008 Team XBMC
 *      http://www.xbmc.org
 *
 *  This Program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2, or (at your option)
 *  any later version.
 *
 *  This Program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with XBMC; see the file COPYING.  If not, write to
 *  the Free Software Foundation, 675 Mass Ave, Cambridge, MA 02139, USA.
 *  http://www.gnu.org/copyleft/gpl.html
 *
 */

#include <memory>
#include <map>

#include <CoreFoundation/CoreFoundation.h>
#include "ZeroconfBrowser.h"

#include <Thread.h>
#include <CriticalSection.h>

#if !defined(__arm__)
#if 0
  // An opaque reference representing a CFNetService.
  typedef struct __CFNetService* CFNetServiceRef;
  // An opaque reference representing a CFNetServiceBrowser.
  typedef struct __CFNetServiceBrowser* CFNetServiceBrowserRef;
#else
  #include <Carbon/Carbon.h>
  #include <CoreServices/CoreServices.h>
#endif
#else
  #include <CFNetwork/CFNetServices.h>
#endif


//platform specific implementation of  zeroconfbrowser interface using native os x APIs
class CZeroconfBrowserOSX : public CZeroconfBrowser
{
public:
  CZeroconfBrowserOSX();
  ~CZeroconfBrowserOSX();

private:
  ///implementation if CZeroconfBrowser interface
  ///@{
  virtual bool doAddServiceType(const CStdString& fcr_service_type);
  virtual bool doRemoveServiceType(const CStdString& fcr_service_type);

  virtual std::vector<CZeroconfBrowser::ZeroconfService> doGetFoundServices();
  virtual bool doResolveService(CZeroconfBrowser::ZeroconfService& fr_service, double f_timeout);
  ///@}

  /// browser callback
  static void BrowserCallback(CFNetServiceBrowserRef browser, CFOptionFlags flags, CFTypeRef domainOrService, CFStreamError *error, void *info);
  /// resolve callback
  static void ResolveCallback(CFNetServiceRef theService, CFStreamError* error, void* info);

  /// adds the service to list of found services
  void addDiscoveredService(CFNetServiceBrowserRef browser, CFOptionFlags flags, ZeroconfService const& fcr_service);
  /// removes the service from list of found services
  void removeDiscoveredService(CFNetServiceBrowserRef browser, CFOptionFlags flags, ZeroconfService const& fcr_service);
  
  //CF runloop ref; we're using main-threads runloop
  CFRunLoopRef m_runloop;
  
  //shared variables (with guard)
  //TODO: split the guard for discovered, resolved access?
  CCriticalSection m_data_guard;
  // tBrowserMap maps service types the corresponding browser
  typedef std::map<CStdString, CFNetServiceBrowserRef> tBrowserMap;
  tBrowserMap m_service_browsers;
  //tDiscoveredServicesMap maps browsers to their discovered services + a ref-count for each service
  //ref-count is needed, because a service might pop up more than once, if there's more than one network-iface
  typedef std::map<CFNetServiceBrowserRef, std::vector<std::pair<ZeroconfService, unsigned int> > > tDiscoveredServicesMap;
  tDiscoveredServicesMap m_discovered_services;
};