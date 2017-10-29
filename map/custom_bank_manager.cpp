#include "map/custom_bank_manager.hpp"
#include "map/user_mark.hpp"
#include "map/framework.hpp"
#include "map/user_mark.hpp"
#include "map/bookmark_manager.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/parse_xml.hpp"

#include "indexer/scales.hpp"

#include "base/logging.hpp"

#include "geometry/transformations.hpp"

#include "base/macros.hpp"
#include "base/stl_add.hpp"

#include "std/target_os.hpp"

#include <algorithm>

CustomBankManager::CustomBankManager(Framework & f)
  : m_framework(f)
{
}

CustomBankManager::~CustomBankManager()
{
  ClearCustomBanks();
}

void CustomBankManager::SetBookmarkManager(BookmarkManager * bmManager)
{
  m_bmManager = bmManager;
}

BookmarkManager * CustomBankManager::GetBookmarkManager()
{
  return m_bmManager;
}

void CustomBankManager::ClearCustomBanks()
{
  //m_bmManager->GetUserMarksController(UserMarkType::CUSTOM_BANK_MARK).Clear();
}

void CustomBankManager::LoadCustomBanks()
{
  ClearCustomBanks();

  string const dir = GetPlatform().SettingsDir();

  Platform::FilesList files;
  Platform::GetFilesByExt(dir, CUSTOM_BANKS_FILE_EXTENSION, files);
  for (size_t i = 0; i < files.size(); ++i)
    LoadFromBNK(make_unique<FileReader>(dir + files[i]));

  /*
  std::ostringstream os;
  os << "Custom_Bank_Manager Listed " << files.size();
  LOG(LDEBUG, (os.str()));
  */
}

namespace
{
  std::string const kBank = "Bank";
  std::string const kDocument = "Document";
  std::string const kBankType = "bankType";

  std::string const kDefaultTrackColor = "DefaultTrackColor";
  float const kDefaultTrackWidth = 5.0f;

  char const * bnkSupportedColors[] = {"bank-lion",    "bank-oromiya",    "bank-somali", "bank-wegagen"};

  string FindMatchingStyle(string const & s, string const & fallback)
  {
    if (s.empty())
      return fallback;

    for (size_t i = 0; i < ARRAY_SIZE(bnkSupportedColors); ++i)
    {
      if (s == bnkSupportedColors[i])
        return s;
    }

    LOG(LWARNING, ("Icon", s, "point is not supported"));
    return fallback;
  }

  std::string GetSupportedBNKStyle(std::string const & s)
  {
    ASSERT(!s.empty(), ());
    std::string const result = s.substr(1);
    return FindMatchingStyle(result, bnkSupportedColors[0]);
  }

  std::string PointToString(m2::PointD const & org)
  {
    double const lon = MercatorBounds::XToLon(org.x);
    double const lat = MercatorBounds::YToLat(org.y);

    ostringstream ss;
    ss.precision(8);

    ss << lon << "," << lat;
    return ss.str();
  }

  enum GeometryType
  {
    GEOMETRY_TYPE_UNKNOWN,
    GEOMETRY_TYPE_POINT
  };

  class BNKParser
  {
    BookmarkManager * bmManager;

    std::vector<std::string> m_tags;
    GeometryType m_geometryType;

    std::string m_styleId;
    std::string m_mapStyleId;

    std::string m_name;
    std::string m_type;
    std::string m_description;

    m2::PointD m_org;

    void Reset()
    {
      m_name.clear();
      m_description.clear();
      m_org = m2::PointD(-1000, -1000);
      m_type.clear();

      m_styleId.clear();
      m_mapStyleId.clear();

      m_geometryType = GEOMETRY_TYPE_UNKNOWN;
    }

    bool ParsePoint(std::string const & s, char const * delim, m2::PointD & pt)
    {
      // order in string is: lon, lat, z

      strings::SimpleTokenizer iter(s, delim);
      if (iter)
      {
        double lon;
        if (strings::to_double(*iter, lon) && MercatorBounds::ValidLon(lon) && ++iter)
        {
          double lat;
          if (strings::to_double(*iter, lat) && MercatorBounds::ValidLat(lat))
          {
            pt = MercatorBounds::FromLatLon(lat, lon);
            return true;
          }
          else
            LOG(LWARNING, ("Invalid coordinates", s, "while loading", m_name));
        }
      }

      return false;
    }

