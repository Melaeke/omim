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

  void LoadCustomBanks();

private:
  Framework & m_framework;

  BookmarkManager * m_bmManager = nullptr;

  bool LoadFromBNK(ReaderPtr<Reader> const & reader);

  void ClearCustomBanks();
};

