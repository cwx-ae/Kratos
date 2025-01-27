
#include <iostream>
#include "DEM_D_Hertz_viscous_Coulomb_Nestle_CL.h"
#include "custom_elements/spheric_particle.h"

namespace Kratos {

    DEMDiscontinuumConstitutiveLaw::Pointer DEM_D_Hertz_viscous_Coulomb_Nestle::Clone() const {

        DEMDiscontinuumConstitutiveLaw::Pointer p_clone(new DEM_D_Hertz_viscous_Coulomb_Nestle(*this));
        return p_clone;
    }

    std::unique_ptr<DEMDiscontinuumConstitutiveLaw> DEM_D_Hertz_viscous_Coulomb_Nestle::CloneUnique() {
        return Kratos::make_unique<DEM_D_Hertz_viscous_Coulomb_Nestle>();
    }

    void DEM_D_Hertz_viscous_Coulomb_Nestle::SetConstitutiveLawInProperties(Properties::Pointer pProp, bool verbose) {

        if (verbose) KRATOS_INFO("DEM") << "Assigning DEM_D_Hertz_viscous_Coulomb_Nestle to Properties " << pProp->Id() << std::endl;
        pProp->SetValue(DEM_DISCONTINUUM_CONSTITUTIVE_LAW_POINTER, this->Clone());
    }

    void DEM_D_Hertz_viscous_Coulomb_Nestle::CalculateViscoDampingForce(double LocalRelVel[3],
                                                                 double ViscoDampingLocalContactForce[3],
                                                                 SphericParticle* const element1,
                                                                 SphericParticle* const element2) {

        const double my_mass    = element1->GetMass();
        const double other_mass = element2->GetMass();

        const double equiv_mass = 1.0 / (1.0/my_mass + 1.0/other_mass);

        Properties& properties_of_this_contact = element1->GetProperties().GetSubProperties(element2->GetProperties().Id());
        const double damping_gamma = properties_of_this_contact[DAMPING_GAMMA];

        const double equiv_visco_damp_coeff_normal     = 2.0 * damping_gamma * sqrt(equiv_mass * mKn);
        const double equiv_visco_damp_coeff_tangential = equiv_visco_damp_coeff_normal / 0.55;

        ViscoDampingLocalContactForce[0] = - equiv_visco_damp_coeff_tangential * LocalRelVel[0];
        ViscoDampingLocalContactForce[1] = - equiv_visco_damp_coeff_tangential * LocalRelVel[1];
        ViscoDampingLocalContactForce[2] = - equiv_visco_damp_coeff_normal     * LocalRelVel[2];
    }

    void DEM_D_Hertz_viscous_Coulomb_Nestle::CalculateViscoDampingForceWithFEM(double LocalRelVel[3],
                                                                double ViscoDampingLocalContactForce[3],
                                                                SphericParticle* const element,
                                                                Condition* const wall) {

        const double my_mass    = element->GetMass();

        Properties& properties_of_this_contact = element->GetProperties().GetSubProperties(wall->GetProperties().Id());
        const double damping_gamma = properties_of_this_contact[DAMPING_GAMMA];

        const double normal_damping_coefficient     = 2.0 * damping_gamma * sqrt(my_mass * mKn);
        const double tangential_damping_coefficient = normal_damping_coefficient / 0.55;

        ViscoDampingLocalContactForce[0] = - tangential_damping_coefficient * LocalRelVel[0];
        ViscoDampingLocalContactForce[1] = - tangential_damping_coefficient * LocalRelVel[1];
        ViscoDampingLocalContactForce[2] = - normal_damping_coefficient     * LocalRelVel[2];
    }
} //namespace Kratos
