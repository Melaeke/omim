#include "map/custom_bank_manager.hpp"
#include "map/user_mark.hpp"
#include "map/framework.hpp"
#include "map/user_mark.hpp"
#include "map/bookmark_manager.hpp"

#include "platform/platform.hpp"
#include "platform/settings.hpp"

#include "coding/parse_xml.hpp"

#include "indexer/scales.hpp"

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

void CustomBankManager::ClearCustomBanks()
{
  //m_customBanks.clear();
  auto & controller = m_bmManager->GetUserMarksController(UserMarkType::CUSTOM_BANK_MARK);
  //controller.Clear();
  //controller.NotifyChanges();
}

void CustomBankManager::SetBookmarkManager(BookmarkManager * bmManager)
{
  m_bmManager = bmManager;
}

BookmarkManager * CustomBankManager::GetBookmarkManager()
{
  return m_bmManager;
}

void CustomBankManager::LoadCustomBanks()
{
  ClearCustomBanks();

  string const dir = GetPlatform().SettingsDir();

  Platform::FilesList files;
  Platform::GetFilesByExt(dir, CUSTOM_BANKS_FILE_EXTENSION, files);
  /*
  for (size_t i = 0; i < files.size(); ++i)
    LoadCustomBank(dir + files[i]);
  */
  for (size_t i = 0; i < files.size(); ++i)
    LoadFromBNK(make_unique<FileReader>(dir + files[i]));
}

void CustomBankManager::NotifyChanges()
{
  /*
  auto & controller = m_bmManager->GetUserMarksController(UserMarkType::CUSTOM_BANK_MARK);
  controller.NotifyChanges();
  */
}

/*
void CustomBankManager::LoadCustomBank(string const & filePath)
{
  std::unique_ptr<CustomBankContainer> custom_bank(CustomBankContainer::CreateFromBNKFile(filePath, m_framework));
  ASSERT(custom_bank != null, ());
}
*/

namespace
{
  std::string const kBank = "Bank";
  std::string const kDocument = "Document";
  std::string const kBankType = "bankType";

  std::string const kDefaultTrackColor = "DefaultTrackColor";
  float const kDefaultTrackWidth = 5.0f;

  // TODO: update array with valid bank icons
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
    CustomBankManager & m_customBankManager;

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
    BNKParser(CustomBankManager & customBankManager)
      : m_customBankManager(customBankManager)
    {
      Reset();
    }

    ~BNKParser()
    {
      //m_customBankManager.NotifyChanges();
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
            auto & controller = m_customBankManager.GetBookmarkManager()->GetUserMarksController(UserMarkType::CUSTOM_BANK_MARK);
            CustomBankMark * cb = static_cast<CustomBankMark *>(controller.CreateUserMark(m_org));
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
            auto & controller = m_customBankManager.GetBookmarkManager()->GetUserMarksController(UserMarkType::CUSTOM_BANK_MARK);
            controller.SetIsVisible(value == "0" ? false : true);
          }
        }
        else if (prevTag == kBank)
        {
          if (currTag == "name")
            m_name = value;
          else if (currTag == "styleUrl")
          {
            m_type = GetSupportedBNKStyle(value);
          }
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
  BNKParser parser(*this);
  if (ParseXML(src, parser, true))
    return true;
  else
  {
    LOG(LERROR, ("XML read error. Probably, incorrect file encoding."));
    return false;
  }
}

namespace
{
char const * bnkHeader =
    "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
    "<kml xmlns=\"http://earth.google.com/kml/2.2\">\n"
    "<Document>\n"
    "  <Style id=\"placemark-blue\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-blue.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-brown\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-brown.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-green\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-green.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-orange\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-orange.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-pink\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-pink.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-purple\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-purple.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-red\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-red.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
    "  <Style id=\"placemark-yellow\">\n"
    "    <IconStyle>\n"
    "      <Icon>\n"
    "        <href>http://mapswith.me/placemarks/placemark-yellow.png</href>\n"
    "      </Icon>\n"
    "    </IconStyle>\n"
    "  </Style>\n"
;

char const * bnkFooter =
    "</Document>\n"
    "</kml>\n";
}

