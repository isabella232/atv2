/*
 *      Copyright (C) 2005-2009 Team XBMC
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

#include "Addon.h"
#include "AddonManager.h"
#include "Settings.h"
#include "GUISettings.h"
#include "StringUtils.h"
#include "FileSystem/Directory.h"
#include "FileSystem/File.h"
#ifdef __APPLE__
#include "../osx/OSXGNUReplacements.h"
#endif
#include "log.h"
#include <vector>
#include <string.h>

using XFILE::CDirectory;
using XFILE::CFile;
using namespace std;

namespace ADDON
{

// BACKWARDCOMPATIBILITY: These can be removed post-Dharma
typedef struct
{
  const char* old;
  const char* id;
} ScraperUpdate;

static const ScraperUpdate music[] =
    {{"allmusic_merlin_lastfm.xml", "metadata.merlin.pl"},
    {"daum.xml",                   "metadata.music.daum.net"},
    {"israel-music.xml",           "metadata.he.israel-music.co.il"},
    {"freebase.xml",               "metadata.freebase.com"},
    {"1ting.xml",                  "metadata.1ting.com"},
    {"allmusic.xml",               "metadata.allmusic.com"},
    {"lastfm.xml",                 "metadata.last.fm"}};

static const ScraperUpdate videos[] =
   {{"7176.xml",                   "metadata.7176.com"},
    {"amazonuk.xml",               "metadata.amazon.co.uk"},
    {"amazonus.xml",               "metadata.amazon.com"},
    {"asiandb.xml",                "metadata.asiandb.com"},
    {"cine-passion.xml",           "metadata.cine-passion.fr"},
    {"cinefacts.xml",              "metadata.cinefacts.de"},
    {"daum-tv.xml",                "metadata.tv.daum.net"},
    {"daum.xml",                   "metadata.movie.daum.net"},
    {"fdbpl.xml",                  "metadata.fdb.pl"},
    {"filmaffinity.xml",           "metadata.filmaffinity.com"},
    {"filmbasen.xml",              "metadata.filmbasen.dagbladet.no"},
    {"filmdelta.xml",              "metadata.filmdelta.se"},
    {"filmstarts.xml",             "metadata.filmstarts.de"},
    {"filmweb.xml",                "metadata.filmweb.pl"},
    {"getlib.xml",                 "metadata.getlib.com"},
    {"imdb.xml",                   "metadata.imdb.com"},
    {"kino-de.xml",                "metadata.kino.de"},
    {"KinoPoisk.xml",              "metadata.kinopoisk.ru"},
    {"M1905.xml",                  "metadata.m1905.com"},
    {"moviemaze.xml",              "metadata.moviemaze.de"},
    {"moviemeter.xml",             "metadata.moviemeter.nl"},
    {"movieplayer-it-film.xml",    "metadata.movieplayer.it"},
    {"movieplayer-it-tv.xml",      "metadata.tv.movieplayer.it"},
    {"mtime.xml",                  "metadata.mtime.com"},
    {"mtv.xml",                    "metadata.mtv.com"},
    {"myMovies.xml",               "metadata.mymovies.it"},
    {"mymoviesdk.xml",             "metadata.mymovies.dk"},
    {"naver.xml",                  "metadata.movie.naver.com"},
    {"ofdb.xml",                   "metadata.ofdb.de"},
    {"ptgate.xml",                 "metadata.ptgate.pt"},
    {"rottentomatoes.xml",         "metadata.rottentomatoes.com"},
    {"sratim.xml",                 "metadata.sratim.co.il"},
    {"tmdb.xml",                   "metadata.themoviedb.org"},
    {"tvdb.xml",                   "metadata.tvdb.com"},
    {"videobuster.xml",            "metadata.videobuster.de"},
    {"worldart.xml",               "metadata.worldart.ru"},
    {"yahoomusic.xml",             "metadata.yahoomusic.com"}};

const CStdString UpdateVideoScraper(const CStdString &old)
{
  for (unsigned int index=0; index < sizeof(videos)/sizeof(videos[0]); ++index)
  {
    if (old == videos[index].old)
      return videos[index].id;
  }
  return "";
}

const CStdString UpdateMusicScraper(const CStdString &old)
{
  for (unsigned int index=0; index < sizeof(music)/sizeof(music[0]); ++index)
  {
    if (old == music[index].old)
      return music[index].id;
  }
  return "";
}

/**
 * helper functions 
 *
 */

