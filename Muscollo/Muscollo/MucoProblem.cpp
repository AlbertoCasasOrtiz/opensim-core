/* -------------------------------------------------------------------------- *
 * OpenSim Muscollo: MucoProblem.cpp                                          *
 * -------------------------------------------------------------------------- *
 * Copyright (c) 2017 Stanford University and the Authors                     *
 *                                                                            *
 * Author(s): Christopher Dembia, Nicholas Bianco                             *
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

#include "MucoProblem.h"
#include "MuscolloUtilities.h"

#include <simbody/internal/Constraint.h>

using namespace OpenSim;


// ============================================================================
// MucoVariableInfo
// ============================================================================

MucoVariableInfo::MucoVariableInfo() {
    constructProperties();
}

MucoVariableInfo::MucoVariableInfo(const std::string& name,
        const MucoBounds& bounds, const MucoInitialBounds& initial,
        const MucoFinalBounds& final) : MucoVariableInfo() {
    setName(name);
    set_bounds(bounds.getAsArray());
    set_initial_bounds(initial.getAsArray());
    set_final_bounds(final.getAsArray());
    validate();
}

void MucoVariableInfo::validate() const {
    const auto& n = getName();
    const auto b = getBounds();
    const auto ib = getInitialBounds();
    const auto fb = getFinalBounds();

    OPENSIM_THROW_IF(ib.isSet() && ib.getLower() < b.getLower(), Exception,
            format("For variable %s, expected "
                    "[initial value lower bound] >= [lower bound], but "
                    "intial value lower bound=%g, lower bound=%g.",
            n, ib.getLower(), b.getLower()));
    OPENSIM_THROW_IF(fb.isSet() && fb.getLower() < b.getLower(), Exception,
            format("For variable %s, expected "
                    "[final value lower bound] >= [lower bound], but "
                    "final value lower bound=%g, lower bound=%g.",
            n, fb.getLower(), b.getLower()));
    OPENSIM_THROW_IF(ib.isSet() && ib.getUpper() > b.getUpper(), Exception,
            format("For variable %s, expected "
                    "[initial value upper bound] >= [upper bound], but "
                    "initial value upper bound=%g, upper bound=%g.",
            n, ib.getUpper(), b.getUpper()));
    OPENSIM_THROW_IF(fb.isSet() && fb.getUpper() > b.getUpper(), Exception,
            format("For variable %s, expected "
                    "[final value upper bound] >= [upper bound], but "
                    "final value upper bound=%g, upper bound=%g.",
            n, fb.getUpper(), b.getUpper()));
}

void MucoVariableInfo::printDescription(std::ostream& stream) const {
    const auto bounds = getBounds();
    stream << getName() << ". bounds: ";
    bounds.printDescription(stream);
    const auto initial = getInitialBounds();
    if (initial.isSet()) {
        stream << " initial: ";
        initial.printDescription(stream);
    }
    const auto final = getFinalBounds();
    if (final.isSet()) {
        stream << " final: ";
        final.printDescription(stream);
    }
    stream << std::endl;
}

void MucoVariableInfo::constructProperties() {
    constructProperty_bounds();
    constructProperty_initial_bounds();
    constructProperty_final_bounds();
}


// ============================================================================
// MucoPhase
// ============================================================================
MucoPhase::MucoPhase() {
    constructProperties();
}
void MucoPhase::constructProperties() {
    constructProperty_model(Model());
    constructProperty_time_initial_bounds();
    constructProperty_time_final_bounds();
    constructProperty_state_infos();
    constructProperty_control_infos();
    constructProperty_parameters();
    constructProperty_costs();
    constructProperty_path_constraints();
    constructProperty_multibody_constraint_bounds({0, 0.0});
    constructProperty_multiplier_bounds({-1000.0, 1000.0});
}
void MucoPhase::setModel(const Model& model) {
    set_model(model);
}
void MucoPhase::setTimeBounds(const MucoInitialBounds& initial,
        const MucoFinalBounds& final) {
    set_time_initial_bounds(initial.getAsArray());
    set_time_final_bounds(final.getAsArray());
}
void MucoPhase::setStateInfo(const std::string& name, const MucoBounds& bounds,
        const MucoInitialBounds& initial, const MucoFinalBounds& final) {
    int idx = getProperty_state_infos().findIndexForName(name);

    MucoVariableInfo info(name, bounds, initial, final);
    if (idx == -1) append_state_infos(info);
    else           upd_state_infos(idx) = info;
}
void MucoPhase::setControlInfo(const std::string& name,
        const MucoBounds& bounds,
        const MucoInitialBounds& initial, const MucoFinalBounds& final) {
    int idx = getProperty_control_infos().findIndexForName(name);

    MucoVariableInfo info(name, bounds, initial, final);
    if (idx == -1) append_control_infos(info);
    else           upd_control_infos(idx) = info;
}
void MucoPhase::addParameter(const MucoParameter& parameter) {
    OPENSIM_THROW_IF_FRMOBJ(parameter.getName().empty(), Exception,
        "Cannot add a parameter if it does not have a name (use setName()).");
    int idx = getProperty_parameters().findIndexForName(parameter.getName());
    OPENSIM_THROW_IF_FRMOBJ(idx != -1, Exception,
        "A parameter with name '" + parameter.getName() + "' already exists.");
    append_parameters(parameter);
}
void MucoPhase::addCost(const MucoCost& cost) {
    OPENSIM_THROW_IF_FRMOBJ(cost.getName().empty(), Exception,
        "Cannot add a cost if it does not have a name (use setName()).");
    int idx = getProperty_costs().findIndexForName(cost.getName());
    OPENSIM_THROW_IF_FRMOBJ(idx != -1, Exception,
        "A cost with name '" + cost.getName() + "' already exists.");
    append_costs(cost);
}
void MucoPhase::addPathConstraint(const MucoPathConstraint& constraint) {
    OPENSIM_THROW_IF_FRMOBJ(constraint.getName().empty(), Exception,
        "Cannot add a constraint if it does not have a name (use setName()).");
    int idx = 
        getProperty_path_constraints().findIndexForName(constraint.getName());
    OPENSIM_THROW_IF_FRMOBJ(idx != -1, Exception,
        "A constraint with name '" + constraint.getName() + "' already "
        "exists.");
    for (int i = 0; i < getProperty_path_constraints().size(); ++i) {
        OPENSIM_THROW_IF_FRMOBJ(get_path_constraints(i).getPathConstraintIndex() 
            != constraint.getPathConstraintIndex(), Exception, "A constraint "
            "with index '" + std::to_string(constraint.getPathConstraintIndex()) 
            + "' already exists.");
    }
    append_path_constraints(constraint);
}
MucoInitialBounds MucoPhase::getTimeInitialBounds() const {
    return MucoInitialBounds(getProperty_time_initial_bounds());
}
MucoFinalBounds MucoPhase::getTimeFinalBounds() const {
    return MucoFinalBounds(getProperty_time_final_bounds());
}
std::vector<std::string> MucoPhase::createStateInfoNames() const {
    std::vector<std::string> names(getProperty_state_infos().size());
    for (int i = 0; i < getProperty_state_infos().size(); ++i) {
        names[i] = get_state_infos(i).getName();
    }
    return names;
}
std::vector<std::string> MucoPhase::createControlInfoNames() const {
    std::vector<std::string> names(getProperty_control_infos().size());
    for (int i = 0; i < getProperty_control_infos().size(); ++i) {
        names[i] = get_control_infos(i).getName();
    }
    return names;
}
std::vector<std::string> MucoPhase::createParameterNames() const {
    std::vector<std::string> names(getProperty_parameters().size());
    for (int i = 0; i < getProperty_parameters().size(); ++i) {
        names[i] = get_parameters(i).getName();
    }
    return names;
}
std::vector<std::string> MucoPhase::createPathConstraintNames() const {
    std::vector<std::string> names(getProperty_path_constraints().size());
    for (int i = 0; i < getProperty_path_constraints().size(); ++i) {
        names[i] = get_path_constraints(i).getName();
    }
    return names;
}
const MucoVariableInfo& MucoPhase::getStateInfo(
        const std::string& name) const {
    int idx = getProperty_state_infos().findIndexForName(name);
    OPENSIM_THROW_IF_FRMOBJ(idx == -1, Exception,
            "No info available for state '" + name + "'.");
    return get_state_infos(idx);
}
const MucoVariableInfo& MucoPhase::getControlInfo(
        const std::string& name) const {

    int idx = getProperty_control_infos().findIndexForName(name);
    OPENSIM_THROW_IF_FRMOBJ(idx == -1, Exception,
            "No info provided for control for '" + name + "'.");
    return get_control_infos(idx);
}
const MucoParameter& MucoPhase::getParameter(
        const std::string& name) const {

    int idx = getProperty_parameters().findIndexForName(name);
    OPENSIM_THROW_IF_FRMOBJ(idx == -1, Exception,
            "No parameter with name '" + name + "' found.");
    return get_parameters(idx);
}
MucoParameter& MucoPhase::updParameter(
    const std::string& name) {

    int idx = getProperty_parameters().findIndexForName(name);
    OPENSIM_THROW_IF_FRMOBJ(idx == -1, Exception,
        "No parameter with name '" + name + "' found.");
    return upd_parameters(idx);
}
const MucoPathConstraint& MucoPhase::getPathConstraint(
    const std::string& name) const {

    int idx = getProperty_path_constraints().findIndexForName(name);
    OPENSIM_THROW_IF_FRMOBJ(idx == -1, Exception,
        "No constraint with name '" + name + "' found.");
    return get_path_constraints(idx);
}
void MucoPhase::printDescription(std::ostream& stream) const {
    stream << "Costs:";
    if (getProperty_costs().empty())
        stream << " none";
    else
        stream << " (total: " << getProperty_costs().size() << ")";
    stream << "\n";
    for (int i = 0; i < getProperty_costs().size(); ++i) {
        stream << "  ";
        get_costs(i).printDescription(stream);
    }

    stream << "Constraints:";
    if (getProperty_path_constraints().empty() 
            && m_multibody_constraint_infos.empty())
        stream << " none";
    else
        stream << " (total: " << getProperty_path_constraints().size() << 
            m_multibody_constraint_infos.size() << ")";
    stream << "\n";
    for (int i = 0; i < m_multibody_constraint_infos.size(); ++i) {
        stream << "  ";
        m_multibody_constraint_infos[i].printDescription(stream);
    }
    for (int i = 0; i < getProperty_path_constraints().size(); ++i) {
        stream << "  ";
        get_path_constraints(i).getConstraintInfo().printDescription(stream);
    }

    stream << "States:";
    if (getProperty_state_infos().empty())
        stream << " none";
    else
        stream << " (total: " << getProperty_state_infos().size() << ")";
    stream << "\n";
    // TODO want to loop through the model's state variables and controls, not
    // just the infos.
    for (int i = 0; i < getProperty_state_infos().size(); ++i) {
        stream << "  ";
        get_state_infos(i).printDescription(stream);
    }

    stream << "Controls:";
    if (getProperty_control_infos().empty())
        stream << " none";
    else
        stream << " (total: " << getProperty_control_infos().size() << "):";
    stream << "\n";
    for (int i = 0; i < getProperty_control_infos().size(); ++i) {
        stream << "  ";
        get_control_infos(i).printDescription(stream);
    }

    stream << "Parameters:";
    if (getProperty_parameters().empty())
        stream << " none";
    else
        stream << " (total: " << getProperty_parameters().size() << "):";
    stream << "\n";
    for (int i = 0; i < getProperty_parameters().size(); ++i) {
        stream << "  ";
        get_parameters(i).printDescription(stream);
    }

    stream.flush();
}

void MucoPhase::initialize(Model& model) const {
    /// Must use the model provided in this function, *not* the one stored as
    /// a property in this class.
    const auto stateNames = model.getStateVariableNames();
    for (int i = 0; i < getProperty_state_infos().size(); ++i) {
        const auto& name = get_state_infos(i).getName();
        OPENSIM_THROW_IF(stateNames.findIndex(name) == -1, Exception,
                "State info provided for nonexistant state '" + name + "'.");
    }
    OpenSim::Array<std::string> actuNames;
    const auto modelPath = model.getAbsolutePath();
    for (const auto& actu : model.getComponentList<Actuator>()) {
        actuNames.append(
                actu.getAbsolutePath().formRelativePath(modelPath).toString());
    }

    // TODO can only handle ScalarActuators?
    for (int i = 0; i < getProperty_control_infos().size(); ++i) {
        const auto& name = get_control_infos(i).getName();
        OPENSIM_THROW_IF(actuNames.findIndex(name) == -1, Exception,
                "Control info provided for nonexistant actuator "
                "'" + name + "'.");
    }

    for (int i = 0; i < getProperty_parameters().size(); ++i) {
        const_cast<MucoParameter&>(get_parameters(i)).initialize(model);
    }

    for (int i = 0; i < getProperty_costs().size(); ++i) {
        const_cast<MucoCost&>(get_costs(i)).initialize(model);
    }

    const auto& matter = model.getMatterSubsystem();
    const auto NC = matter.getNumConstraints();
    const auto& state = model.getWorkingState();
    int mp, mv, ma;
    for (SimTK::ConstraintIndex cid(0); cid < NC; ++cid) {
        const SimTK::Constraint& constraint = matter.getConstraint(cid);
        if (!constraint.isDisabled(state)) {
            constraint.getNumConstraintEquationsInUse(state, mp, mv, ma);
            MucoMultibodyConstraintInfo mcInfo(cid, mp, mv, ma);

            // If user provided custom bounds for the multibody constraint
            // equations, update the current multibody constraint info.
            // TODO user eventually should specify a constraint info
            if (!getProperty_multibody_constraint_bounds().empty()) {
                std::vector<MucoBounds> mcBounds(mcInfo.getNumEquations(), 
                    get_multibody_constraint_bounds());
                mcInfo.setBounds(mcBounds);
            }

            m_multibody_constraint_infos.push_back(mcInfo);
        }
    }

    for (int i = 0; i < getProperty_path_constraints().size(); ++i) {
        const_cast<MucoPathConstraint&>(get_path_constraints(i)).initialize(
            model, m_num_path_constraint_eqs);
        m_num_path_constraint_eqs += 
            get_path_constraints(i).getConstraintInfo().getNumEquations();
    }
}
void MucoPhase::applyParametersToModel(
        const SimTK::Vector& parameterValues) const {
    OPENSIM_THROW_IF(parameterValues.size() != getProperty_parameters().size(), 
        Exception, "There are " + 
        std::to_string(getProperty_parameters().size()) + " parameters in "
        "this MucoProblem, but " + std::to_string(parameterValues.size()) + 
        " values were provided.");
    for (int i = 0; i < getProperty_parameters().size(); ++i) {
        get_parameters(i).applyParameterToModel(parameterValues(i));
    }
}



// ============================================================================
// MucoProblem
// ============================================================================
MucoProblem::MucoProblem() {
    constructProperties();
}

void MucoProblem::setModel(const Model& model) {
    upd_phases(0).setModel(model);
}
void MucoProblem::setTimeBounds(const MucoInitialBounds& initial,
        const MucoFinalBounds& final) {
    upd_phases(0).setTimeBounds(initial, final);
}
void MucoProblem::setStateInfo(const std::string& name,
        const MucoBounds& bounds,
        const MucoInitialBounds& initial, const MucoFinalBounds& final) {
    upd_phases(0).setStateInfo(name, bounds, initial, final);
}
void MucoProblem::setControlInfo(const std::string& name,
        const MucoBounds& bounds,
        const MucoInitialBounds& initial, const MucoFinalBounds& final) {
    upd_phases(0).setControlInfo(name, bounds, initial, final);
}
void MucoProblem::addParameter(const MucoParameter& parameter) {
    upd_phases(0).addParameter(parameter);
}
void MucoProblem::addCost(const MucoCost& cost) {
    upd_phases(0).addCost(cost);
}
void MucoProblem::addPathConstraint(const MucoPathConstraint& cost) {
    upd_phases(0).addPathConstraint(cost);
}
void MucoProblem::printDescription(std::ostream& stream) const {
    std::stringstream ss;
    const int& numPhases = getProperty_phases().size();
    if (numPhases > 1) stream << "Number of phases: " << numPhases << "\n";
    for (int i = 0; i < numPhases; ++i) {
        get_phases(i).printDescription(ss);
    }
    stream << ss.str() << std::endl;
}
void MucoProblem::initialize(Model& model) const {
    for (int i = 0; i < getProperty_phases().size(); ++i)
        get_phases(i).initialize(model);
}
void MucoProblem::constructProperties() {
    constructProperty_phases(Array<MucoPhase>(MucoPhase(), 1));
}
