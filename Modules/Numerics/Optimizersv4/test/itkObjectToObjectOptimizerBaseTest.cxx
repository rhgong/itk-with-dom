/*=========================================================================
 *
 *  Copyright Insight Software Consortium
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *         http://www.apache.org/licenses/LICENSE-2.0.txt
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 *
 *=========================================================================*/
#include "itkObjectToObjectOptimizerBase.h"
#include "itkImage.h"
#include "itkTestingMacros.h"

/* Create a simple metric to use for testing here. */
template< class TFixedObject,  class TMovingObject >
class ITK_EXPORT ObjectToObjectOptimizerBaseTestMetric:
  public itk::ObjectToObjectMetricBase
{
public:
  /** Standard class typedefs. */
  typedef ObjectToObjectOptimizerBaseTestMetric     Self;
  typedef itk::ObjectToObjectMetricBase             Superclass;
  typedef itk::SmartPointer< Self >                 Pointer;
  typedef itk::SmartPointer< const Self >           ConstPointer;

  typedef typename Superclass::MeasureType          MeasureType;
  typedef typename Superclass::DerivativeType       DerivativeType;
  typedef typename Superclass::ParametersType       ParametersType;
  typedef typename Superclass::ParametersValueType  ParametersValueType;

  itkTypeMacro(ObjectToObjectOptimizerBaseTestMetric, ObjectToObjectMetricBase);

  itkNewMacro(Self);

  // Pure virtual functions that all Metrics must provide
  unsigned int GetNumberOfParameters() const { return 5; }

  MeasureType GetValue() const
    {
    return 1.0;
    }

  virtual void GetDerivative( DerivativeType & derivative ) const
    {
    derivative.Fill(0.0);
    }

  virtual void GetValueAndDerivative( MeasureType & value, DerivativeType & derivative ) const
    {
    value = 1.0; derivative.Fill(0.0);
    }

  unsigned int GetNumberOfLocalParameters() const
  { return 3; }

  bool HasLocalSupport() const
  { return false; }

  void UpdateTransformParameters( DerivativeType &, ParametersValueType ) {}

  const ParametersType & GetParameters() const
  { return m_Parameters; }

  void SetParameters( ParametersType & ) {}

  void Initialize(void) throw ( itk::ExceptionObject ) {}

  void PrintSelf(std::ostream& os, itk::Indent indent) const
  { Superclass::PrintSelf( os, indent ); }

  ParametersType  m_Parameters;

private:
  ObjectToObjectOptimizerBaseTestMetric() {}
  ~ObjectToObjectOptimizerBaseTestMetric() {}
};

/* Define a simple derived optimizer class.
 * \class ObjectToObjectOptimizerBaseTestOptimizer */
class ObjectToObjectOptimizerBaseTestOptimizer
  : public itk::ObjectToObjectOptimizerBase
{
public:
  /** Standard "Self" typedef. */
  typedef ObjectToObjectOptimizerBaseTestOptimizer Self;
  typedef ObjectToObjectOptimizerBase              Superclass;
  typedef itk::SmartPointer< Self >                Pointer;
  typedef itk::SmartPointer< const Self >          ConstPointer;

  /** Method for creation through the object factory. */
  itkNewMacro(Self);

  /** Run-time type information (and related methods). */
  itkTypeMacro(ObjectToObjectOptimizerBaseTestOptimizer, ObjectToObjectOptimizerBase);

  /* Provide initialization for this class */
  void StartOptimization()
    {
    Superclass::StartOptimization();
    std::cout << "StartOptimization called from derived class." << std::endl;
    }

};

/**
 */
int itkObjectToObjectOptimizerBaseTest(int , char* [])
{
  const int ImageDimension = 2;
  typedef itk::Image<double, ImageDimension>                    ImageType;

  typedef ObjectToObjectOptimizerBaseTestMetric<ImageType,ImageType> MetricType;

  MetricType::Pointer metric = MetricType::New();
  ObjectToObjectOptimizerBaseTestOptimizer::Pointer optimizer = ObjectToObjectOptimizerBaseTestOptimizer::New();

  /* exercise some methods */
  optimizer->SetMetric( metric );
  if( optimizer->GetMetric() != metric )
    {
    std::cerr << "Set/GetMetric failed." << std::endl;
    return EXIT_FAILURE;
    }

  std::cout << "value: " << optimizer->GetValue() << std::endl;

  /* Test set/get of scales */
  ObjectToObjectOptimizerBaseTestOptimizer::NumberOfParametersType
    scalesSize = metric->GetNumberOfLocalParameters();
  ObjectToObjectOptimizerBaseTestOptimizer::ScalesType scales(scalesSize);
  scales.Fill(3.19);
  optimizer->SetScales( scales );
  const ObjectToObjectOptimizerBaseTestOptimizer::ScalesType& scalesReturn = optimizer->GetScales();
  if( scalesReturn != scales )
    {
    std::cerr << "Set/GetScales failed." << std::endl;
    return EXIT_FAILURE;
    }

  optimizer->SetNumberOfThreads( 1 );

  /* Test StartOptimization */
  TRY_EXPECT_NO_EXCEPTION( optimizer->StartOptimization() );

  /* Test with incorrectly-sized scales. Expect exception */
  scales.SetSize(scalesSize+1);
  optimizer->SetScales( scales );
  TRY_EXPECT_EXCEPTION( optimizer->StartOptimization() );

  /* Test with scales close to identity, within tolerance.
   * The flag indicating identity scales should be set. */
  scales.SetSize(scalesSize);
  scales.Fill( 0.999 );
  optimizer->SetScales( scales );
  TRY_EXPECT_NO_EXCEPTION( optimizer->StartOptimization() );
  if( ! optimizer->GetScalesAreIdentity() )
    {
    std::cerr << "Expected GetScalesAreIdentity to return true." << std::endl;
    return EXIT_FAILURE;
    }

  std::cout << "Printing self.." << std::endl;
  std::cout << optimizer << std::endl;

  return EXIT_SUCCESS;
}
