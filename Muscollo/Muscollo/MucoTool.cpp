/* -------------------------------------------------------------------------- *
 * OpenSim Muscollo: MucoTool.cpp                                             *
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
#include "MucoTool.h"
#include "MucoProblem.h"

namespace OpenSim {
class MucoSolver { // TODO
};
}

using namespace OpenSim;

MucoTool::MucoTool() : _problem(new MucoProblem()) {}

MucoProblem& MucoTool::updProblem() {
    return _problem.updRef();
}

MucoSolver& MucoTool::initSolver() {
    MucoSolver ms;
    return ms; /*TODO*/
}

MucoSolution MucoTool::solve() {
    return MucoSolution();
}
