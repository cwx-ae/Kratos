// Authors: J. Irazábal (CIMNE)
// Date: November 2016

#include "DEM_D_Conical_damage_CL.h"
#include "DEM_D_Hertz_viscous_Coulomb_CL.h"
#include "custom_elements/spheric_particle.h"

namespace Kratos {

    DEMDiscontinuumConstitutiveLaw::Pointer DEM_D_Conical_damage::Clone() const {
        DEMDiscontinuumConstitutiveLaw::Pointer p_clone(new DEM_D_Conical_damage(*this));
        return p_clone;
    }

    std::unique_ptr<DEMDiscontinuumConstitutiveLaw> DEM_D_Conical_damage::CloneUnique() {
        return Kratos::make_unique<DEM_D_Conical_damage>();
    }

    void DEM_D_Conical_damage::SetConstitutiveLawInProperties(Properties::Pointer pProp, bool verbose) {
        if(verbose) KRATOS_INFO("DEM") << "Assigning DEM_D_Conical_damage to Properties " << pProp->Id() << std::endl;
        pProp->SetValue(DEM_DISCONTINUUM_CONSTITUTIVE_LAW_POINTER, this->Clone());
        this->Check(pProp);
    }

    void DEM_D_Conical_damage::Check(Properties::Pointer pProp) const {
        DEMDiscontinuumConstitutiveLaw::Check(pProp);
        if(!pProp->Has(CONICAL_DAMAGE_CONTACT_RADIUS)) {
            KRATOS_WARNING("DEM")<<std::endl;
            KRATOS_WARNING("DEM")<<"WARNING: Variable CONICAL_DAMAGE_CONTACT_RADIUS should be present in the properties when using DEM_D_Conical_damage. 0.0 value assigned by default."<<std::endl;
            KRATOS_WARNING("DEM")<<std::endl;
            pProp->GetValue(CONICAL_DAMAGE_CONTACT_RADIUS) = 0.0;
        }
        if(!pProp->Has(CONICAL_DAMAGE_MAX_STRESS)) {
            KRATOS_WARNING("DEM")<<std::endl;
            KRATOS_WARNING("DEM")<<"WARNING: Variable CONICAL_DAMAGE_MAX_STRESS should be present in the properties when using DEM_D_Conical_damage. 1.0e20 value assigned by default."<<std::endl;
            KRATOS_WARNING("DEM")<<std::endl;
            pProp->GetValue(CONICAL_DAMAGE_MAX_STRESS) = 1.0e20;
        }
        if(!pProp->Has(CONICAL_DAMAGE_ALPHA)) {
            KRATOS_WARNING("DEM")<<std::endl;
            KRATOS_WARNING("DEM")<<"WARNING: Variable CONICAL_DAMAGE_ALPHA should be present in the properties when using DEM_D_Conical_damage. 90.0 value assigned by default."<<std::endl;
            KRATOS_WARNING("DEM")<<std::endl;
            pProp->GetValue(CONICAL_DAMAGE_ALPHA) = 90.0;
        }
        if(!pProp->Has(CONICAL_DAMAGE_GAMMA)) {
            KRATOS_WARNING("DEM")<<std::endl;
            KRATOS_WARNING("DEM")<<"WARNING: Variable CONICAL_DAMAGE_GAMMA should be present in the properties when using DEM_D_Conical_damage. 0.0 value assigned by default."<<std::endl;
            KRATOS_WARNING("DEM")<<std::endl;
            pProp->GetValue(CONICAL_DAMAGE_GAMMA) = 0.0;
        }
    }

    std::string DEM_D_Conical_damage::GetTypeOfLaw() {
        std::string type_of_law = "Conical_damage";
        return type_of_law;
    }

    /////////////////////////
    // DEM-DEM INTERACTION //
    /////////////////////////

    void DEM_D_Conical_damage::InitializeDependentContact(double equiv_radius,
                                                          const double equiv_level_of_fouling,
                                                          const double equiv_young,
                                                          const double equiv_shear,
                                                          const double indentation) {
        //Normal and Tangent elastic constants
        const double sqrt_equiv_radius_and_indentation = sqrt(equiv_level_of_fouling * equiv_radius * indentation);
        mKn = 2.0 * equiv_young * sqrt_equiv_radius_and_indentation;
        mKt = 4.0 * equiv_shear * mKn / equiv_young;
    }

