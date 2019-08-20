/* -------------------------------------------------------------------------- *
 * OpenSim Moco: DeGrooteFregly2016Muscle.cpp                                 *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2017 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Christopher Dembia                                              *
 *                                                                            *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may    *
 * not use this file except in compliance with the License. You may obtain a  *
 * copy of the License at http://www.apache.org/licenses/LICENSE-2.0          *
 *                                                                            *
 * Unless required by applicable law or agreed to in writing, software        *
 * distributed under the License is distributed on an "AS IS" BASIS,          *
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.   *
 * See the License for the specific language governing permissions and        *
 * limitations under the License.                                             *
 * -------------------------------------------------------------------------- */

#include "DeGrooteFregly2016Muscle.h"

#include <OpenSim/Actuators/Millard2012EquilibriumMuscle.h>
#include <OpenSim/Actuators/Thelen2003Muscle.h>
#include <OpenSim/Simulation/Model/Model.h>

using namespace OpenSim;

const std::string DeGrooteFregly2016Muscle::STATE_ACTIVATION_NAME("activation");
const std::string DeGrooteFregly2016Muscle::STATE_NORMALIZED_TENDON_FORCE_NAME(
        "normalized_tendon_force");

void DeGrooteFregly2016Muscle::constructProperties() {
    //constructProperty_default_normalized_fiber_length(1.0);
    constructProperty_activation_time_constant(0.015);
    constructProperty_deactivation_time_constant(0.060);
    constructProperty_default_activation(0.5);
    constructProperty_default_normalized_tendon_force(0.5);

    constructProperty_active_force_width_scale(1.0);
    constructProperty_fiber_damping(0.01);
    constructProperty_tendon_strain_at_one_norm_force(0.049);

    constructProperty_ignore_passive_fiber_force(false);
}

void DeGrooteFregly2016Muscle::extendFinalizeFromProperties() {
    Super::extendFinalizeFromProperties();
    OPENSIM_THROW_IF_FRMOBJ(!getProperty_optimal_force().getValueIsDefault(),
            Exception,
            "The optimal_force property is ignored for this Force; "
            "use max_isometric_force instead.");

    //SimTK_ERRCHK2_ALWAYS(get_default_normalized_fiber_length() >= 0.2,
    //        "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
    //        "%s: default_normalized_fiber_length must be >= 0.2, but it is %g.",
    //        getName().c_str(), get_default_normalized_fiber_length());

    //SimTK_ERRCHK2_ALWAYS(get_default_normalized_fiber_length() <= 1.8,
    //        "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
    //        "%s: default_normalized_fiber_length must be <= 1.8, but it is %g.",
    //        getName().c_str(), get_default_normalized_fiber_length());

    SimTK_ERRCHK2_ALWAYS(get_activation_time_constant() > 0,
            "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
            "%s: activation_time_constant must be greater than zero, "
            "but it is %g.",
            getName().c_str(), get_activation_time_constant());

    SimTK_ERRCHK2_ALWAYS(get_deactivation_time_constant() > 0,
            "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
            "%s: deactivation_time_constant must be greater than zero, "
            "but it is %g.",
            getName().c_str(), get_deactivation_time_constant());

    SimTK_ERRCHK2_ALWAYS(get_default_activation() > 0,
            "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
            "%s: default_activation must be greater than zero, "
            "but it is %g.",
            getName().c_str(), get_default_activation());

    SimTK_ERRCHK2_ALWAYS(get_default_normalized_tendon_force() >= 0,
            "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
            "%s: default_normalized_tendon_force must be >= 0, but it is %g.",
            getName().c_str(), get_default_normalized_tendon_force());

    SimTK_ERRCHK2_ALWAYS(get_default_normalized_tendon_force() <= 5,
            "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
            "%s: default_normalized_tendon_force must be <= 5, but it is %g.",
            getName().c_str(), get_default_normalized_tendon_force());

    SimTK_ERRCHK2_ALWAYS(get_active_force_width_scale() >= 1,
            "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
            "%s: active_force_width_scale must be greater than or equal to "
            "1.0, "
            "but it is %g.",
            getName().c_str(), get_active_force_width_scale());

    SimTK_ERRCHK2_ALWAYS(get_fiber_damping() >= 0,
            "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
            "%s: fiber_damping must be greater than or equal to zero, "
            "but it is %g.",
            getName().c_str(), get_fiber_damping());

    SimTK_ERRCHK2_ALWAYS(get_tendon_strain_at_one_norm_force() > 0,
            "DeGrooteFregly2016Muscle::extendFinalizeFromProperties",
            "%s: tendon_strain_at_one_norm_force must be greater than zero, "
            "but it is %g.",
            getName().c_str(), get_tendon_strain_at_one_norm_force());

    OPENSIM_THROW_IF_FRMOBJ(
            get_pennation_angle_at_optimal() < 0 ||
                    get_pennation_angle_at_optimal() >
                            SimTK::Pi / 2.0 - SimTK::SignificantReal,
            InvalidPropertyValue,
            getProperty_pennation_angle_at_optimal().getName(),
            "Pennation angle at optimal fiber length must be in the range [0, "
            "Pi/2).");

    using SimTK::square;
    const auto normFiberWidth = sin(get_pennation_angle_at_optimal());
    m_fiberWidth = get_optimal_fiber_length() * normFiberWidth;
    m_squareFiberWidth = square(m_fiberWidth);
    m_maxContractionVelocityInMetersPerSecond =
            get_max_contraction_velocity() * get_optimal_fiber_length();
    m_kT = log((1.0 + c3) / c1) /
           (1.0 + get_tendon_strain_at_one_norm_force() - c2);
}

