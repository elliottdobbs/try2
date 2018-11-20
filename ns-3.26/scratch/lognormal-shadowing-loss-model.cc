/* -*-  Mode: C++; c-file-style: "gnu"; indent-tabs-mode:nil; -*- */
/*
 * Copyright (c) 2011, 2012 Centre Tecnologic de Telecomunicacions de Catalunya (CTTC)
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation;
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 *
 * Author: Mahima Agumbe Suresh <mahima.as@tamu.edu>
 * 
 */
#include "ns3/log.h"
#include "ns3/double.h"
#include "ns3/enum.h"
#include "ns3/string.h"
#include "ns3/pointer.h"
#include "ns3/mobility-model.h"
#include <cmath>

#include "lognormal-shadowing-loss-model.h"

namespace ns3 {

NS_LOG_COMPONENT_DEFINE ("LogNormalPropagationLossModel");

// ------------------------------------------------------------------------- //

NS_OBJECT_ENSURE_REGISTERED (LogNormalPropagationLossModel);

TypeId
LogNormalPropagationLossModel::GetTypeId (void)
{
  static TypeId tid = TypeId ("ns3::LogNormalPropagationLossModel")
    .SetParent<PropagationLossModel> ()
    .SetGroupName ("Propagation")
    .AddConstructor<LogNormalPropagationLossModel> ()
    .AddAttribute ("Exponent",
                   "The exponent of the Path Loss propagation model",
                   DoubleValue (3.0),
                   MakeDoubleAccessor (&LogNormalPropagationLossModel::m_exponent),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceDistance",
                   "The distance at which the reference loss is calculated (m)",
                   DoubleValue (1.0),
                   MakeDoubleAccessor (&LogNormalPropagationLossModel::m_referenceDistance),
                   MakeDoubleChecker<double> ())
    .AddAttribute ("ReferenceLoss",
                   "The reference loss at reference distance (dB). (Default is Friis at 1m with 5.15 GHz)",
                   DoubleValue (46.6777),
                   MakeDoubleAccessor (&LogNormalPropagationLossModel::m_referenceLoss),
                   MakeDoubleChecker<double> ())
	.AddAttribute ("RandomVariable", "The Gaussian random variable to add when CalcRxPower is invoked.",
                   StringValue ("ns3::NormalRandomVariable[Mean=0.0|Variance=4.0]"),
                   MakePointerAccessor (&LogNormalPropagationLossModel::m_randVariable),
                   MakePointerChecker<NormalRandomVariable> ())
  ;
  return tid;

}

LogNormalPropagationLossModel::LogNormalPropagationLossModel ()
{
}

void
LogNormalPropagationLossModel::SetPathLossExponent (double n)
{
  m_exponent = n;
}
void
LogNormalPropagationLossModel::SetReference (double referenceDistance, double referenceLoss)
{
  m_referenceDistance = referenceDistance;
  m_referenceLoss = referenceLoss;
}
double
LogNormalPropagationLossModel::GetPathLossExponent (void) const
{
  return m_exponent;
}

double
LogNormalPropagationLossModel::DoCalcRxPower (double txPowerDbm,
                                                Ptr<MobilityModel> a,
                                                Ptr<MobilityModel> b) const
{
  double distance = a->GetDistanceFrom (b);
  if (distance <= m_referenceDistance)
    {
      return txPowerDbm;
    }

  double pathLossDb = 10 * m_exponent * std::log10 (distance / m_referenceDistance) + m_randVariable->GetValue ();
  double rxc = -m_referenceLoss - pathLossDb;
  NS_LOG_DEBUG ("distance="<<distance<<"m, reference-attenuation="<< -m_referenceLoss<<"dB, "<<
                "attenuation coefficient="<<rxc<<"db");
  return txPowerDbm + rxc;
}

int64_t
LogNormalPropagationLossModel::DoAssignStreams (int64_t stream)
{
  m_randVariable->SetStream (stream);
  return 1;
}


} // namespace ns3