    void DEM_D_Conical_damage::DamageContact(ContactInfoSphericParticle* const element1,
                                             ContactInfoSphericParticle* const element2,
                                             double& equiv_radius,
                                             const double equiv_level_of_fouling,
                                             const double equiv_young,
                                             const double equiv_shear,
                                             double& indentation,
                                             const double normal_contact_force) {

        //Get new Equivalent Radius
        Properties& properties_of_this_contact = element1->GetProperties().GetSubProperties(element2->GetProperties().Id());
        double equiv_radius_new = (equiv_young * sqrt(6 * normal_contact_force)) / (pow(Globals::Pi * properties_of_this_contact[CONICAL_DAMAGE_MAX_STRESS],1.5));

        if (equiv_radius_new > equiv_level_of_fouling * equiv_radius) {
            const double AlphaFunction = properties_of_this_contact[CONICAL_DAMAGE_ALPHA_FUNCTION];
            double offset = (equiv_radius_new - equiv_radius) * AlphaFunction;
            equiv_radius = equiv_radius_new;

            for (unsigned int i = 0; element1->mNeighbourElements.size(); i++) {
                if (element1->mNeighbourElements[i]->Id() == element2->Id()) {
                    element1->mNeighbourContactRadius[i] = equiv_radius;
                    if (indentation > offset) element1->mNeighbourIndentation[i] = indentation - offset;
                    else element1->mNeighbourIndentation[i] = 0.0;
                    indentation = element1->mNeighbourIndentation[i];
                    break;
                }
            }
        }

        //New Normal and Tangent elastic constants
        const double sqrt_equiv_radius_and_indentation = sqrt(equiv_level_of_fouling * equiv_radius * indentation);
        mKn = 2.0 * equiv_young * sqrt_equiv_radius_and_indentation;
        mKt = 4.0 * equiv_shear * mKn / equiv_young;
    }

    void DEM_D_Conical_damage::CalculateForces(const ProcessInfo& r_process_info,
                                               const double OldLocalElasticContactForce[3],
                                               double LocalElasticContactForce[3],
                                               double LocalDeltDisp[3],
                                               double LocalRelVel[3],
                                               double indentation,
                                               double previous_indentation,
                                               double ViscoDampingLocalContactForce[3],
                                               double& cohesive_force,
                                               SphericParticle* element1,
                                               SphericParticle* element2,
                                               bool& sliding,
                                               double LocalCoordSystem[3][3]) {

        ContactInfoSphericParticle* p_element1 = dynamic_cast<ContactInfoSphericParticle*>(element1);
        ContactInfoSphericParticle* p_element2 = dynamic_cast<ContactInfoSphericParticle*>(element2);

        Properties& properties_of_this_contact = element1->GetProperties().GetSubProperties(element2->GetProperties().Id());

        double equiv_radius          = properties_of_this_contact[CONICAL_DAMAGE_CONTACT_RADIUS];
        double original_equiv_radius = equiv_radius;

        double elastic_indentation = indentation;

        for (unsigned int i = 0; p_element1->mNeighbourElements.size(); i++) {
            if (p_element1->mNeighbourElements[i]->Id() == p_element2->Id()) {
                if (p_element1->mNeighbourContactRadius[i] > equiv_radius) {
                    equiv_radius = p_element1->mNeighbourContactRadius[i];
                    p_element1->mNeighbourIndentation[i] += (indentation - previous_indentation);
                    elastic_indentation = p_element1->mNeighbourIndentation[i];
                }
                break;
            }
        }

        if (elastic_indentation > 0.0) {
            //Get equivalent Young's Modulus
            const double my_young        = p_element1->GetYoung();
            const double other_young     = p_element2->GetYoung();
            const double my_poisson      = p_element1->GetPoisson();
            const double other_poisson   = p_element2->GetPoisson();
            const double equiv_young     = my_young * other_young / (other_young * (1.0 - my_poisson * my_poisson) + my_young * (1.0 - other_poisson * other_poisson));

            //Get equivalent Shear Modulus
            const double my_shear_modulus = 0.5 * my_young / (1.0 + my_poisson);
            const double other_shear_modulus = 0.5 * other_young / (1.0 + other_poisson);
            const double equiv_shear = 1.0 / ((2.0 - my_poisson)/my_shear_modulus + (2.0 - other_poisson)/other_shear_modulus);

            //Level of fouling in case it is considered
            const double equiv_level_of_fouling = 1.0 + properties_of_this_contact[LEVEL_OF_FOULING];

            InitializeDependentContact(equiv_radius, equiv_level_of_fouling, equiv_young, equiv_shear, elastic_indentation);

            LocalElasticContactForce[2] = DEM_D_Hertz_viscous_Coulomb::CalculateNormalForce(elastic_indentation);

            double contact_stress = (3 * LocalElasticContactForce[2]) / (2 * Globals::Pi * equiv_level_of_fouling * equiv_radius * elastic_indentation);

            if (contact_stress > properties_of_this_contact[CONICAL_DAMAGE_MAX_STRESS]) {
                DamageContact(p_element1, p_element2, equiv_radius, equiv_level_of_fouling, equiv_young, equiv_shear, elastic_indentation, LocalElasticContactForce[2]);
                if (elastic_indentation > 0.0) LocalElasticContactForce[2] = DEM_D_Hertz_viscous_Coulomb::CalculateNormalForce(elastic_indentation);
                else LocalElasticContactForce[2] = 0.0;
            }

            if (elastic_indentation > 0.0) {
                CalculateViscoDampingForce(LocalRelVel, ViscoDampingLocalContactForce, p_element1, p_element2);

                double normal_contact_force = LocalElasticContactForce[2] + ViscoDampingLocalContactForce[2];

                if (normal_contact_force < 0.0) {
                    normal_contact_force = 0.0;
                    ViscoDampingLocalContactForce[2] = -1.0 * LocalElasticContactForce[2];
                }

                double AuxElasticShearForce;
                double MaximumAdmisibleShearForce;

                CalculateTangentialForce(normal_contact_force, OldLocalElasticContactForce, LocalElasticContactForce, ViscoDampingLocalContactForce, LocalDeltDisp,
                                         LocalRelVel, sliding, p_element1, p_element2, original_equiv_radius, equiv_young, elastic_indentation, previous_indentation, AuxElasticShearForce, MaximumAdmisibleShearForce);

                double& elastic_energy = p_element1->GetElasticEnergy();
                DEM_D_Hertz_viscous_Coulomb::CalculateElasticEnergyDEM(elastic_energy, elastic_indentation, LocalElasticContactForce);

                if(sliding && MaximumAdmisibleShearForce != 0.0){
                    double& inelastic_frictional_energy = p_element1->GetInelasticFrictionalEnergy();
                    DEM_D_Hertz_viscous_Coulomb::CalculateInelasticFrictionalEnergyDEM(inelastic_frictional_energy, AuxElasticShearForce, LocalElasticContactForce);
                }

                double& inelastic_viscodamping_energy = p_element1->GetInelasticViscodampingEnergy();
                DEM_D_Hertz_viscous_Coulomb::CalculateInelasticViscodampingEnergyDEM(inelastic_viscodamping_energy, ViscoDampingLocalContactForce, LocalDeltDisp);
            }
        }
    }

