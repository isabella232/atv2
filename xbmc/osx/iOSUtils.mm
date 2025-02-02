/*
 *      Copyright (C) 2005-2010 Team XBMC
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

#define BOOL XBMC_BOOL 
#include "system.h"
#include "utils/log.h"
#undef BOOL

#if defined(__APPLE__)
  #if defined(__arm__)
    #import <Foundation/Foundation.h>
  #else
    #import <Cocoa/Cocoa.h>
  #endif
#import <mach/mach_host.h>
#import <sys/sysctl.h>

#import "iOSUtils.h"

CFURLRef CreateCFURLRefFromFilePath(const char *filepath)
{
  NSString *fpath = [[NSString alloc]initWithUTF8String:filepath];
  CFURLRef fileURL = (CFURLRef)[[NSURL alloc] initFileURLWithPath: fpath];
  [fpath release];

  return fileURL;
}

int  GetIOSFrameworkPath(char* path, uint32_t *pathsize)
{
  // see if we can figure out who we are
	NSString *pathname;

  // a) XBMC frappliance running under ATV2
  Class XBMCfrapp = NSClassFromString(@"XBMCAppliance");
  if (XBMCfrapp != NULL)
  {
    pathname = [[NSBundle bundleForClass:XBMCfrapp] pathForResource:@"Frameworks" ofType:@""];
    strcpy(path, [pathname UTF8String]);
    *pathsize = strlen(path);
    //NSLog(@"%s XBMC Frapp Frameworks path %s", __PRETTY_FUNCTION__, path);
    return 0;
  }

  // b) XBMC application running under IOS
  Class XBMCapp= NSClassFromString(@"XBMCApplication");
  if (XBMCfrapp != NULL)
  {
    pathname = [[NSBundle bundleForClass:XBMCapp] pathForResource:@"Frameworks" ofType:@""];
    strcpy(path, [pathname UTF8String]);
    *pathsize = strlen(path);
    //NSLog(@"%s XBMC IOS Frameworks path %s", __PRETTY_FUNCTION__, path);
    return 0;
  }

  // c) XBMC application running under OSX
  pathname = [[NSBundle mainBundle] privateFrameworksPath];
  strcpy(path, [pathname UTF8String]);
  *pathsize = strlen(path);
  //NSLog(@"%s XBMC Frameworks path %s", __PRETTY_FUNCTION__, path);

  return 0;
}

int  GetIOSExecutablePath(char* path, uint32_t *pathsize)
{
  // see if we can figure out who we are
	NSString *pathname;

  // a) XBMC frappliance running under ATV2
  Class XBMCfrapp = NSClassFromString(@"XBMCAppliance");
  if (XBMCfrapp != NULL)
  {
    pathname = [[NSBundle bundleForClass:XBMCfrapp] pathForResource:@"XBMC" ofType:@""];
    strcpy(path, [pathname UTF8String]);
    *pathsize = strlen(path);
    //NSLog(@"%s XBMC frapp executable_path %s", __PRETTY_FUNCTION__, path);
    return 0;
  }

  // b) XBMC application running under IOS
  // c) XBMC application running under OSX
  pathname = [[NSBundle mainBundle] executablePath];
  strcpy(path, [pathname UTF8String]);
  *pathsize = strlen(path);
  //NSLog(@"%s XBMC app executable_path %s", __PRETTY_FUNCTION__, path);

  return 0;
}

bool iOS_HasVideoToolboxDecoder(void)
{
  bool bDecoderAvailable = false;

  Class XBMCfrapp = NSClassFromString(@"XBMCAppliance");
  if (XBMCfrapp != NULL)
  {
    // atv2 has seatbelt profile key removed so nothing to do here
    bDecoderAvailable = true;
  }
  else
  {
    /* Get Application directory */
    uint32_t path_size = 2*MAXPATHLEN;
    char     given_path[2*MAXPATHLEN];
    int      result = -1;
    
    memset(given_path, 0x0, path_size);
    result = GetIOSExecutablePath(given_path, &path_size);
    if(result == 0) 
    {
      /* When XBMC is started from a sandbox directory we have to check the sysctl values */
      if(strlen("/var/mobile/Applications/") < path_size &&
         strncmp(given_path, "/var/mobile/Applications/", strlen("/var/mobile/Applications/")) == 0) {

        uint64_t proc_enforce = 0;
        uint64_t vnode_enforce = 0; 
        size_t size = sizeof(vnode_enforce);
        
        sysctlbyname("security.mac.proc_enforce", &proc_enforce, &size, NULL, 0);  
        sysctlbyname("security.mac.vnode_enforce", &vnode_enforce, &size, NULL, 0);
        
        if(vnode_enforce && proc_enforce)
        {
          bDecoderAvailable = false;
          CLog::Log(LOGINFO, "VideoToolBox decoder not available. Use : sysctl -w security.mac.proc_enforce=0; sysctl -w security.mac.vnode_enforce=0\n");
          //NSLog(@"%s VideoToolBox decoder not available. Use : sysctl -w security.mac.proc_enforce=0; sysctl -w security.mac.vnode_enforce=0", __PRETTY_FUNCTION__);
        }
        else
        {
          bDecoderAvailable = true;
          CLog::Log(LOGINFO, "VideoToolBox decoder available\n");
          //NSLog(@"%s VideoToolBox decoder available", __PRETTY_FUNCTION__);
        }  
      }
      else
      {
        bDecoderAvailable = true;
      }
      
      //NSLog(@"%s Executable path %s", __PRETTY_FUNCTION__, given_path);
    }
    else
    {
      /* In theory this case can never happen. But who knows. */
      bDecoderAvailable = true;
    }
  }

  return bDecoderAvailable;
}

#endif