void DeGrooteFregly2016Muscle::extendAddToSystem(
        SimTK::MultibodySystem& system) const {
    Super::extendAddToSystem(system);
    if (!get_ignore_activation_dynamics()) {
        addStateVariable(STATE_ACTIVATION_NAME, SimTK::Stage::Dynamics);
    }
    if (!get_ignore_tendon_compliance()) {
        addStateVariable(
                STATE_NORMALIZED_TENDON_FORCE_NAME, SimTK::Stage::Dynamics);
        addDiscreteVariable(
                "implicitderiv_" + STATE_NORMALIZED_TENDON_FORCE_NAME,
                SimTK::Stage::Dynamics);
        addCacheVariable(
                "implicitresidual_" + STATE_NORMALIZED_TENDON_FORCE_NAME,
                double(0), SimTK::Stage::Dynamics);
    }
}

void DeGrooteFregly2016Muscle::extendInitStateFromProperties(
        SimTK::State& s) const {
    Super::extendInitStateFromProperties(s);
    if (!get_ignore_activation_dynamics()) {
        setStateVariableValue(
                s, STATE_ACTIVATION_NAME, get_default_activation());
    }
    if (!get_ignore_tendon_compliance()) {
        setStateVariableValue(s, STATE_NORMALIZED_TENDON_FORCE_NAME,
                get_default_normalized_tendon_force());
    }
}

void DeGrooteFregly2016Muscle::extendSetPropertiesFromState(
        const SimTK::State& s) {
    Super::extendSetPropertiesFromState(s);
    if (!get_ignore_activation_dynamics()) {
        set_default_activation(getStateVariableValue(s, STATE_ACTIVATION_NAME));
    }
    if (!get_ignore_tendon_compliance()) {
        set_default_normalized_tendon_force(
                getStateVariableValue(s, STATE_NORMALIZED_TENDON_FORCE_NAME));
    }
}

void DeGrooteFregly2016Muscle::computeStateVariableDerivatives(
        const SimTK::State& s) const {
    if (!get_ignore_activation_dynamics()) {
        const auto& activation = getActivation(s);
        const auto& excitation = getControl(s);
        static const double actTimeConst = get_activation_time_constant();
        static const double deactTimeConst = get_deactivation_time_constant();
        static const double tanhSteepness = 0.1;
        //     f = 0.5 tanh(b(e - a))
        //     z = 0.5 + 1.5a
        // da/dt = [(f + 0.5)/(tau_a * z) + (-f + 0.5)*z/tau_d] * (e - a)
        const SimTK::Real timeConstFactor = 0.5 + 1.5 * activation;
        const SimTK::Real tempAct = 1.0 / (actTimeConst * timeConstFactor);
        const SimTK::Real tempDeact = timeConstFactor / deactTimeConst;
        const SimTK::Real f =
                0.5 * tanh(tanhSteepness * (excitation - activation));
        const SimTK::Real timeConst =
                tempAct * (f + 0.5) + tempDeact * (-f + 0.5);
        const SimTK::Real derivative = timeConst * (excitation - activation);
        setStateVariableDerivativeValue(s, STATE_ACTIVATION_NAME, derivative);
    }
    if (!get_ignore_tendon_compliance()) {
        const auto& derivNormTendonForce = getDiscreteVariableValue(
                s, "implicitderiv_" + STATE_NORMALIZED_TENDON_FORCE_NAME);
        setStateVariableDerivativeValue(
                s, STATE_NORMALIZED_TENDON_FORCE_NAME, derivNormTendonForce);
    }
}

