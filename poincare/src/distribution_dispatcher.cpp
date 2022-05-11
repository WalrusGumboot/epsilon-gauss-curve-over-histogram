#include <poincare/distribution_dispatcher.h>
#include <poincare/simplification_helper.h>
#include <poincare/layout_helper.h>
#include <poincare/serialization_helper.h>
#include <poincare/approximation_helper.h>
#include <assert.h>

namespace Poincare {

constexpr Expression::FunctionHelper NormCDF::s_functionHelper;
constexpr Expression::FunctionHelper NormCDFRange::s_functionHelper;
constexpr Expression::FunctionHelper NormPDF::s_functionHelper;
constexpr Expression::FunctionHelper InvNorm::s_functionHelper;

constexpr Expression::FunctionHelper StudentCDF::s_functionHelper;
constexpr Expression::FunctionHelper StudentCDFRange::s_functionHelper;
constexpr Expression::FunctionHelper StudentPDF::s_functionHelper;
constexpr Expression::FunctionHelper InvStudent::s_functionHelper;

constexpr Expression::FunctionHelper PoissonCDF::s_functionHelper;
constexpr Expression::FunctionHelper PoissonPDF::s_functionHelper;

constexpr Expression::FunctionHelper BinomCDF::s_functionHelper;
constexpr Expression::FunctionHelper BinomPDF::s_functionHelper;
constexpr Expression::FunctionHelper InvBinom::s_functionHelper;

constexpr Expression::FunctionHelper GeomCDF::s_functionHelper;
constexpr Expression::FunctionHelper GeomCDFRange::s_functionHelper;
constexpr Expression::FunctionHelper GeomPDF::s_functionHelper;
constexpr Expression::FunctionHelper InvGeom::s_functionHelper;

Layout DistributionDispatcherNode::createLayout(Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return LayoutHelper::Prefix(DistributionDispatcher(this), floatDisplayMode, numberOfSignificantDigits, name());
}

int DistributionDispatcherNode::serialize(char * buffer, int bufferSize, Preferences::PrintFloatMode floatDisplayMode, int numberOfSignificantDigits) const {
  return SerializationHelper::Prefix(this, buffer, bufferSize, floatDisplayMode, numberOfSignificantDigits, name());
}

Expression DistributionDispatcherNode::shallowReduce(ReductionContext reductionContext) {
  return DistributionDispatcher(this).shallowReduce(reductionContext);
}

Expression DistributionDispatcher::shallowReduce(ExpressionNode::ReductionContext reductionContext, bool * stopReduction) {
  if (stopReduction != nullptr) {
    *stopReduction = true;
  }
  {
    Expression e = SimplificationHelper::defaultShallowReduce(*this);
    if (!e.isUninitialized()) {
      return e;
    }
    e = SimplificationHelper::undefinedOnMatrix(*this, reductionContext);
    if (!e.isUninitialized()) {
      return e;
    }
    e = SimplificationHelper::distributeReductionOverLists(*this, reductionContext);
    if (!e.isUninitialized()) {
      return e;
    }
  }

  int childIndex = 0;
  Expression abscissae[DistributionMethod::k_maxNumberOfParameters];
  for (int i=0; i < DistributionMethod::numberOfParameters(methodType()); i++) {
    abscissae[i] = childAtIndex(childIndex++);
  }
  Expression parameters[Distribution::k_maxNumberOfParameters];
  for (int i=0; i < Distribution::numberOfParameters(distributionType()); i++) {
    parameters[i] = childAtIndex(childIndex++);
  }
  const DistributionMethod * function = DistributionMethod::Get(methodType());
  const Distribution * distribution = Distribution::Get(distributionType());

  bool parametersAreOk;
  bool couldCheckParameters = distribution->ExpressionParametersAreOK(&parametersAreOk, parameters, reductionContext.context());

  if (!couldCheckParameters) {
    return *this;
  }
  if (!parametersAreOk) {
    return replaceWithUndefinedInPlace();
  }

  Expression e = function->shallowReduce(abscissae, distribution, parameters, reductionContext, this);
  if (!e.isUninitialized()) {
    return e;
  }

  if (stopReduction != nullptr) {
    *stopReduction = false;
  }
  return *this;
}

template<typename T>
Evaluation<T> DistributionDispatcherNode::templatedApproximate(ApproximationContext approximationContext) const {
  return ApproximationHelper::Map<T>(
    this,
    approximationContext,
    [] (const std::complex<T> * c, int numberOfComplexes, Preferences::ComplexFormat complexFormat, Preferences::AngleUnit angleUnit, void * ctx) {
      const DistributionDispatcherNode * self = static_cast<DistributionDispatcherNode *>(ctx);
      int childIndex = 0;
      T abscissa[DistributionMethod::k_maxNumberOfParameters];
      for (int i=0; i < DistributionMethod::numberOfParameters(self->m_methodType); i++) {
        // Evaluation<T> evaluation = childAtIndex(childIndex++)->approximate(T(), approximationContext);
        abscissa[i] = ComplexNode<T>::ToScalar(c[childIndex++]);//evaluation.toScalar();
      }
      T parameters[Distribution::k_maxNumberOfParameters];
      for (int i=0; i < Distribution::numberOfParameters(self->m_distributionType); i++) {
        // Evaluation<T> evaluation = childAtIndex(childIndex++)->approximate(T(), approximationContext);
        parameters[i] = ComplexNode<T>::ToScalar(c[childIndex++]);//evaluation.toScalar();
      }
      const DistributionMethod * function = DistributionMethod::Get(self->m_methodType);
      const Distribution * distribution = Distribution::Get(self->m_distributionType);
      return Complex<T>::Builder(function->EvaluateAtAbscissa(abscissa, distribution, parameters));
    },
    true,
    static_cast<void*>(const_cast<DistributionDispatcherNode *>(this))
    );
}

}