#pragma once

#include "map/user_mark_container.hpp"

#include <iostream>
#include <memory>
#include <string>
#include <vector>

struct CustomBankData
{
public:
  CustomBankData()
  {
  }

  CustomBankData(std::string const & name,
                 std::string const & symbolName)
    : m_name(name)
    , m_symbolName(symbolName)
  {
  }

  std::string m_name;
  std::string m_symbolName;
};

class CustomBankMark : public UserMark
{
public:
  CustomBankMark(const m2::PointD & ptOrg, UserMarkContainer * container);

  dp::Anchor GetAnchor() const override;
  df::RenderState::DepthLayer GetDepthLayer() const override;

  std::string GetSymbolName() const override { return m_data.m_symbolName; }
  UserMark::Type GetMarkType() const override { return UserMark::Type::CUSTOM_BANK; }

  void SetData(CustomBankData && data);
private:
  CustomBankData m_data;
};

class CustomBankContainer : public UserMarkContainer
{
public:
  CustomBankContainer(double layerDepth, Framework & framework);

protected:
  UserMark * AllocateUserMark(m2::PointD const & ptOrg) override;
};