/*
namespace
{
  inline void SaveStringWithCDATA(std::ostream & stream, std::string const & s)
  {
    // According to kml/xml spec, we need to escape special symbols with CDATA
    if (s.find_first_of("<&") != std::string::npos)
      stream << "<![CDATA[" << s << "]]>";
    else
      stream << s;
  }
}
*/

/*
void CustomBankManager::SaveToBNK(std::ostream & s)
{
  s << kmlHeader;

  // Use CDATA if we have special symbols in the name
  s << "  <name>";
  SaveStringWithCDATA(s, GetName());
  s << "</name>\n";

  s << "  <visibility>" << (IsVisible() ? "1" : "0") <<"</visibility>\n";

  // Bookmarks are stored to KML file in reverse order, so, least
  // recently added bookmark will be stored last. The reason is that
  // when bookmarks will be loaded from the KML file, most recently
  // added bookmark will be loaded last and in accordance with current
  // logic will added to the beginning of the bookmarks list. Thus,
  // this method preserves LRU bookmarks order after store -> load
  // actions.
  //
  // Loop invariant: on each iteration count means number of already
  // stored bookmarks and i means index of the bookmark that should be
  // processed during the iteration. That's why i is initially set to
  // GetBookmarksCount() - 1, i.e. to the last bookmark in the
  // bookmarks list.
  for (size_t count = 0, i = GetUserMarkCount() - 1;
       count < GetUserPointCount(); ++count, --i)
  {
    CustomBankMark const * bm = static_cast<CustomBankMark const *>(GetUserMark(i));
    s << "  <Placemark>\n";
    s << "    <name>";
    SaveStringWithCDATA(s, bm->GetName());
    s << "</name>\n";

    if (!bm->GetDescription().empty())
    {
      s << "    <description>";
      SaveStringWithCDATA(s, bm->GetDescription());
      s << "</description>\n";
    }

    time_t const timeStamp = bm->GetTimeStamp();
    if (timeStamp != my::INVALID_TIME_STAMP)
    {
      std::string const strTimeStamp = my::TimestampToString(timeStamp);
      ASSERT_EQUAL(strTimeStamp.size(), 20, ("We always generate fixed length UTC-format timestamp"));
      s << "    <TimeStamp><when>" << strTimeStamp << "</when></TimeStamp>\n";
    }

    s << "    <styleUrl>#" << bm->GetType() << "</styleUrl>\n"
      << "    <Point><coordinates>" << PointToString(bm->GetPivot()) << "</coordinates></Point>\n";

    double const scale = bm->GetScale();
    if (scale != -1.0)
    {
      /// @todo Factor out to separate function to use for other custom params.
      s << "    <ExtendedData xmlns:mwm=\"http://mapswith.me\">\n"
        << "      <mwm:scale>" << bm->GetScale() << "</mwm:scale>\n"
        << "    </ExtendedData>\n";
    }

    s << "  </Placemark>\n";
  }

  // Saving tracks
  for (size_t i = 0; i < GetTracksCount(); ++i)
  {
    Track const * track = GetTrack(i);

    s << "  <Placemark>\n";
    s << "    <name>";
    SaveStringWithCDATA(s, track->GetName());
    s << "</name>\n";

    ASSERT_GREATER(track->GetLayerCount(), 0, ());

    s << "<Style><LineStyle>";
    dp::Color const & col = track->GetColor(0);
    s << "<color>"
      << NumToHex(col.GetAlfa())
      << NumToHex(col.GetBlue())
      << NumToHex(col.GetGreen())
      << NumToHex(col.GetRed());
    s << "</color>\n";

    s << "<width>"
      << track->GetWidth(0);
    s << "</width>\n";

    s << "</LineStyle></Style>\n";
    // stop style saving

    s << "    <LineString><coordinates>";

    Track::PolylineD const & poly = track->GetPolyline();
    for (auto pt = poly.Begin(); pt != poly.End(); ++pt)
      s << PointToString(*pt) << " ";

    s << "    </coordinates></LineString>\n"
      << "  </Placemark>\n";
  }

  s << kmlFooter;
}
*/
