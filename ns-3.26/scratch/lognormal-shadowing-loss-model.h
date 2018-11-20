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

#ifndef LOGNORMAL_PROPAGATION_LOSS_MODEL_H
#define LOGNORMAL_PROPAGATION_LOSS_MODEL_H

#include <ns3/propagation-loss-model.h>
#include "ns3/random-variable-stream.h"

namespace ns3 {


/**
 * \ingroup propagation
 *
 * \brief this class implements the Log Normal Shadowing propagation loss model
 * 
 * this class implements the Log Normal Shadowing propagation loss model.
 */
class LogNormalPropagationLossModel : public PropagationLossModel
{
public:
  /**
   * \brief Get the type ID.
   * \return the object TypeId
   */
  static TypeId GetTypeId (void);
  LogNormalPropagationLossModel ();

  /**
   * \param n the path loss exponent.
   * Set the path loss exponent.
   */
  void SetPathLossExponent (double n);
  /** 
   * \returns the current path loss exponent.
   */
  double GetPathLossExponent (void) const;

  /**
   * Set the reference path loss at a given distance
   * \param referenceDistance reference distance
   * \param referenceLoss reference path loss
   */
  void SetReference (double referenceDistance, double referenceLoss);

private:
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   */
  LogNormalPropagationLossModel (const LogNormalPropagationLossModel &);
  /**
   * \brief Copy constructor
   *
   * Defined and unimplemented to avoid misuse
   * \returns
   */
  LogNormalPropagationLossModel & operator = (const LogNormalPropagationLossModel &);

  virtual double DoCalcRxPower (double txPowerDbm,
                                Ptr<MobilityModel> a,
                                Ptr<MobilityModel> b) const;
  virtual int64_t DoAssignStreams (int64_t stream);

  /**
   *  Creates a default reference loss model
   * \return a default reference loss model
   */
  static Ptr<PropagationLossModel> CreateDefaultReference (void);

  double m_exponent; //!< model exponent
  double m_referenceDistance; //!< reference distance
  double m_referenceLoss; //!< reference loss
  Ptr<NormalRandomVariable> m_randVariable; 
};

} // namespace ns3


#endif // LOGNORMAL_PROPAGATION_LOSS_MODEL_H