double DeGrooteFregly2016Muscle::computeActuation(const SimTK::State& s) const {
    const auto& mdi = getMuscleDynamicsInfo(s);
    return mdi.tendonForce;
}

void DeGrooteFregly2016Muscle::calcMuscleLengthInfo(
        const SimTK::State& s, MuscleLengthInfo& mli) const {

    // Tendon.
    // -------
    if (get_ignore_tendon_compliance()) {
        mli.normTendonLength = 1.0;
    } else {
        const auto& normTendonForce =
                getStateVariableValue(s, STATE_NORMALIZED_TENDON_FORCE_NAME);
        mli.normTendonLength =
                calcTendonForceLengthInverseCurve(normTendonForce);
    }
    mli.tendonStrain = mli.normTendonLength - 1.0;
    mli.tendonLength = get_tendon_slack_length() * mli.normTendonLength;

    const auto& MTULength = getLength(s);
    if (mli.tendonLength < get_tendon_slack_length() && getPrintWarnings()) {
        // TODO the Millard model sets fiber velocity to zero when the
        //       tendon is buckling, but this may create a discontinuity.
        std::cout << "Warning: DeGrooteFregly2016Muscle '" << getName()
                  << "' is buckling (length < tendon_slack_length) at time "
                  << s.getTime() << " s." << std::endl;
    }

    // Fiber.
    // ------
    mli.fiberLengthAlongTendon = MTULength - mli.tendonLength;
    mli.fiberLength = sqrt(
            SimTK::square(mli.fiberLengthAlongTendon) + m_squareFiberWidth);
    mli.normFiberLength = mli.fiberLength / get_optimal_fiber_length();

    // Pennation.
    // ----------
    mli.cosPennationAngle = mli.fiberLengthAlongTendon / mli.fiberLength;
    mli.sinPennationAngle = m_fiberWidth / mli.fiberLength;
    mli.pennationAngle = asin(mli.sinPennationAngle);

    // Multipliers.
    // ------------
    mli.fiberPassiveForceLengthMultiplier =
            calcPassiveForceMultiplier(mli.normFiberLength);
    mli.fiberActiveForceLengthMultiplier =
            calcActiveForceLengthMultiplier(mli.normFiberLength);
}

void DeGrooteFregly2016Muscle::calcFiberVelocityInfo(
        const SimTK::State& s, FiberVelocityInfo& fvi) const {
    const auto& mli = getMuscleLengthInfo(s);
    const auto& MTUVel = getLengtheningSpeed(s);

    if (get_ignore_tendon_compliance()) {
        // For a rigid tendon:
        // lMT = lT + lM cos(alpha) -> differentiate:
        // vMT = vM cos(alpha) - lM alphaDot sin(alpha) (1)
        // w = lM sin(alpha) -> differentiate:
        // 0 = lMdot sin(alpha) + lM alphaDot cos(alpha) ->
        // alphaDot = -vM sin(alpha) / (lM cos(alpha)) -> plug into (1)
        // vMT = vM cos(alpha) + vM sin^2(alpha) / cos(alpha)
        // vMT = vM (cos^2(alpha) + sin^2(alpha)) / cos(alpha)
        // vM = vMT cos(alpha)
        fvi.tendonVelocity = 0;
        fvi.normTendonVelocity = 0;
        fvi.fiberVelocity = MTUVel * mli.cosPennationAngle;
        fvi.fiberVelocityAlongTendon = MTUVel;
    } else {
        // From DeGroote et al. 2016 supplementary data.
        const auto& derivNormTendonForce = getDiscreteVariableValue(
                s, "implicitderiv_" + STATE_NORMALIZED_TENDON_FORCE_NAME);
        fvi.normTendonVelocity = calcDerivativeTendonForceLengthInverseCurve(
                derivNormTendonForce, mli.normTendonLength);
        fvi.tendonVelocity = get_tendon_slack_length() * fvi.normTendonVelocity;
        fvi.fiberVelocityAlongTendon = MTUVel - fvi.tendonVelocity;
        fvi.fiberVelocity = 
                fvi.fiberVelocityAlongTendon * mli.cosPennationAngle; 
    }

    fvi.normFiberVelocity =
            fvi.fiberVelocity / m_maxContractionVelocityInMetersPerSecond;
    const SimTK::Real tanPennationAngle =
            m_fiberWidth / mli.fiberLengthAlongTendon;
    fvi.pennationAngularVelocity =
            -fvi.fiberVelocity / mli.fiberLength * tanPennationAngle;

    fvi.fiberForceVelocityMultiplier =
            calcForceVelocityMultiplier(fvi.normFiberVelocity);
}