typedef struct
{
  const char* name;
  TYPE        type;
  int         pretty;
  const char* icon;
} TypeMapping;

static const TypeMapping types[] =
  {{"unknown",                           ADDON_UNKNOWN,                 0, "" },
   {"xbmc.metadata.scraper.albums",      ADDON_SCRAPER_ALBUMS,      24016, "DefaultAddonAlbumInfo.png" },
   {"xbmc.metadata.scraper.artists",     ADDON_SCRAPER_ARTISTS,     24017, "DefaultAddonArtistInfo.png" },
   {"xbmc.metadata.scraper.movies",      ADDON_SCRAPER_MOVIES,      24007, "DefaultAddonMovieInfo.png" },
   {"xbmc.metadata.scraper.musicvideos", ADDON_SCRAPER_MUSICVIDEOS, 24015, "DefaultAddonMusicVideoInfo.png" },
   {"xbmc.metadata.scraper.tvshows",     ADDON_SCRAPER_TVSHOWS,     24014, "DefaultAddonTvInfo.png" },
   {"xbmc.metadata.scraper.library",     ADDON_SCRAPER_LIBRARY,         0, "" },
   {"xbmc.ui.screensaver",               ADDON_SCREENSAVER,         24008, "DefaultAddonScreensaver.png" },
   {"xbmc.player.musicviz",              ADDON_VIZ,                 24010, "DefaultAddonVisualization.png" },
   {"visualization-library",             ADDON_VIZ_LIBRARY,             0, "" },
   {"xbmc.python.pluginsource",          ADDON_PLUGIN,              24005, "" },
   {"xbmc.python.script",                ADDON_SCRIPT,              24009, "" },
   {"xbmc.python.weather",               ADDON_SCRIPT_WEATHER,      24027, "DefaultAddonWeather.png" },
   {"xbmc.python.subtitles",             ADDON_SCRIPT_SUBTITLES,    24012, "DefaultAddonSubtitles.png" },
   {"xbmc.python.lyrics",                ADDON_SCRIPT_LYRICS,       24013, "DefaultAddonLyrics.png" },
   {"xbmc.python.library",               ADDON_SCRIPT_LIBRARY,      24014, "" },
   {"xbmc.python.module",                ADDON_SCRIPT_MODULE,           0, "" },
   {"xbmc.gui.skin",                     ADDON_SKIN,                  166, "DefaultAddonSkin.png" },
   {"xbmc.gui.webinterface",             ADDON_WEB_INTERFACE,         199, "DefaultAddonWebSkin.png" },
   {"xbmc.addon.repository",             ADDON_REPOSITORY,          24011, "DefaultAddonRepository.png" },
   {"pvrclient",                         ADDON_PVRDLL,                  0, "" },
   {"xbmc.addon.video",                  ADDON_VIDEO,                1037, "DefaultAddonVideo.png" },
   {"xbmc.addon.audio",                  ADDON_AUDIO,                1038, "DefaultAddonMusic.png" },
   {"xbmc.addon.image",                  ADDON_IMAGE,                1039, "DefaultAddonPicture.png" },
   {"xbmc.addon.executable",             ADDON_EXECUTABLE,           1043, "DefaultAddonProgram.png" }};

const CStdString TranslateType(const ADDON::TYPE &type, bool pretty/*=false*/)
{
  for (unsigned int index=0; index < sizeof(types)/sizeof(types[0]); ++index)
  {
    const TypeMapping &map = types[index];
    if (type == map.type)
    {
      if (pretty && map.pretty)
        return g_localizeStrings.Get(map.pretty);
      else
        return map.name;
    }
  }
  return "";
}

const TYPE TranslateType(const CStdString &string)
{
  for (unsigned int index=0; index < sizeof(types)/sizeof(types[0]); ++index)
  {
    const TypeMapping &map = types[index];
    if (string.Equals(map.name))
      return map.type;
  }
  return ADDON_UNKNOWN;
}

const CStdString GetIcon(const ADDON::TYPE& type)
{
  for (unsigned int index=0; index < sizeof(types)/sizeof(types[0]); ++index)
  {
    const TypeMapping &map = types[index];
    if (type == map.type)
      return map.icon;
  }
  return "";
}

/**
 * AddonVersion
 *
 */

bool AddonVersion::operator==(const AddonVersion &rhs) const
{
  return str.Equals(rhs.str);
}

