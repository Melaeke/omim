#pragma once

#include "map/bookmark_manager.hpp"
#include "map/custom_bank_mark.hpp"

#include "platform/platform.hpp"
#include "geometry/any_rect2d.hpp"

#include <functional>
#include <memory>
#include <string>
#include <vector>

class CustomBankManager final
{
public:
  CustomBankManager(Framework & f);
  ~CustomBankManager();

  void SetBookmarkManager(BookmarkManager * bmManager);
  BookmarkManager * GetBookmarkManager();

  void ClearCustomBanks();

  void LoadCustomBanks();

  void NotifyChanges();

  //void LoadCustomBank(string const & filePath);

  /*
  size_t CreateCustomBankContainer(std::string const & name);
  void SetName(std::string const & name) { m_name = name; }
  std::string const & GetName() const { return m_name; }
  std::string const & GetFileName() const { return m_file; }
  */

  //bool LoadFromBNK(ReaderPtr<Reader> const & reader);
  //void SaveToBNK(std::ostream & s);

  /// Uses the same file name from which was loaded, or
  /// creates unique file name on first save and uses it every time.
  //bool SaveToBNKFile();

private:
  using CustomBankCollection = std::vector<std::unique_ptr<CustomBankContainer>>;
  using CustomBankIter = CustomBankCollection::iterator;

  //CustomBankCollection m_customBanks;

  Framework & m_framework;

  BookmarkManager * m_bmManager = nullptr;

  bool LoadFromBNK(ReaderPtr<Reader> const & reader);

  //std::string m_name;
  //std::string m_file;
};