void DeGrooteFregly2016Muscle::calcMuscleDynamicsInfo(
        const SimTK::State& s, MuscleDynamicsInfo& mdi) const {

    mdi.activation = getActivation(s);
    const auto& mli = getMuscleLengthInfo(s);
    const auto& fvi = getFiberVelocityInfo(s);

    SimTK::Vec4 fiberForceVec = calcFiberForce(mdi.activation,
            mli.fiberActiveForceLengthMultiplier,
            fvi.fiberForceVelocityMultiplier,
            mli.fiberPassiveForceLengthMultiplier, fvi.normFiberVelocity);

    double totalFiberForce = fiberForceVec[0];
    const double activeFiberForce = fiberForceVec[1];
    const double conPassiveFiberForce = fiberForceVec[2];
    double nonConPassiveFiberForce = fiberForceVec[3];
    double passiveFiberForce = conPassiveFiberForce + nonConPassiveFiberForce;

    // When using a rigid tendon, avoid generating compressive fiber forces by 
    // saturating the damping force produced by the parallel element. 
    // Based on Millard2012EquilibriumMuscle::calcMuscleDynamicsInfo().
    if (get_ignore_tendon_compliance()) {
        if (totalFiberForce < 0) {
            totalFiberForce = 0.0;
            nonConPassiveFiberForce = -activeFiberForce - conPassiveFiberForce;
            passiveFiberForce = conPassiveFiberForce + nonConPassiveFiberForce;
        }
    }

    // Compute force entries.
    // ----------------------
    const auto FMo = get_max_isometric_force();
    mdi.fiberForce = totalFiberForce;
    mdi.activeFiberForce = activeFiberForce;
    mdi.passiveFiberForce = passiveFiberForce;
    mdi.normFiberForce = mdi.fiberForce / FMo;
    mdi.fiberForceAlongTendon = mdi.fiberForce * mli.cosPennationAngle;

    if (get_ignore_tendon_compliance()) {
        mdi.normTendonForce = mdi.normFiberForce * mli.cosPennationAngle;
        mdi.tendonForce = mdi.fiberForceAlongTendon;
    } else {
        mdi.normTendonForce =
                getStateVariableValue(s, STATE_NORMALIZED_TENDON_FORCE_NAME);
        mdi.tendonForce = FMo * mdi.normTendonForce;
    }

    // Compute stiffness entries.
    // --------------------------
    mdi.fiberStiffness = calcFiberStiffness(mdi.activation, mli.normFiberLength,
            fvi.fiberForceVelocityMultiplier);
    const auto& Dpenn_DlM =
            calc_DPennationAngle_DFiberLength(mli.fiberLength);
    const auto& dFMAT_dlM =
            calc_DFiberForceAT_DFiberLength(mdi.fiberForce, mdi.fiberStiffness,
                    mli.sinPennationAngle, mli.cosPennationAngle, Dpenn_DlM);
    const auto& dFMAT_dlMAT = calc_DFiberForceAT_DFiberLengthAT(mli.fiberLength,
            dFMAT_dlM, mli.sinPennationAngle, mli.cosPennationAngle, Dpenn_DlM);
    mdi.fiberStiffnessAlongTendon = dFMAT_dlMAT;
    if (!get_ignore_tendon_compliance()) {
        const auto& dFT_dlT =
                (get_max_isometric_force() / get_tendon_slack_length()) *
                calcTendonForceMultiplierDerivative(mli.normTendonLength);
        // Compute stiffness of whole musculotendon actuator. Based on 
        // Millard2012EquilibriumMuscle.
        SimTK::Real Ke = 0;
        if (abs(dFMAT_dlMAT*dFT_dlT) > 0.0 
            && abs(dFMAT_dlMAT+dFT_dlT) > SimTK::SignificantReal) {
            Ke = (dFMAT_dlMAT * dFT_dlT) / (dFMAT_dlMAT + dFT_dlT);
        }
        mdi.tendonStiffness = dFT_dlT;
        mdi.muscleStiffness = Ke;
    } else {
        mdi.tendonStiffness = SimTK::Infinity;
        mdi.muscleStiffness = mdi.fiberStiffnessAlongTendon;
    }

    // Compute power entries.
    // ----------------------
    mdi.fiberActivePower = 
            -(mdi.activeFiberForce+nonConPassiveFiberForce)*fvi.fiberVelocity;
    mdi.fiberPassivePower = -conPassiveFiberForce*fvi.fiberVelocity;
    mdi.tendonPower = -mdi.tendonForce*fvi.tendonVelocity;
    mdi.musclePower = -mdi.tendonForce*getLengtheningSpeed(s);

}