bool AddonVersion::operator!=(const AddonVersion &rhs) const
{
  return !(*this == rhs);
}

bool AddonVersion::operator>(const AddonVersion &rhs) const
{
  return (strverscmp(str.c_str(), rhs.str.c_str()) > 0);
}

bool AddonVersion::operator>=(const AddonVersion &rhs) const
{
  return (*this == rhs) || (*this > rhs);
}

bool AddonVersion::operator<(const AddonVersion &rhs) const
{
  return (strverscmp(str.c_str(), rhs.str.c_str()) < 0);
}

bool AddonVersion::operator<=(const AddonVersion &rhs) const
{
  return (*this == rhs) || !(*this > rhs);
}

CStdString AddonVersion::Print() const
{
  CStdString out;
  out.Format("%s %s", g_localizeStrings.Get(24051), str); // "Version <str>"
  return CStdString(out);
}

#define EMPTY_IF(x,y) \
  { \
    CStdString fan=CAddonMgr::Get().GetExtValue(metadata->configuration, x); \
    if (fan.Equals("true")) \
      y.Empty(); \
  }

AddonProps::AddonProps(cp_plugin_info_t *props)
  : id(props->identifier)
  , version(props->version)
  , name(props->name)
  , path(props->plugin_path)
  , author(props->provider_name)
  , stars(0)
{
  //FIXME only considers the first registered extension for each addon
  if (props->extensions->ext_point_id)
    type = TranslateType(props->extensions->ext_point_id);

  icon = "icon.png";
  fanart = CUtil::AddFileToFolder(path, "fanart.jpg");
  changelog = CUtil::AddFileToFolder(path, "changelog.txt");
  // Grab more detail from the props...
  const cp_extension_t *metadata = CAddonMgr::Get().GetExtension(props, "xbmc.addon.metadata");
  if (metadata)
  {
    summary = CAddonMgr::Get().GetTranslatedString(metadata->configuration, "summary");
    description = CAddonMgr::Get().GetTranslatedString(metadata->configuration, "description");
    disclaimer = CAddonMgr::Get().GetTranslatedString(metadata->configuration, "disclaimer");
    license = CAddonMgr::Get().GetExtValue(metadata->configuration, "license");
    broken = CAddonMgr::Get().GetExtValue(metadata->configuration, "broken");
    EMPTY_IF("nofanart",fanart)
    EMPTY_IF("noicon",icon)
    EMPTY_IF("nochangelog",changelog)
  }
}

/**
 * CAddon
 *
 */

CAddon::CAddon(const cp_extension_t *ext)
  : m_props(ext ? ext->plugin : NULL)
  , m_parent(AddonPtr())
{
  BuildLibName(ext);
  BuildProfilePath();
  CUtil::AddFileToFolder(Profile(), "settings.xml", m_userSettingsPath);
  m_enabled = true;
  m_hasStrings = false;
  m_checkedStrings = false;
  m_settingsLoaded = false;
  m_userSettingsLoaded = false;
}

CAddon::CAddon(const AddonProps &props)
  : m_props(props)
  , m_parent(AddonPtr())
{
  if (props.libname.IsEmpty()) BuildLibName();
  else m_strLibName = props.libname;
  BuildProfilePath();
  CUtil::AddFileToFolder(Profile(), "settings.xml", m_userSettingsPath);
  m_enabled = true;
  m_hasStrings = false;
  m_checkedStrings = false;
  m_settingsLoaded = false;
  m_userSettingsLoaded = false;
}

CAddon::CAddon(const CAddon &rhs, const AddonPtr &parent)
  : m_props(rhs.Props())
  , m_parent(parent)
{
  m_settings  = rhs.m_settings;
  m_addonXmlDoc = rhs.m_addonXmlDoc;
  m_settingsLoaded = rhs.m_settingsLoaded;
  m_userSettingsLoaded = rhs.m_userSettingsLoaded;
  BuildProfilePath();
  CUtil::AddFileToFolder(Profile(), "settings.xml", m_userSettingsPath);
  m_strLibName  = rhs.m_strLibName;
  m_enabled = rhs.Enabled();
  m_hasStrings  = false;
  m_checkedStrings  = false;
}

AddonPtr CAddon::Clone(const AddonPtr &self) const
{
  return AddonPtr(new CAddon(*this, self));
}

const AddonVersion CAddon::Version()
{
  return m_props.version;
}

