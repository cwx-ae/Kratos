// KRATOS   ___                _   _ _         _   _             __
//        / __\___  _ __  ___| |_(_) |_ _   _| |_(_)_   _____  / /  __ ___      _____
//       / /  / _ \| '_ \/ __| __| | __| | | | __| \ \ / / _ \/ /  / _` \ \ /\ / / __|
//      / /__| (_) | | | \__ \ |_| | |_| |_| | |_| |\ V /  __/ /__| (_| |\ V  V /\__ \
//      \____/\___/|_| |_|___/\__|_|\__|\__,_|\__|_| \_/ \___\____/\__,_| \_/\_/ |___/  Application
//
//  License:         BSD License
//                   license: constitutive_laws_application/license.txt
//
//  Main authors:   Alejandro Cornejo
//  Collaborator:
//

#if !defined(KRATOS_CALCULATE_CRACK_LENGTH_UTILITY_H_INCLUDED)
#define KRATOS_CALCULATE_CRACK_LENGTH_UTILITY_H_INCLUDED

// System includes

// External includes

// Project includes
#include "includes/model_part.h"
#include "structural_mechanics_application_variables.h"

namespace Kratos
{

///@name Kratos Globals
///@{

///@}
///@name Type Definitions
///@{

    typedef std::size_t SizeType;

///@}
///@name  Enum's
///@{

///@}
///@name  Functions
///@{

///@}
///@name Kratos Classes
///@{

/**
 * @class CalculateCrackLengthUtility
 * @ingroup ConstitutiveLawsApplication
 * @brief This utility computed the crack length in damage mechanics
 * @authors Alejandro Cornejo
 */
class KRATOS_API(CONSTITUTIVE_LAWS_APPLICATION) CalculateCrackLengthUtility
{
public:
    ///@name Type Definitions
    ///@{

    /// Pointer definition of CalculateCrackLengthUtility
    KRATOS_CLASS_POINTER_DEFINITION(CalculateCrackLengthUtility);

    /// The definition of the Voigt array type
    typedef array_1d<double, 6> BoundedVectorVoigtType;
    typedef array_1d<double, 3> BoundedVectorDimensionType;

    /// Definition of the zero tolerance
    static constexpr double tolerance = std::numeric_limits<double>::epsilon();

    ///@}
    ///@name Life Cycle
    ///@{

    CalculateCrackLengthUtility(
        ModelPart &rModelPart,
        const array_1d<double, 3> &rAdvancingDirection,
        const SizeType InitialElementId = 0);

    CalculateCrackLengthUtility(
        ModelPart &rModelPart,
        const array_1d<double, 3> &rAdvancingDirection);

    /// Destructor.
    virtual ~CalculateCrackLengthUtility() {}

    ///@}
    ///@name Operators
    ///@{

    ///@}
    ///@name Operations
    ///@{

protected:
    ///@name Protected static Member Variables
    ///@{

    ///@}
    ///@name Protected member Variables
    ///@{

    ///@}
    ///@name Protected Operators
    ///@{

    ///@}
    ///@name Protected Operations
    ///@{

    ///@}
    ///@name Protected  Access
    ///@{

    ///@}
    ///@name Protected Inquiry
    ///@{

    ///@}
    ///@name Protected LifeCycle
    ///@{

    ///@}
private:
    ///@name Static Member Variables
    ///@{

    ModelPart& mrModelPart;
    int mCrackTipElementId = 0;
    double mCrackLength    = 0;
    BoundedVectorDimensionType mAdvancingDirection = ZeroVector(3);
    std::vector<int> mArrayElementIds;

    ///@}
    ///@name Member Variables
    ///@{

    ///@}
    ///@name Private Operators
    ///@{

    ///@}
    ///@name Private Operations
    ///@{

protected:
    ///@name Protected static Member Variables
    ///@{

    ///@}
    ///@name Protected member Variables
    ///@{

    ///@}
    ///@name Protected Operators
    ///@{

    ///@}
    ///@name Protected Operations
    ///@{

    ///@}
    ///@name Protected  Access
    ///@{

    ///@}
    ///@name Protected Inquiry
    ///@{

    ///@}
    ///@name Protected LifeCycle
    ///@{

    ///@}

    /// Assignment operator.
    CalculateCrackLengthUtility &operator=(CalculateCrackLengthUtility const &rOther);
};
} // namespace Kratos.
#endif // KRATOS_CALCULATE_CRACK_LENGTH_UTILITY_H_INCLUDED  defined