void DeGrooteFregly2016Muscle::calcMusclePotentialEnergyInfo(
        const SimTK::State& s, MusclePotentialEnergyInfo& mpei) const {
    
    // Based on Millard2012EquilibriumMuscle::calcMusclePotentialEnergyInfo().

    const MuscleLengthInfo& mli = getMuscleLengthInfo(s);

    // Fiber potential energy.
    // -----------------------
    mpei.fiberPotentialEnergy =
            calcPassiveForceMultiplierIntegral(mli.normFiberLength) *
            (get_optimal_fiber_length() * get_max_isometric_force());

    // Tendon potential energy.
    // ------------------------
    mpei.tendonPotentialEnergy = 0;
    if (!get_ignore_tendon_compliance()) {
        mpei.tendonPotentialEnergy =
                calcTendonForceMultiplierIntegral(mli.normTendonLength) *
                (get_tendon_slack_length() * get_max_isometric_force());
    }

    // Total potential energy.
    // -----------------------
    mpei.musclePotentialEnergy =
            mpei.fiberPotentialEnergy + mpei.tendonPotentialEnergy;
}

void DeGrooteFregly2016Muscle::computeInitialFiberEquilibrium(
        SimTK::State& /*s*/) const {
    if (get_ignore_tendon_compliance()) return;
}

double DeGrooteFregly2016Muscle::getImplicitResidualNormalizedTendonForce(
        const SimTK::State& s) const {
    // Recompute residual if cache is invalid.
    if (!isCacheVariableValid(
                s, "implicitresidual_" + STATE_NORMALIZED_TENDON_FORCE_NAME)) {
        // Compute muscle-tendon equilibrium residual value to update the 
        // cache variable.
        setCacheVariableValue(s, 
            "implicitresidual_" + STATE_NORMALIZED_TENDON_FORCE_NAME, 
            calcEquilibriumResidual(s));
        markCacheVariableValid(
                s, "implicitresidual_" + STATE_NORMALIZED_TENDON_FORCE_NAME);
    }
    
    return getCacheVariableValue<double>(
            s, "implicitresidual_" + STATE_NORMALIZED_TENDON_FORCE_NAME);
}

DataTable DeGrooteFregly2016Muscle::exportFiberLengthCurvesToTable(
        const SimTK::Vector& normFiberLengths) const {
    SimTK::Vector def;
    const SimTK::Vector* x = nullptr;
    if (normFiberLengths.nrow()) {
        x = &normFiberLengths;
    } else {
        def = createVectorLinspace(
                200, m_minNormFiberLength, m_maxNormFiberLength);
        x = &def;
    }

    DataTable table;
    table.setColumnLabels(
            {"active_force_length_multiplier", "passive_force_multiplier"});
    SimTK::RowVector row(2);
    for (int irow = 0; irow < x->nrow(); ++irow) {
        const auto& normFiberLength = x->get(irow);
        row[0] = calcActiveForceLengthMultiplier(normFiberLength);
        row[1] = calcPassiveForceMultiplier(normFiberLength);
        table.appendRow(normFiberLength, row);
    }
    return table;
}