//TODO platform/path crap should be negotiated between the addon and
// the handler for it's type
void CAddon::BuildLibName(const cp_extension_t *extension)
{
  if (!extension)
  {
    m_strLibName = "default";
    CStdString ext;
    switch (m_props.type)
    {
    case ADDON_SCRAPER_ALBUMS:
    case ADDON_SCRAPER_ARTISTS:
    case ADDON_SCRAPER_MOVIES:
    case ADDON_SCRAPER_MUSICVIDEOS:
    case ADDON_SCRAPER_TVSHOWS:
    case ADDON_SCRAPER_LIBRARY:
      ext = ADDON_SCRAPER_EXT;
      break;
    case ADDON_SCREENSAVER:
      ext = ADDON_SCREENSAVER_EXT;
      break;
    case ADDON_SKIN:
      m_strLibName = "skin.xml";
      return;
    case ADDON_VIZ:
      ext = ADDON_VIS_EXT;
      break;
    case ADDON_SCRIPT:
    case ADDON_SCRIPT_LIBRARY:
    case ADDON_SCRIPT_LYRICS:
    case ADDON_SCRIPT_WEATHER:
    case ADDON_SCRIPT_SUBTITLES:
    case ADDON_PLUGIN:
      ext = ADDON_PYTHON_EXT;
      break;
    default:
      m_strLibName.clear();
      return;
    }
    // extensions are returned as *.ext
    // so remove the asterisk
    ext.erase(0,1);
    m_strLibName.append(ext);
  }
  else
  {
    switch (m_props.type)
    {
      case ADDON_SCREENSAVER:
      case ADDON_SCRIPT:
      case ADDON_SCRIPT_LIBRARY:
      case ADDON_SCRIPT_LYRICS:
      case ADDON_SCRIPT_WEATHER:
      case ADDON_SCRIPT_SUBTITLES:
      case ADDON_SCRIPT_MODULE:
      case ADDON_SCRAPER_ALBUMS:
      case ADDON_SCRAPER_ARTISTS:
      case ADDON_SCRAPER_MOVIES:
      case ADDON_SCRAPER_MUSICVIDEOS:
      case ADDON_SCRAPER_TVSHOWS:
      case ADDON_SCRAPER_LIBRARY:
      case ADDON_PLUGIN:
        {
          CStdString temp = CAddonMgr::Get().GetExtValue(extension->configuration, "@library");
          m_strLibName = temp;
        }
        break;
      default:
        m_strLibName.clear();
        break;
    }
  }
}

/**
 * Language File Handling
 */
bool CAddon::LoadStrings()
{
  // Path where the language strings reside
  CStdString chosenPath;
  chosenPath.Format("resources/language/%s/strings.xml", g_guiSettings.GetString("locale.language").c_str());
  CStdString chosen = CUtil::AddFileToFolder(m_props.path, chosenPath);
  CStdString fallback = CUtil::AddFileToFolder(m_props.path, "resources/language/English/strings.xml");

  m_hasStrings = m_strings.Load(chosen, fallback);
  return m_checkedStrings = true;
}

void CAddon::ClearStrings()
{
  // Unload temporary language strings
  m_strings.Clear();
  m_hasStrings = false;
}

CStdString CAddon::GetString(uint32_t id)
{
  if (!m_hasStrings && ! m_checkedStrings && !LoadStrings())
     return "";

  return m_strings.Get(id);
}

/**
 * Settings Handling
 */
bool CAddon::HasSettings()
{
  return LoadSettings();
}

bool CAddon::LoadSettings()
{
  if (m_settingsLoaded)
    return true;

  CStdString addonFileName = CUtil::AddFileToFolder(m_props.path, "resources/settings.xml");

  if (!m_addonXmlDoc.LoadFile(addonFileName))
  {
    if (CFile::Exists(addonFileName))
      CLog::Log(LOGERROR, "Unable to load: %s, Line %d\n%s", addonFileName.c_str(), m_addonXmlDoc.ErrorRow(), m_addonXmlDoc.ErrorDesc());
    return false;
  }

  // Make sure that the addon XML has the settings element
  TiXmlElement *setting = m_addonXmlDoc.RootElement();
  if (!setting || strcmpi(setting->Value(), "settings") != 0)
  {
    CLog::Log(LOGERROR, "Error loading Settings %s: cannot find root element 'settings'", addonFileName.c_str());
    return false;
  }
  SettingsFromXML(m_addonXmlDoc, true);
  LoadUserSettings();
  m_settingsLoaded = true;
  return true;
}

