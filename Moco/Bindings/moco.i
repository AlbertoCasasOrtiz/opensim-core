%newobject *::clone;

/* To recognize SimTK::RowVector in header files (TODO: move to simbody.i) */
typedef SimTK::RowVector_<double> RowVector;

%include <Moco/osimMocoDLL.h>

%include <Moco/Common/TableProcessor.h>

%include <Moco/MocoCost/MocoCost.h>
%include <Moco/MocoWeightSet.h>
%include <Moco/MocoCost/MocoStateTrackingCost.h>
%include <Moco/MocoCost/MocoMarkerTrackingCost.h>
%include <Moco/MocoCost/MocoMarkerEndpointCost.h>
%include <Moco/MocoCost/MocoControlCost.h>
%include <Moco/MocoCost/MocoJointReactionCost.h>
%include <Moco/MocoCost/MocoOrientationTrackingCost.h>
%include <Moco/MocoCost/MocoTranslationTrackingCost.h>


// %template(MocoBoundsVector) std::vector<OpenSim::MocoBounds>;

%include <Moco/MocoBounds.h>
%include <Moco/MocoVariableInfo.h>

// %template(MocoVariableInfoVector) std::vector<OpenSim::MocoVariableInfo>;

%ignore OpenSim::MocoMultibodyConstraint::getKinematicLevels;
%ignore OpenSim::MocoConstraintInfo::getBounds;
%ignore OpenSim::MocoConstraintInfo::setBounds;
%ignore OpenSim::MocoProblemRep::getMultiplierInfos;

%include <Moco/MocoConstraint.h>

// unique_ptr
// ----------
// https://stackoverflow.com/questions/27693812/how-to-handle-unique-ptrs-with-swig
namespace std {
%feature("novaluewrapper") unique_ptr;
template <typename Type>
struct unique_ptr {
    typedef Type* pointer;
    explicit unique_ptr( pointer Ptr );
    unique_ptr (unique_ptr&& Right);
    template<class Type2, Class Del2> unique_ptr( unique_ptr<Type2, Del2>&& Right );
    unique_ptr( const unique_ptr& Right) = delete;
    pointer operator-> () const;
    pointer release ();
    void reset (pointer __p=pointer());
    void swap (unique_ptr &__u);
    pointer get () const;
    operator bool () const;
    ~unique_ptr();
};
}

// https://github.com/swig/swig/blob/master/Lib/python/std_auto_ptr.i
#if SWIGPYTHON
%define moco_unique_ptr(TYPE)
    %template() std::unique_ptr<TYPE>;
    %newobject std::unique_ptr<TYPE>::release;
    %typemap(out) std::unique_ptr<TYPE> %{
        %set_output(SWIG_NewPointerObj($1.release(), $descriptor(TYPE *), SWIG_POINTER_OWN | %newpointer_flags));
    %}
%enddef
#endif

// https://github.com/swig/swig/blob/master/Lib/java/std_auto_ptr.i
#if SWIGJAVA
%define moco_unique_ptr(TYPE)
    %template() std::unique_ptr<TYPE>;
    %typemap(jni) std::unique_ptr<TYPE> "jlong"
    %typemap(jtype) std::unique_ptr<TYPE> "long"
    %typemap(jstype) std::unique_ptr<TYPE> "$typemap(jstype, TYPE)"
    %typemap(out) std::unique_ptr<TYPE> %{
        jlong lpp = 0;
        *(TYPE**) &lpp = $1.release();
        $result = lpp;
    %}
    %typemap(javaout) std::unique_ptr<TYPE> {
        long cPtr = $jnicall;
        return (cPtr == 0) ? null : new $typemap(jstype, TYPE)(cPtr, true);
    }
%enddef
#endif

moco_unique_ptr(OpenSim::MocoProblemRep);

%include <Moco/MocoProblemRep.h>

// MocoProblemRep() is not copyable, but by default, SWIG tries to make a copy
// when wrapping createRep().
%rename(createRep) OpenSim::MocoProblem::createRepHeap;

namespace OpenSim {
    %ignore MocoPhase::setModel(Model);
    %ignore MocoPhase::setModel(std::unique_ptr<Model>);
    %ignore MocoProblem::setModel(Model);
    %ignore MocoProblem::setModel(std::unique_ptr<Model>);
}

%extend OpenSim::MocoPhase {
    void setModel(Model* model) {
        $self->setModel(std::unique_ptr<Model>(model));
    }
    void addParameter(MocoParameter* ptr) {
        $self->addParameter(std::unique_ptr<MocoParameter>(ptr));
    }
    void addCost(MocoCost* ptr) {
        $self->addCost(std::unique_ptr<MocoCost>(ptr));
    }
    void addPathConstraint(MocoPathConstraint* ptr) {
        $self->addPathConstraint(std::unique_ptr<MocoPathConstraint>(ptr));
    }
}

%extend OpenSim::MocoProblem {
    void setModel(Model* model) {
        $self->setModel(std::unique_ptr<Model>(model));
    }
    void addParameter(MocoParameter* ptr) {
        $self->addParameter(std::unique_ptr<MocoParameter>(ptr));
    }
    void addCost(MocoCost* ptr) {
        $self->addCost(std::unique_ptr<MocoCost>(ptr));
    }
    void addPathConstraint(MocoPathConstraint* ptr) {
        $self->addPathConstraint(std::unique_ptr<MocoPathConstraint>(ptr));
    }
}

%include <Moco/MocoProblem.h>
%include <Moco/MocoParameter.h>

// Workaround for SWIG not supporting inherited constructors.
%define EXPOSE_BOUNDS_CONSTRUCTORS_HELPER(NAME)
%extend OpenSim::NAME {
    NAME() { return new NAME(); }
    NAME(double value) { return new NAME(value); }
    NAME(double lower, double upper) { return new NAME(lower, upper); }
};
%enddef
EXPOSE_BOUNDS_CONSTRUCTORS_HELPER(MocoInitialBounds);
EXPOSE_BOUNDS_CONSTRUCTORS_HELPER(MocoFinalBounds);


// SWIG does not support initializer_list, but we can use Java arrays to
// achieve similar syntax in MATLAB.
%ignore OpenSim::MocoIterate::setTime(std::initializer_list<double>);
%ignore OpenSim::MocoIterate::setState(const std::string&,
        std::initializer_list<double>);
%ignore OpenSim::MocoIterate::setControl(const std::string&,
        std::initializer_list<double>);
%ignore OpenSim::MocoIterate::setMultiplier(const std::string&,
        std::initializer_list<double>);

%include <Moco/MocoIterate.h>

%include <Moco/MocoSolver.h>
%include <Moco/MocoDirectCollocationSolver.h>


namespace OpenSim {
    %ignore MocoTropterSolver::MocoTropterSolver(const MocoProblem&);
}
%include <Moco/MocoTropterSolver.h>
%include <Moco/MocoCasADiSolver/MocoCasADiSolver.h>
%include <Moco/MocoStudy.h>

%include <Moco/Components/ActivationCoordinateActuator.h>
%include <Moco/Components/DeGrooteFregly2016Muscle.h>
%include <Moco/MocoUtilities.h>

%include <Moco/Components/ModelFactory.h>