DataTable DeGrooteFregly2016Muscle::exportTendonForceMultiplierToTable(
        const SimTK::Vector& normTendonLengths) const {
    SimTK::Vector def;
    const SimTK::Vector* x = nullptr;
    if (normTendonLengths.nrow()) {
        x = &normTendonLengths;
    } else {
        // Evaluate the inverse of the tendon curve at y = 1.
        def = createVectorLinspace(
                200, 0.95, 1.0 + get_tendon_strain_at_one_norm_force());
        x = &def;
    }

    DataTable table;
    table.setColumnLabels({"tendon_force_multiplier"});
    SimTK::RowVector row(1);
    for (int irow = 0; irow < x->nrow(); ++irow) {
        const auto& normTendonLength = x->get(irow);
        row[0] = calcTendonForceMultiplier(normTendonLength);
        table.appendRow(normTendonLength, row);
    }
    return table;
}

DataTable DeGrooteFregly2016Muscle::exportFiberVelocityMultiplierToTable(
        const SimTK::Vector& normFiberVelocities) const {
    SimTK::Vector def;
    const SimTK::Vector* x = nullptr;
    if (normFiberVelocities.nrow()) {
        x = &normFiberVelocities;
    } else {
        def = createVectorLinspace(200, -1.1, 1.1);
        x = &def;
    }

    DataTable table;
    table.setColumnLabels({"force_velocity_multiplier"});
    SimTK::RowVector row(1);
    for (int irow = 0; irow < x->nrow(); ++irow) {
        const auto& normFiberVelocity = x->get(irow);
        row[0] = calcForceVelocityMultiplier(normFiberVelocity);
        table.appendRow(normFiberVelocity, row);
    }
    return table;
}

void DeGrooteFregly2016Muscle::printCurvesToSTOFiles(
        const std::string& directory) const {
    std::string prefix =
            directory + SimTK::Pathname::getPathSeparator() + getName();
    writeTableToFile(exportFiberLengthCurvesToTable(),
            prefix + "_fiber_length_curves.sto");
    writeTableToFile(exportFiberVelocityMultiplierToTable(),
            prefix + "_fiber_velocity_multiplier.sto");
    writeTableToFile(exportTendonForceMultiplierToTable(),
            prefix + "_tendon_force_multiplier.sto");
}