    void SetOrigin(std::string const & s)
    {
      m_geometryType = GEOMETRY_TYPE_POINT;

      m2::PointD pt;
      if (ParsePoint(s, ", \n\r\t", pt))
        m_org = pt;
    }

    bool MakeValid()
    {
      if (GEOMETRY_TYPE_POINT != m_geometryType)
        return false;

      if (MercatorBounds::ValidX(m_org.x) && MercatorBounds::ValidY(m_org.y))
      {
        // set default name
        if (m_name.empty())
          m_name = PointToString(m_org);

        // set default pin
        if (m_type.empty())
          m_type = bnkSupportedColors[0];

        return true;
      }
      return false;
    }

  public:
    BNKParser(BookmarkManager * manager)
      : bmManager(manager)
    {
      Reset();
    }

    ~BNKParser()
    {
    }

    bool Push(std::string const & name)
    {
      m_tags.push_back(name);
      return true;
    }

    void AddAttr(std::string const & attr, std::string const & value)
    {
      std::string attrInLowerCase = attr;
      strings::AsciiToLower(attrInLowerCase);
    }

    bool IsValidAttribute(std::string const & type, std::string const & value,
                          std::string const & attrInLowerCase) const
    {
      return (GetTagFromEnd(0) == type && !value.empty() && attrInLowerCase == "id");
    }

    std::string const & GetTagFromEnd(size_t n) const
    {
      ASSERT_LESS(n, m_tags.size(), ());
      return m_tags[m_tags.size() - n - 1];
    }

    void Pop(std::string const & tag)
    {
      ASSERT_EQUAL(m_tags.back(), tag, ());

      if (tag == kBank)
      {
        if (MakeValid())
        {
          if (GEOMETRY_TYPE_POINT == m_geometryType)
          {
            // NOTE: the {@code CreateUserMark} adds it in the list of user marks
            // inside {@code BASE::UserMarkContainer}
            UserMarkNotificationGuard guard(*bmManager, UserMarkType::CUSTOM_BANK_MARK);
            CustomBankMark * cb = static_cast<CustomBankMark *>(guard.m_controller.CreateUserMark(m_org));
            cb->SetData(CustomBankData(m_name, m_type));
          }
        }
        Reset();
      }

      m_tags.pop_back();
    }

    void CharData(std::string value)
    {
      strings::Trim(value);

      size_t const count = m_tags.size();
      if (count > 1 && !value.empty())
      {
        std::string const & currTag = m_tags[count - 1];
        std::string const & prevTag = m_tags[count - 2];
        std::string const ppTag = count > 3 ? m_tags[count - 3] : std::string();

        if (prevTag == kDocument)
        {
          if (currTag == "visibility") {
            UserMarkNotificationGuard guard(*bmManager, UserMarkType::CUSTOM_BANK_MARK);
            guard.m_controller.SetIsVisible(value == "0" ? false : true);
          }
        }
        else if (prevTag == kBank)
        {
          if (currTag == "name")
            m_name = value;
          else if (currTag == kBankType)
            m_type = GetSupportedBNKStyle(value);
          else if (currTag == "description")
            m_description = value;
        }
        else if (ppTag == kBank)
        {
          if (prevTag == "Point")
          {
            if (currTag == "coordinates")
              SetOrigin(value);
          }
        }
      }
    }
  };
}

bool CustomBankManager::LoadFromBNK(ReaderPtr<Reader> const & reader)
{
  ReaderSource<ReaderPtr<Reader> > src(reader);
  ASSERT(m_bmManager != nullptr, ());
  BNKParser parser(m_bmManager);
  LOG(LDEBUG, ("Custom_Bank_Manager Starting File Reading"));
  if (ParseXML(src, parser, true))
  {
    LOG(LDEBUG, ("Custom_Bank_Manager File Reading Successful"));
    return true;
  }
  else
  {
    LOG(LERROR, ("Custom_Bank_Manager Reading error"));
    return false;
  }
}
