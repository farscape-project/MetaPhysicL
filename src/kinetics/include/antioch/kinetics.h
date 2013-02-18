//-----------------------------------------------------------------------bl-
//--------------------------------------------------------------------------
// 
// Antioch - A Gas Dynamics Thermochemistry Library
//
// Copyright (C) 2013 The PECOS Development Team
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the Version 2.1 GNU Lesser General
// Public License as published by the Free Software Foundation.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
// Lesser General Public License for more details.
//
// You should have received a copy of the GNU Lesser General Public
// License along with this library; if not, write to the Free Software
// Foundation, Inc. 51 Franklin Street, Fifth Floor,
// Boston, MA  02110-1301  USA
//
//-----------------------------------------------------------------------el-
//
// $Id$
//
//--------------------------------------------------------------------------
//--------------------------------------------------------------------------

#ifndef ANTIOCH_KINETICS_H
#define ANTIOCH_KINETICS_H

// C++
#include <vector>

// Antioch
#include "antioch/reaction.h"

namespace Antioch
{
  // Forward declarations
  template<class NumericType>
  class ReactionSet;

  template<class NumericType>
  class ChemicalMixture;
  
  //! Class to handle computing mass source terms for a given ReactionSet.
  /*! This class preallocates work arrays and so *must* be created within a spawned
   *  thread, if running in a threaded environment. It takes a reference to an
   *  already created ReactionSet, so there's little construction penalty.
   */
  template<class NumericType>
  class Kinetics
  {
  public:

    Kinetics( const ReactionSet<NumericType>& reaction_set );
    ~Kinetics();

    const ReactionSet<NumericType>& reaction_set() const;

    //! Compute species production/destruction rates per unit volume in \f$ \left(kg/sec/m^3\right)\f$
    void compute_mass_sources ( const NumericType T,
				const NumericType rho,
				const NumericType R_mix,
				const std::vector<NumericType>& mass_fractions,
				const std::vector<NumericType>& molar_densities,
				const std::vector<NumericType>& h_RT_minus_s_R,
				std::vector<NumericType>& mass_sources );

    unsigned int n_species() const;

    unsigned int n_reactions() const;

  protected:

    const ReactionSet<NumericType>& _reaction_set;

    const ChemicalMixture<NumericType>& _chem_mixture;

    //! Work arrays for compute mass sources
    std::vector<NumericType> _net_reaction_rates;
  };

  /* ------------------------- Inline Functions -------------------------*/
  template<class NumericType>
  inline
  const ReactionSet<NumericType>& Kinetics<NumericType>::reaction_set() const
  {
    return _reaction_set;
  }

  template<class NumericType>
  inline
  unsigned int Kinetics<NumericType>::n_species() const
  {
    return _chem_mixture.n_species();
  }

  template<class NumericType>
  inline
  unsigned int Kinetics<NumericType>::n_reactions() const
  {
    return _reaction_set.n_reactions();
  }


  template<class NumericType>
  inline
  Kinetics<NumericType>::Kinetics( const ReactionSet<NumericType>& reaction_set )
    : _reaction_set( reaction_set ),
      _chem_mixture( reaction_set.chemical_mixture() ),
      _net_reaction_rates( reaction_set.n_reactions(), 0.0 )
  {
    return;
  }


  template<class NumericType>
  inline
  Kinetics<NumericType>::~Kinetics()
  {
    return;
  }


  template<class NumericType>
  inline
  void Kinetics<NumericType>::compute_mass_sources( const NumericType T,
						    const NumericType rho,
						    const NumericType R_mix,
						    const std::vector<NumericType>& mass_fractions,
						    const std::vector<NumericType>& molar_densities,
						    const std::vector<NumericType>& h_RT_minus_s_R,
						    std::vector<NumericType>& mass_sources )
  {
    antioch_assert_greater(T, 0.0);
    antioch_assert_greater(rho, 0.0);
    antioch_assert_greater(R_mix, 0.0);
    antioch_assert_equal_to( mass_fractions.size(), this->n_species() );
    antioch_assert_equal_to( molar_densities.size(), this->n_species() );
    antioch_assert_equal_to( h_RT_minus_s_R.size(), this->n_species() );
    antioch_assert_equal_to( mass_sources.size(), this->n_species() );
    antioch_assert_equal_to( this->_net_reaction_rates.size(), this->n_reactions() );

    std::fill( mass_sources.begin(), mass_sources.end(), 0.0 );
    
    // compute the requisite reaction rates
    this->_reaction_set.compute_reaction_rates( T, rho, R_mix, mass_fractions, molar_densities,
						h_RT_minus_s_R, this->_net_reaction_rates );

    // compute the actual mass sources in kmol/sec/m^3
    for (unsigned int rxn = 0; rxn < this->n_reactions(); rxn++)
      {
	const Reaction<NumericType>& reaction = this->_reaction_set.reaction(rxn);
	const NumericType rate = this->_net_reaction_rates[rxn];
	
	// reactant contributions
	for (unsigned int r = 0; r < reaction.n_reactants(); r++)
	  {
	    const unsigned int r_id = reaction.reactant_id(r);
	    const unsigned int r_stoich = reaction.reactant_stoichiometric_coefficient(r);
	    
	    mass_sources[r_id] -= (static_cast<NumericType>(r_stoich)*rate);
	  }
	
	// product contributions
	for (unsigned int p=0; p < reaction.n_products(); p++)
	  {
	    const unsigned int p_id = reaction.product_id(p);
	    const unsigned int p_stoich = reaction.product_stoichiometric_coefficient(p);
	    
	    mass_sources[p_id] += (static_cast<NumericType>(p_stoich)*rate);
	  }
      }

    // finally scale by molar mass
    for (unsigned int s=0; s < this->n_species(); s++)
      {
	mass_sources[s] *= _chem_mixture.M(s);
      }
    
    return;
  }


} // end namespace Antioch

#endif // ANTIOCH_KINETICS_H