    /////////////////////////
    // DEM-FEM INTERACTION //
    /////////////////////////

    void DEM_D_Conical_damage::InitializeDependentContactWithFEM(double effective_radius,
                                                                 const double equiv_level_of_fouling,
                                                                 const double equiv_young,
                                                                 const double equiv_shear,
                                                                 const double indentation) {
        //Normal and Tangent elastic constants
        const double sqrt_equiv_radius_and_indentation = sqrt(equiv_level_of_fouling * effective_radius * indentation);
        mKn = 2.0 * equiv_young * sqrt_equiv_radius_and_indentation;
        mKt = 4.0 * equiv_shear * mKn / equiv_young;
    }

    void DEM_D_Conical_damage::DamageContactWithFEM(ContactInfoSphericParticle* const element,
                                                    Condition* const wall,
                                                    double& effective_radius,
                                                    const double equiv_level_of_fouling,
                                                    const double equiv_young,
                                                    const double equiv_shear,
                                                    double& indentation,
                                                    const double normal_contact_force) {
        //Get new Equivalent Radius
        Properties& properties_of_this_contact = element->GetProperties().GetSubProperties(wall->GetProperties().Id());
        double effective_radius_new = (equiv_young * sqrt(6 * normal_contact_force)) / (pow(Globals::Pi * properties_of_this_contact[CONICAL_DAMAGE_MAX_STRESS],1.5));

        if (effective_radius_new > equiv_level_of_fouling * effective_radius) {
            const double AlphaFunction = properties_of_this_contact[CONICAL_DAMAGE_ALPHA_FUNCTION];
            double offset = (effective_radius_new - effective_radius) * AlphaFunction;
            effective_radius = effective_radius_new;

            for (unsigned int i = 0; element->mNeighbourRigidFaces.size(); i++) {
                if (element->mNeighbourRigidFaces[i]->Id() == wall->Id()) {
                    element->mNeighbourRigidContactRadius[i] = effective_radius;
                    element->mNeighbourRigidIndentation[i] = indentation - offset;
                    indentation = element->mNeighbourRigidIndentation[i];
                    break;
                }
            }
        }

        //New Normal and Tangent elastic constants
        const double sqrt_equiv_radius_and_indentation = sqrt(equiv_level_of_fouling * effective_radius * indentation);
        mKn = 2.0 * equiv_young * sqrt_equiv_radius_and_indentation;
        mKt = 4.0 * equiv_shear * mKn / equiv_young;
    }