void DeGrooteFregly2016Muscle::replaceMuscles(
        Model& model, bool allowUnsupportedMuscles) {

    model.finalizeConnections();

    // Create path actuators from muscle properties and add to the model. Save
    // a list of pointers of the muscles to delete.
    std::vector<Muscle*> musclesToDelete;
    auto& muscleSet = model.updMuscles();
    for (int im = 0; im < muscleSet.getSize(); ++im) {
        auto& muscBase = muscleSet.get(im);

        // pre-emptively create a default DeGrooteFregly2016Muscle
        // (not ideal to do this)
        auto actu = OpenSim::make_unique<DeGrooteFregly2016Muscle>();

        // peform muscle-model-specific mappings or throw exception if muscle
        // not supported
        if (auto musc = dynamic_cast<Millard2012EquilibriumMuscle*>(
                    &muscBase)) {

            // TODO: There is a bug in Millard2012EquilibriumMuscle
            // where the default fiber length is 0.1 by default instead
            // of optimal fiber length.
            //if (!SimTK::isNumericallyEqual(
            //            musc->get_default_fiber_length(), 0.1)) {
            //    actu->set_default_normalized_fiber_length(
            //            musc->get_default_fiber_length() /
            //            musc->get_optimal_fiber_length());
            //}
            // TODO how to set normalized tendon force default?
            actu->set_default_normalized_tendon_force(0.5);
            actu->set_default_activation(musc->get_default_activation());
            actu->set_activation_time_constant(
                    musc->get_activation_time_constant());
            actu->set_deactivation_time_constant(
                    musc->get_deactivation_time_constant());

            // TODO
            actu->set_fiber_damping(0);
            // actu->set_fiber_damping(musc->get_fiber_damping());
            actu->set_tendon_strain_at_one_norm_force(
                    musc->get_TendonForceLengthCurve()
                            .get_strain_at_one_norm_force());

        } else if (auto musc = dynamic_cast<Thelen2003Muscle*>(&muscBase)) {

            //actu->set_default_normalized_fiber_length(
            //        musc->get_default_fiber_length() /
            //        musc->get_optimal_fiber_length());
            // TODO how to set normalized tendon force default?
            actu->set_default_normalized_tendon_force(0.5);
            actu->set_default_activation(musc->getDefaultActivation());
            actu->set_activation_time_constant(
                    musc->get_activation_time_constant());
            actu->set_deactivation_time_constant(
                    musc->get_deactivation_time_constant());

            // TODO: currently needs to be hardcoded for 
            // Thelen2003Muscle as damping is not a property
            actu->set_fiber_damping(0);
            actu->set_tendon_strain_at_one_norm_force(
                    musc->get_FmaxTendonStrain());

        } else {
            OPENSIM_THROW_IF(!allowUnsupportedMuscles, Exception,
                    format("Muscle '%s' of type %s is unsupported and "
                           "allowUnsupportedMuscles=false.",
                            muscBase.getName(),
                            muscBase.getConcreteClassName()));
            continue;
        }

        // Perform all the common mappings at base class level (OpenSim::Muscle)

        actu->setName(muscBase.getName());
        muscBase.setName(muscBase.getName() + "_delete");
        actu->setMinControl(muscBase.getMinControl());
        actu->setMaxControl(muscBase.getMaxControl());

        actu->setMaxIsometricForce(muscBase.getMaxIsometricForce());
        actu->setOptimalFiberLength(muscBase.getOptimalFiberLength());
        actu->setTendonSlackLength(muscBase.getTendonSlackLength());
        actu->setPennationAngleAtOptimalFiberLength(
                muscBase.getPennationAngleAtOptimalFiberLength());
        actu->setMaxContractionVelocity(muscBase.getMaxContractionVelocity());
        actu->set_ignore_tendon_compliance(
                muscBase.get_ignore_tendon_compliance());
        actu->set_ignore_activation_dynamics(
                muscBase.get_ignore_activation_dynamics());

        const auto& pathPointSet = muscBase.getGeometryPath().getPathPointSet();
        auto& geomPath = actu->updGeometryPath();
        for (int ipp = 0; ipp < pathPointSet.getSize(); ++ipp) {
            auto* pathPoint = pathPointSet.get(ipp).clone();
            const auto& socketNames = pathPoint->getSocketNames();
            for (const auto& socketName : socketNames) {
                pathPoint->updSocket(socketName)
                        .connect(pathPointSet.get(ipp)
                                         .getSocket(socketName)
                                         .getConnecteeAsObject());
            }
            geomPath.updPathPointSet().adoptAndAppend(pathPoint);
        }
        std::string actname = actu->getName();
        model.addForce(actu.release());

        // Workaround for a bug in prependComponentPathToConnecteePath().
        for (auto& comp : model.updComponentList()) {
            const auto& socketNames = comp.getSocketNames();
            for (const auto& socketName : socketNames) {
                auto& socket = comp.updSocket(socketName);
                auto connecteePath = socket.getConnecteePath();
                std::string prefix = "/forceset/" + actname;
                if (startsWith(connecteePath, prefix)) {
                    connecteePath = connecteePath.substr(prefix.length());
                    socket.setConnecteePath(connecteePath);
                }
            }
        }
        musclesToDelete.push_back(&muscBase);
    }

    // Delete the muscles.
    for (const auto* musc : musclesToDelete) {
        int index = model.getForceSet().getIndex(musc, 0);
        OPENSIM_THROW_IF(index == -1, Exception,
                format("Muscle with name %s not found in ForceSet.",
                        musc->getName()));
        bool success = model.updForceSet().remove(index);
        OPENSIM_THROW_IF(!success, Exception,
                format("Attempt to remove muscle with "
                       "name %s was unsuccessful.",
                        musc->getName()));
    }

    model.finalizeFromProperties();
    model.finalizeConnections();
}