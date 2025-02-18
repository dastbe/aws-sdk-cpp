﻿/*
* Copyright 2010-2017 Amazon.com, Inc. or its affiliates. All Rights Reserved.
*
* Licensed under the Apache License, Version 2.0 (the "License").
* You may not use this file except in compliance with the License.
* A copy of the License is located at
*
*  http://aws.amazon.com/apache2.0
*
* or in the "license" file accompanying this file. This file is distributed
* on an "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either
* express or implied. See the License for the specific language governing
* permissions and limitations under the License.
*/

#include <aws/ec2/model/VpnConnectionOptionsSpecification.h>
#include <aws/core/utils/xml/XmlSerializer.h>
#include <aws/core/utils/StringUtils.h>
#include <aws/core/utils/memory/stl/AWSStringStream.h>

#include <utility>

using namespace Aws::Utils::Xml;
using namespace Aws::Utils;

namespace Aws
{
namespace EC2
{
namespace Model
{

VpnConnectionOptionsSpecification::VpnConnectionOptionsSpecification() : 
    m_staticRoutesOnly(false),
    m_staticRoutesOnlyHasBeenSet(false),
    m_tunnelOptionsHasBeenSet(false)
{
}

VpnConnectionOptionsSpecification::VpnConnectionOptionsSpecification(const XmlNode& xmlNode) : 
    m_staticRoutesOnly(false),
    m_staticRoutesOnlyHasBeenSet(false),
    m_tunnelOptionsHasBeenSet(false)
{
  *this = xmlNode;
}

VpnConnectionOptionsSpecification& VpnConnectionOptionsSpecification::operator =(const XmlNode& xmlNode)
{
  XmlNode resultNode = xmlNode;

  if(!resultNode.IsNull())
  {
    XmlNode staticRoutesOnlyNode = resultNode.FirstChild("staticRoutesOnly");
    if(!staticRoutesOnlyNode.IsNull())
    {
      m_staticRoutesOnly = StringUtils::ConvertToBool(StringUtils::Trim(Aws::Utils::Xml::DecodeEscapedXmlText(staticRoutesOnlyNode.GetText()).c_str()).c_str());
      m_staticRoutesOnlyHasBeenSet = true;
    }
    XmlNode tunnelOptionsNode = resultNode.FirstChild("TunnelOptions");
    if(!tunnelOptionsNode.IsNull())
    {
      XmlNode tunnelOptionsMember = tunnelOptionsNode.FirstChild("item");
      while(!tunnelOptionsMember.IsNull())
      {
        m_tunnelOptions.push_back(tunnelOptionsMember);
        tunnelOptionsMember = tunnelOptionsMember.NextNode("item");
      }

      m_tunnelOptionsHasBeenSet = true;
    }
  }

  return *this;
}

void VpnConnectionOptionsSpecification::OutputToStream(Aws::OStream& oStream, const char* location, unsigned index, const char* locationValue) const
{
  if(m_staticRoutesOnlyHasBeenSet)
  {
      oStream << location << index << locationValue << ".StaticRoutesOnly=" << std::boolalpha << m_staticRoutesOnly << "&";
  }

  if(m_tunnelOptionsHasBeenSet)
  {
      unsigned tunnelOptionsIdx = 1;
      for(auto& item : m_tunnelOptions)
      {
        Aws::StringStream tunnelOptionsSs;
        tunnelOptionsSs << location << index << locationValue << ".TunnelOptions." << tunnelOptionsIdx++;
        item.OutputToStream(oStream, tunnelOptionsSs.str().c_str());
      }
  }

}

void VpnConnectionOptionsSpecification::OutputToStream(Aws::OStream& oStream, const char* location) const
{
  if(m_staticRoutesOnlyHasBeenSet)
  {
      oStream << location << ".StaticRoutesOnly=" << std::boolalpha << m_staticRoutesOnly << "&";
  }
  if(m_tunnelOptionsHasBeenSet)
  {
      unsigned tunnelOptionsIdx = 1;
      for(auto& item : m_tunnelOptions)
      {
        Aws::StringStream tunnelOptionsSs;
        tunnelOptionsSs << location <<  ".Item." << tunnelOptionsIdx++;
        item.OutputToStream(oStream, tunnelOptionsSs.str().c_str());
      }
  }
}

} // namespace Model
} // namespace EC2
} // namespace Aws