    void DEM_D_Conical_damage::CalculateForcesWithFEM(const ProcessInfo& r_process_info,
                                                      const double OldLocalElasticContactForce[3],
                                                      double LocalElasticContactForce[3],
                                                      double LocalDeltDisp[3],
                                                      double LocalRelVel[3],
                                                      double indentation,
                                                      double previous_indentation,
                                                      double ViscoDampingLocalContactForce[3],
                                                      double& cohesive_force,
                                                      SphericParticle* const element,
                                                      Condition* const wall,
                                                      bool& sliding) {

        ContactInfoSphericParticle* p_element = dynamic_cast<ContactInfoSphericParticle*>(element);

        //Get effective Radius
        Properties& properties_of_this_contact = element->GetProperties().GetSubProperties(wall->GetProperties().Id());
        double effective_radius = properties_of_this_contact[CONICAL_DAMAGE_CONTACT_RADIUS];
        double original_effective_radius = effective_radius;

        double elastic_indentation = indentation;

        for (unsigned int i = 0; p_element->mNeighbourRigidFaces.size(); i++) {
            if (p_element->mNeighbourRigidFaces[i]->Id() == wall->Id()) {
                if (p_element->mNeighbourRigidContactRadius[i] > effective_radius) {
                    effective_radius = p_element->mNeighbourRigidContactRadius[i];
                    p_element->mNeighbourRigidIndentation[i] += (indentation - previous_indentation);
                    elastic_indentation = p_element->mNeighbourRigidIndentation[i];
                }
                break;
            }
        }

        if (elastic_indentation > 0.0) {
            //Get equivalent Young's Modulus
            const double my_young            = p_element->GetYoung();
            const double walls_young         = wall->GetProperties()[YOUNG_MODULUS];
            const double my_poisson          = p_element->GetPoisson();
            const double walls_poisson       = wall->GetProperties()[POISSON_RATIO];
            const double equiv_young         = my_young * walls_young / (walls_young * (1.0 - my_poisson * my_poisson) + my_young * (1.0 - walls_poisson * walls_poisson));

            //Get equivalent Shear Modulus
            const double my_shear_modulus    = 0.5 * my_young / (1.0 + my_poisson);
            const double walls_shear_modulus = 0.5 * walls_young / (1.0 + walls_poisson);
            const double equiv_shear         = 1.0 / ((2.0 - my_poisson)/my_shear_modulus + (2.0 - walls_poisson)/walls_shear_modulus);


            //Level of fouling in case it is considered
            const double equiv_level_of_fouling = 1.0 + properties_of_this_contact[LEVEL_OF_FOULING];

            InitializeDependentContactWithFEM(effective_radius, equiv_level_of_fouling, equiv_young, equiv_shear, elastic_indentation);

            LocalElasticContactForce[2] = DEM_D_Hertz_viscous_Coulomb::CalculateNormalForce(elastic_indentation);

            double contact_stress = (3 * LocalElasticContactForce[2]) / (2 * Globals::Pi * equiv_level_of_fouling * effective_radius * elastic_indentation);

            if (contact_stress > properties_of_this_contact[CONICAL_DAMAGE_MAX_STRESS]) {
                DamageContactWithFEM(p_element, wall, effective_radius, equiv_level_of_fouling, equiv_young, equiv_shear, elastic_indentation, LocalElasticContactForce[2]);
                if (elastic_indentation > 0.0) LocalElasticContactForce[2] = DEM_D_Hertz_viscous_Coulomb::CalculateNormalForce(elastic_indentation);
                else LocalElasticContactForce[2] = 0.0;
            }

            if (elastic_indentation > 0.0) {
                CalculateViscoDampingForceWithFEM(LocalRelVel, ViscoDampingLocalContactForce, p_element, wall);

                double normal_contact_force = LocalElasticContactForce[2] + ViscoDampingLocalContactForce[2];

                if (normal_contact_force < 0.0) {
                    normal_contact_force = 0.0;
                    ViscoDampingLocalContactForce[2] = -1.0 * LocalElasticContactForce[2];
                }

                double AuxElasticShearForce;
                double MaximumAdmisibleShearForce;

                CalculateTangentialForceWithFEM(normal_contact_force, OldLocalElasticContactForce, LocalElasticContactForce, ViscoDampingLocalContactForce, LocalDeltDisp,
                                                LocalRelVel, sliding, p_element, wall, original_effective_radius, equiv_young, elastic_indentation, previous_indentation, AuxElasticShearForce, MaximumAdmisibleShearForce);

                double& elastic_energy = p_element->GetElasticEnergy();
                DEM_D_Hertz_viscous_Coulomb::CalculateElasticEnergyFEM(elastic_energy, elastic_indentation, LocalElasticContactForce);

                if(sliding && MaximumAdmisibleShearForce != 0.0){
                    double& inelastic_frictional_energy = p_element->GetInelasticFrictionalEnergy();
                    DEM_D_Hertz_viscous_Coulomb::CalculateInelasticFrictionalEnergyFEM(inelastic_frictional_energy, AuxElasticShearForce, LocalElasticContactForce);
                }

                double& inelastic_viscodamping_energy = p_element->GetInelasticViscodampingEnergy();
                DEM_D_Hertz_viscous_Coulomb::CalculateInelasticViscodampingEnergyFEM(inelastic_viscodamping_energy, ViscoDampingLocalContactForce, LocalDeltDisp);
            }
        }
    }

