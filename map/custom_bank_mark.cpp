#include "map/custom_bank_mark.hpp"
#include "map/custom_bank_mark.hpp"

#include "base/scope_guard.hpp"

#include "geometry/mercator.hpp"

#include "coding/file_reader.hpp"
#include "coding/parse_xml.hpp"  // LoadFromKML
#include "coding/internal/file_data.hpp"
#include "coding/hex.hpp"

#include "drape/drape_global.hpp"
#include "drape/color.hpp"

#include "drape_frontend/color_constants.hpp"

#include "platform/platform.hpp"

#include "base/stl_add.hpp"
#include "base/string_utils.hpp"

#include <algorithm>
#include <fstream>
#include <iterator>
#include <map>
#include <memory>

CustomBankMark::CustomBankMark(m2::PointD const & ptOrg,
                           UserMarkContainer * container)
  : UserMark(ptOrg, container)
{
}

void CustomBankMark::SetData(CustomBankData && data)
{
  SetDirty();
  m_data = std::move(data);
}

dp::Anchor CustomBankMark::GetAnchor() const
{
  return dp::Bottom;
}

df::RenderState::DepthLayer CustomBankMark::GetDepthLayer() const
{
  return df::RenderState::CustomBankLayer;
}


CustomBankContainer::CustomBankContainer(double layerDepth, Framework & fm)
  : UserMarkContainer(layerDepth, UserMarkType::CUSTOM_BANK_MARK, fm)
{}

UserMark * CustomBankContainer::AllocateUserMark(m2::PointD const & ptOrg)
{
  return new CustomBankMark(ptOrg, this);
}