bool CAddon::HasUserSettings()
{
  if (!LoadSettings())
    return false;

  return m_userSettingsLoaded;
}

bool CAddon::LoadUserSettings()
{
  m_userSettingsLoaded = false;
  TiXmlDocument doc;
  if (doc.LoadFile(m_userSettingsPath))
    m_userSettingsLoaded = SettingsFromXML(doc);
  return m_userSettingsLoaded;
}

void CAddon::SaveSettings(void)
{
  if (!m_settings.size())
    return; // no settings to save

  // break down the path into directories
  CStdString strRoot, strAddon;
  CUtil::GetDirectory(m_userSettingsPath, strAddon);
  CUtil::RemoveSlashAtEnd(strAddon);
  CUtil::GetDirectory(strAddon, strRoot);
  CUtil::RemoveSlashAtEnd(strRoot);

  // create the individual folders
  if (!CDirectory::Exists(strRoot))
    CDirectory::Create(strRoot);
  if (!CDirectory::Exists(strAddon))
    CDirectory::Create(strAddon);

  // create the XML file
  TiXmlDocument doc;
  SettingsToXML(doc);
  doc.SaveFile(m_userSettingsPath);
}

CStdString CAddon::GetSetting(const CStdString& key)
{
  if (!LoadSettings())
    return ""; // no settings available

  map<CStdString, CStdString>::const_iterator i = m_settings.find(key);
  if (i != m_settings.end())
    return i->second;
  return "";
}

void CAddon::UpdateSetting(const CStdString& key, const CStdString& value)
{
  LoadSettings();
  if (key.empty()) return;
  m_settings[key] = value;
}

bool CAddon::SettingsFromXML(const TiXmlDocument &doc, bool loadDefaults /*=false */)
{
  if (!doc.RootElement())
    return false;

  if (loadDefaults)
    m_settings.clear();

  const TiXmlElement* category = doc.RootElement()->FirstChildElement("category");
  if (!category)
    category = doc.RootElement();

  bool foundSetting = false;
  while (category)
  {
    const TiXmlElement *setting = category->FirstChildElement("setting");
    while (setting)
    {
      const char *id = setting->Attribute("id");
      const char *value = setting->Attribute(loadDefaults ? "default" : "value");
      if (id && value)
      {
        m_settings[id] = value;
        foundSetting = true;
      }
      setting = setting->NextSiblingElement("setting");
    }
    category = category->NextSiblingElement("category");
  }
  return foundSetting;
}

void CAddon::SettingsToXML(TiXmlDocument &doc) const
{
  TiXmlElement node("settings");
  doc.InsertEndChild(node);
  for (map<CStdString, CStdString>::const_iterator i = m_settings.begin(); i != m_settings.end(); ++i)
  {
    TiXmlElement nodeSetting("setting");
    nodeSetting.SetAttribute("id", i->first.c_str());
    nodeSetting.SetAttribute("value", i->second.c_str());
    doc.RootElement()->InsertEndChild(nodeSetting);
  }
  doc.SaveFile(m_userSettingsPath);
}

TiXmlElement* CAddon::GetSettingsXML()
{
  return m_addonXmlDoc.RootElement();
}

void CAddon::BuildProfilePath()
{
  m_profile.Format("special://profile/addon_data/%s/", ID().c_str());
}

const CStdString CAddon::Icon() const
{
  if (CURL::IsFullPath(m_props.icon))
    return m_props.icon;
  return CUtil::AddFileToFolder(m_props.path, m_props.icon);
}

const CStdString CAddon::LibPath() const
{
  return CUtil::AddFileToFolder(m_props.path, m_strLibName);
}

ADDONDEPS CAddon::GetDeps()
{
  return CAddonMgr::Get().GetDeps(ID());
}

/**
 * CAddonLibrary
 *
 */

CAddonLibrary::CAddonLibrary(const cp_extension_t *ext)
  : CAddon(ext)
  , m_addonType(SetAddonType())
{
}

CAddonLibrary::CAddonLibrary(const AddonProps& props)
  : CAddon(props)
  , m_addonType(SetAddonType())
{
}

TYPE CAddonLibrary::SetAddonType()
{
  if (Type() == ADDON_VIZ_LIBRARY)
    return ADDON_VIZ;
  else
    return ADDON_UNKNOWN;
}

} /* namespace ADDON */