    void DEM_D_Conical_damage::CalculateViscoDampingForce(double LocalRelVel[3],
                                                          double ViscoDampingLocalContactForce[3],
                                                          ContactInfoSphericParticle* const element1,
                                                          ContactInfoSphericParticle* const element2) {

        const double my_mass    = element1->GetMass();
        const double other_mass = element2->GetMass();

        const double equiv_mass = 1.0 / (1.0/my_mass + 1.0/other_mass);

        Properties& properties_of_this_contact = element1->GetProperties().GetSubProperties(element2->GetProperties().Id());
        const double damping_gamma = properties_of_this_contact[DAMPING_GAMMA];

        const double equiv_visco_damp_coeff_normal     = 2.0 * damping_gamma * sqrt(equiv_mass * mKn);
        const double equiv_visco_damp_coeff_tangential = 2.0 * damping_gamma * sqrt(equiv_mass * mKt);

        ViscoDampingLocalContactForce[0] = - equiv_visco_damp_coeff_tangential * LocalRelVel[0];
        ViscoDampingLocalContactForce[1] = - equiv_visco_damp_coeff_tangential * LocalRelVel[1];
        ViscoDampingLocalContactForce[2] = - equiv_visco_damp_coeff_normal     * LocalRelVel[2];
    }

    void DEM_D_Conical_damage::CalculateTangentialForce(const double normal_contact_force,
                                                        const double OldLocalElasticContactForce[3],
                                                        double LocalElasticContactForce[3],
                                                        double ViscoDampingLocalContactForce[3],
                                                        const double LocalDeltDisp[3],
                                                        const double LocalRelVel[3],
                                                        bool& sliding,
                                                        ContactInfoSphericParticle* const element1,
                                                        ContactInfoSphericParticle* const element2,
                                                        const double original_equiv_radius,
                                                        const double equiv_young,
                                                        double indentation,
                                                        double previous_indentation,
                                                        double& AuxElasticShearForce,
                                                        double& MaximumAdmisibleShearForce) {

        LocalElasticContactForce[0] = OldLocalElasticContactForce[0] - mKt * LocalDeltDisp[0];
        LocalElasticContactForce[1] = OldLocalElasticContactForce[1] - mKt * LocalDeltDisp[1];

        if (previous_indentation > indentation) {
            const double minoring_factor = sqrt (indentation / previous_indentation);
            LocalElasticContactForce[0] = OldLocalElasticContactForce[0] * minoring_factor - mKt * LocalDeltDisp[0];
            LocalElasticContactForce[1] = OldLocalElasticContactForce[1] * minoring_factor - mKt * LocalDeltDisp[1];
        }

        AuxElasticShearForce = sqrt(LocalElasticContactForce[0] * LocalElasticContactForce[0] + LocalElasticContactForce[1] * LocalElasticContactForce[1]);

        Properties& properties_of_this_contact = element1->GetProperties().GetSubProperties(element2->GetProperties().Id());

        double equiv_tg_of_static_fri_ang = properties_of_this_contact[STATIC_FRICTION];
        double equiv_tg_of_dynamic_fri_ang = properties_of_this_contact[DYNAMIC_FRICTION];
        double equiv_friction_decay_coefficient = properties_of_this_contact[FRICTION_DECAY];

        if (fabs(equiv_tg_of_static_fri_ang) > 1.0e-12 || fabs(equiv_tg_of_dynamic_fri_ang) > 1.0e-12) {
            double critical_force = 0.166666667 * pow(Globals::Pi * properties_of_this_contact[CONICAL_DAMAGE_MAX_STRESS], 3) * pow(original_equiv_radius / equiv_young, 2);
            if (LocalElasticContactForce[2] > critical_force) {
                double critical_force_inv = 1.0  / critical_force;
                double wear_value = pow((LocalElasticContactForce[2] * critical_force_inv), properties_of_this_contact[CONICAL_DAMAGE_GAMMA]);
                equiv_tg_of_static_fri_ang *= wear_value;
                equiv_tg_of_dynamic_fri_ang *= wear_value;
            }
        }

        for (unsigned int i = 0; element1->mNeighbourElements.size(); i++) {
            if (element1->mNeighbourElements[i]->Id() == element2->Id()) {
                if (element1->mNeighbourTgOfStatFriAng[i] <= equiv_tg_of_static_fri_ang) equiv_tg_of_static_fri_ang = element1->mNeighbourTgOfStatFriAng[i];
                else element1->mNeighbourTgOfStatFriAng[i] = equiv_tg_of_static_fri_ang;
                if (element1->mNeighbourTgOfDynFriAng[i] <= equiv_tg_of_dynamic_fri_ang) equiv_tg_of_dynamic_fri_ang = element1->mNeighbourTgOfDynFriAng[i];
                else element1->mNeighbourTgOfDynFriAng[i] = equiv_tg_of_dynamic_fri_ang;
                break;
            }
        }

        const double ShearRelVel = sqrt(LocalRelVel[0] * LocalRelVel[0] + LocalRelVel[1] * LocalRelVel[1]);
        double equiv_friction = equiv_tg_of_dynamic_fri_ang + (equiv_tg_of_static_fri_ang - equiv_tg_of_dynamic_fri_ang) * exp(-equiv_friction_decay_coefficient * ShearRelVel);

        MaximumAdmisibleShearForce = normal_contact_force * equiv_friction;

        const double tangential_contact_force_0 = LocalElasticContactForce[0] + ViscoDampingLocalContactForce[0];
        const double tangential_contact_force_1 = LocalElasticContactForce[1] + ViscoDampingLocalContactForce[1];

        const double ActualTotalShearForce = sqrt(tangential_contact_force_0 * tangential_contact_force_0 + tangential_contact_force_1 * tangential_contact_force_1);

        if (ActualTotalShearForce > MaximumAdmisibleShearForce) {

            const double ActualElasticShearForce = sqrt(LocalElasticContactForce[0] * LocalElasticContactForce[0] + LocalElasticContactForce[1] * LocalElasticContactForce[1]);

            const double dot_product = LocalElasticContactForce[0] * ViscoDampingLocalContactForce[0] + LocalElasticContactForce[1] * ViscoDampingLocalContactForce[1];
            const double ViscoDampingLocalContactForceModule = sqrt(ViscoDampingLocalContactForce[0] * ViscoDampingLocalContactForce[0] +\
                                                                    ViscoDampingLocalContactForce[1] * ViscoDampingLocalContactForce[1]);

            if (dot_product >= 0.0) {

                if (ActualElasticShearForce > MaximumAdmisibleShearForce) {
                    const double fraction = MaximumAdmisibleShearForce / ActualElasticShearForce;
                    LocalElasticContactForce[0]      = LocalElasticContactForce[0] * fraction;
                    LocalElasticContactForce[1]      = LocalElasticContactForce[1] * fraction;
                    ViscoDampingLocalContactForce[0] = 0.0;
                    ViscoDampingLocalContactForce[1] = 0.0;
                }
                else {
                    const double ActualViscousShearForce = MaximumAdmisibleShearForce - ActualElasticShearForce;
                    const double fraction = ActualViscousShearForce / ViscoDampingLocalContactForceModule;
                    ViscoDampingLocalContactForce[0] *= fraction;
                    ViscoDampingLocalContactForce[1] *= fraction;
                }
            }
            else {
                if (ViscoDampingLocalContactForceModule >= ActualElasticShearForce) {
                    const double fraction = (MaximumAdmisibleShearForce + ActualElasticShearForce) / ViscoDampingLocalContactForceModule;
                    ViscoDampingLocalContactForce[0] *= fraction;
                    ViscoDampingLocalContactForce[1] *= fraction;
                }
                else {
                    const double fraction = MaximumAdmisibleShearForce / ActualElasticShearForce;
                    LocalElasticContactForce[0]      = LocalElasticContactForce[0] * fraction;
                    LocalElasticContactForce[1]      = LocalElasticContactForce[1] * fraction;
                    ViscoDampingLocalContactForce[0] = 0.0;
                    ViscoDampingLocalContactForce[1] = 0.0;
                }
            }
            sliding = true;
        }
    }

