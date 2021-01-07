//    |  /           |
//    ' /   __| _` | __|  _ \   __|
//    . \  |   (   | |   (   |\__ `
//   _|\_\_|  \__,_|\__|\___/ ____/
//                   Multi-Physics
//
//  License:         BSD License
//                   Kratos default license: kratos/license.txt
//
//  Main authors:    Suneth Warnakulasuriya
//

#if !defined(KRATOS_FLUID_ADJOINT_UTILITIES_H)
#define KRATOS_FLUID_ADJOINT_UTILITIES_H

// System includes

// External includes

// Project includes
#include "geometries/geometry.h"
#include "includes/node.h"
#include "includes/ublas_interface.h"
#include "utilities/coordinate_transformation_utilities.h"

// Application includes
#include "custom_utilities/fluid_calculation_utilities.h"

namespace Kratos
{
///@addtogroup FluidDynamicsApplication
///@{

///@name Kratos classes
///@{

template<unsigned int TDim, unsigned int TBlockSize = TDim + 1>
class KRATOS_API(FLUID_DYNAMICS_APPLICATION) FluidAdjointUtilities
{
public:
    ///@name Type Definitions
    ///@{

    using NodeType = Node<3>;

    using IndexType = std::size_t;

    using GeometryType = Geometry<NodeType>;

    ///@}
    ///@name Static Operations
    ///@{

    /**
     * @brief
     *
     * @param rOutput
     * @param rResidualDerivatives
     * @param rGeometry
     */
    static void CalculateRotatedSlipConditionAppliedSlipVariableDerivatives(
        Matrix& rOutput,
        const Matrix& rResidualDerivatives,
        const GeometryType& rGeometry)
    {
        if (rOutput.size1() != rResidualDerivatives.size1() ||
            rOutput.size2() != rResidualDerivatives.size2()) {
            rOutput.resize(rResidualDerivatives.size1(),
                           rResidualDerivatives.size2(), false);
        }

        rOutput.clear();

        const IndexType number_of_nodes = rGeometry.PointsNumber();

        // add residual derivative contributions
        for (IndexType a = 0; a < number_of_nodes; ++a) {
            const auto& r_node = rGeometry[a];
            const IndexType block_index = a * TBlockSize;
            if (r_node.Is(SLIP)) {
                AddNodalRotationDerivatives(rOutput, rResidualDerivatives, block_index, r_node);
                AddNodalApplySlipConditionDerivatives(rOutput, block_index, r_node);
            } else {
                AddNodalResidualDerivatives(rOutput, rResidualDerivatives, block_index);
            }
        }
    }

    /**
     * @brief
     *
     * @param rOutput
     * @param rResidualDerivatives
     * @param rGeometry
     */
    static void CalculateRotatedSlipConditionAppliedNonSlipVariableDerivatives(
        Matrix& rOutput,
        const Matrix& rResidualDerivatives,
        const GeometryType& rGeometry)
    {
        if (rOutput.size1() != rResidualDerivatives.size1() ||
            rOutput.size2() != rResidualDerivatives.size2()) {
            rOutput.resize(rResidualDerivatives.size1(),
                           rResidualDerivatives.size2(), false);
        }

        rOutput.clear();

        const IndexType number_of_nodes = rGeometry.PointsNumber();

        // add residual derivative contributions
        for (IndexType a = 0; a < number_of_nodes; ++a) {
            const auto& r_node = rGeometry[a];
            const IndexType block_index = a * TBlockSize;
            if (r_node.Is(SLIP)) {
                AddNodalRotationDerivatives(rOutput, rResidualDerivatives, block_index, r_node);
                // since slip condition is only based on first derivative
                // variable, make the column zero for all derivatives
                ClearNodalResidualDerivatives(rOutput, block_index);
            } else {
                AddNodalResidualDerivatives(rOutput, rResidualDerivatives, block_index);
            }
        }
    }

    /**
     * @brief
     *
     * @param rOutput
     * @param rResidualDerivatives
     * @param NodeStartIndex
     * @param rNode
     */
    static void AddNodalRotationDerivatives(
        Matrix& rOutput,
        const Matrix& rResidualDerivatives,
        const IndexType NodeStartIndex,
        const NodeType& rNode)
    {
        KRATOS_TRY

        using coordinate_transformation_utils =
            CoordinateTransformationUtils<Matrix, Vector, double>;

        BoundedVector<double, TDim> residual_derivative, aux_vector;
        BoundedMatrix<double, TDim, TDim> rotation_matrix;
        coordinate_transformation_utils::LocalRotationOperatorPure(rotation_matrix, rNode);

        // add rotated residual derivative contributions
        for (IndexType c = 0; c < rResidualDerivatives.size1(); ++c) {
            // get the residual derivative relevant for node
            FluidCalculationUtilities::ReadSubVector<TDim>(
                residual_derivative, row(rResidualDerivatives, c), NodeStartIndex);

            // rotate residual derivative
            noalias(aux_vector) = prod(rotation_matrix, residual_derivative);

            // add rotated residual derivative to local matrix
            FluidCalculationUtilities::AddSubVector<TDim>(rOutput, aux_vector, c, NodeStartIndex);

            // add rest of the equation derivatives
            for (IndexType a = TDim; a < TBlockSize; ++a) {
                rOutput(c, NodeStartIndex + a) +=
                    rResidualDerivatives(c, NodeStartIndex + a);
            }
        }

        KRATOS_CATCH("");
    }

    /**
     * @brief
     *
     * @param rOutput
     * @param NodeStartIndex
     * @param rNode
     */
    static void AddNodalApplySlipConditionDerivatives(
        Matrix& rOutput,
        const IndexType NodeStartIndex,
        const NodeType& rNode)
    {
        KRATOS_TRY

        // Apply slip condition in primal scheme makes first momentum dof
        // fixed, making the velocity in the normal direction as rhs.

        // first clear the residual derivative
        ClearNodalResidualDerivatives(rOutput, NodeStartIndex);

        auto normal = rNode.FastGetSolutionStepValue(NORMAL);
        normal /= norm_2(normal);

        for (IndexType i = 0; i < TDim; ++i) {
            rOutput(NodeStartIndex + i, NodeStartIndex) -= normal[i];
        }

        KRATOS_CATCH("");
    }

    /**
     * @brief
     *
     * @param rOutput
     * @param rResidualDerivatives
     * @param NodeStartIndex
     */
    static void AddNodalResidualDerivatives(
        Matrix& rOutput,
        const Matrix& rResidualDerivatives,
        const IndexType NodeStartIndex)
    {
        KRATOS_TRY

        // add non-rotated residual derivative contributions
        for (IndexType c = 0; c < rResidualDerivatives.size1(); ++c) {
            for (IndexType i = 0; i < TBlockSize; ++i) {
                rOutput(c, NodeStartIndex + i) +=
                    rResidualDerivatives(c, NodeStartIndex + i);
            }
        }

        KRATOS_CATCH("");
    }

    static void ClearNodalResidualDerivatives(
        Matrix& rOutput,
        const IndexType ResidualIndex)
    {
        for (IndexType c = 0; c < rOutput.size1(); ++c) {
            rOutput(c, ResidualIndex) = 0.0;
        }
    }

    ///@}
};

///@}

///@}

} // namespace Kratos

#endif // KRATOS_FLUID_ADJOINT_UTILITIES_H