    void DEM_D_Conical_damage::CalculateViscoDampingForceWithFEM(double LocalRelVel[3],
                                                                 double ViscoDampingLocalContactForce[3],
                                                                 ContactInfoSphericParticle* const element,
                                                                 Condition* const wall) {

        const double my_mass    = element->GetMass();

        Properties& properties_of_this_contact = element->GetProperties().GetSubProperties(wall->GetProperties().Id());
        const double damping_gamma = properties_of_this_contact[DAMPING_GAMMA];

        const double normal_damping_coefficient     = 2.0 * damping_gamma * sqrt(my_mass * mKn);
        const double tangential_damping_coefficient = 2.0 * damping_gamma * sqrt(my_mass * mKt);

        ViscoDampingLocalContactForce[0] = - tangential_damping_coefficient * LocalRelVel[0];
        ViscoDampingLocalContactForce[1] = - tangential_damping_coefficient * LocalRelVel[1];
        ViscoDampingLocalContactForce[2] = - normal_damping_coefficient     * LocalRelVel[2];

    }

    void DEM_D_Conical_damage::CalculateTangentialForceWithFEM(const double normal_contact_force,
                                                               const double OldLocalElasticContactForce[3],
                                                               double LocalElasticContactForce[3],
                                                               double ViscoDampingLocalContactForce[3],
                                                               const double LocalDeltDisp[3],
                                                               const double LocalRelVel[3],
                                                               bool& sliding,
                                                               ContactInfoSphericParticle* const element,
                                                               Condition* const wall,
                                                               const double original_effective_radius,
                                                               const double equiv_young,
                                                               double indentation,
                                                               double previous_indentation,
                                                               double& AuxElasticShearForce,
                                                               double& MaximumAdmisibleShearForce) {

        LocalElasticContactForce[0] = OldLocalElasticContactForce[0] - mKt * LocalDeltDisp[0];
        LocalElasticContactForce[1] = OldLocalElasticContactForce[1] - mKt * LocalDeltDisp[1];

        if (previous_indentation > indentation) {
            const double minoring_factor = sqrt (indentation / previous_indentation);
            LocalElasticContactForce[0] = OldLocalElasticContactForce[0] * minoring_factor - mKt * LocalDeltDisp[0];
            LocalElasticContactForce[1] = OldLocalElasticContactForce[1] * minoring_factor - mKt * LocalDeltDisp[1];
        }

        AuxElasticShearForce = sqrt(LocalElasticContactForce[0] * LocalElasticContactForce[0] + LocalElasticContactForce[1] * LocalElasticContactForce[1]);

        Properties& properties_of_this_contact = element->GetProperties().GetSubProperties(wall->GetProperties().Id());

        double equiv_tg_of_static_fri_ang = properties_of_this_contact[STATIC_FRICTION];
        double equiv_tg_of_dynamic_fri_ang = properties_of_this_contact[DYNAMIC_FRICTION];
        double equiv_friction_decay_coefficient = properties_of_this_contact[FRICTION_DECAY];

        if (fabs(equiv_tg_of_static_fri_ang) > 1.0e-12 || fabs(equiv_tg_of_dynamic_fri_ang) > 1.0e-12) {
            double critical_force = 0.166666667 * pow(Globals::Pi * properties_of_this_contact[CONICAL_DAMAGE_MAX_STRESS], 3) * pow(original_effective_radius / equiv_young, 2);
            if (LocalElasticContactForce[2] > critical_force) {
                double critical_force_inv = 1.0  / critical_force;
                double wear_value = pow((LocalElasticContactForce[2] * critical_force_inv), properties_of_this_contact[CONICAL_DAMAGE_GAMMA]);
                equiv_tg_of_static_fri_ang *= wear_value;
                equiv_tg_of_dynamic_fri_ang *= wear_value;
            }
        }

        for (unsigned int i = 0; element->mNeighbourRigidFaces.size(); i++) {
            if (element->mNeighbourRigidFaces[i]->Id() == wall->Id()) {
                if (element->mNeighbourRigidTgOfStatFriAng[i] <= equiv_tg_of_static_fri_ang) equiv_tg_of_static_fri_ang = element->mNeighbourRigidTgOfStatFriAng[i];
                else element->mNeighbourRigidTgOfStatFriAng[i] = equiv_tg_of_static_fri_ang;
                if (element->mNeighbourRigidTgOfDynFriAng[i] <= equiv_tg_of_dynamic_fri_ang) equiv_tg_of_dynamic_fri_ang = element->mNeighbourRigidTgOfDynFriAng[i];
                else element->mNeighbourRigidTgOfDynFriAng[i] = equiv_tg_of_dynamic_fri_ang;
                break;
            }
        }

        const double ShearRelVel = sqrt(LocalRelVel[0] * LocalRelVel[0] + LocalRelVel[1] * LocalRelVel[1]);
        double equiv_friction = equiv_tg_of_dynamic_fri_ang + (equiv_tg_of_static_fri_ang - equiv_tg_of_dynamic_fri_ang) * exp(-equiv_friction_decay_coefficient * ShearRelVel);

        MaximumAdmisibleShearForce = normal_contact_force * equiv_friction;

        const double tangential_contact_force_0 = LocalElasticContactForce[0] + ViscoDampingLocalContactForce[0];
        const double tangential_contact_force_1 = LocalElasticContactForce[1] + ViscoDampingLocalContactForce[1];

        const double ActualTotalShearForce = sqrt(tangential_contact_force_0 * tangential_contact_force_0 + tangential_contact_force_1 * tangential_contact_force_1);

        if (ActualTotalShearForce > MaximumAdmisibleShearForce) {

            const double ActualElasticShearForce = sqrt(LocalElasticContactForce[0] * LocalElasticContactForce[0] + LocalElasticContactForce[1] * LocalElasticContactForce[1]);

            const double dot_product = LocalElasticContactForce[0] * ViscoDampingLocalContactForce[0] + LocalElasticContactForce[1] * ViscoDampingLocalContactForce[1];
            const double ViscoDampingLocalContactForceModule = sqrt(ViscoDampingLocalContactForce[0] * ViscoDampingLocalContactForce[0] +\
                                                                    ViscoDampingLocalContactForce[1] * ViscoDampingLocalContactForce[1]);

            if (dot_product >= 0.0) {

                if (ActualElasticShearForce > MaximumAdmisibleShearForce) {
                    const double fraction = MaximumAdmisibleShearForce / ActualElasticShearForce;
                    LocalElasticContactForce[0]      = LocalElasticContactForce[0] * fraction;
                    LocalElasticContactForce[1]      = LocalElasticContactForce[1] * fraction;
                    ViscoDampingLocalContactForce[0] = 0.0;
                    ViscoDampingLocalContactForce[1] = 0.0;
                }
                else {
                    const double ActualViscousShearForce = MaximumAdmisibleShearForce - ActualElasticShearForce;
                    const double fraction = ActualViscousShearForce / ViscoDampingLocalContactForceModule;
                    ViscoDampingLocalContactForce[0] *= fraction;
                    ViscoDampingLocalContactForce[1] *= fraction;
                }
            }
            else {
                if (ViscoDampingLocalContactForceModule >= ActualElasticShearForce) {
                    const double fraction = (MaximumAdmisibleShearForce + ActualElasticShearForce) / ViscoDampingLocalContactForceModule;
                    ViscoDampingLocalContactForce[0] *= fraction;
                    ViscoDampingLocalContactForce[1] *= fraction;
                }
                else {
                    const double fraction = MaximumAdmisibleShearForce / ActualElasticShearForce;
                    LocalElasticContactForce[0]      = LocalElasticContactForce[0] * fraction;
                    LocalElasticContactForce[1]      = LocalElasticContactForce[1] * fraction;
                    ViscoDampingLocalContactForce[0] = 0.0;
                    ViscoDampingLocalContactForce[1] = 0.0;
                }
            }
            sliding = true;
        }
    }

} // namespace Kratos
