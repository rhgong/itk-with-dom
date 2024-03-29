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

#include "itkExpectationBasedPointSetToPointSetMetricv4.h"
#include "itkGradientDescentOptimizerv4.h"
#include "itkTransform.h"
#include "itkAffineTransform.h"
#include "itkRegistrationParameterScalesFromPhysicalShift.h"
#include "itkCommand.h"

#include <fstream>

template<class TFilter>
class itkExpectationBasedPointSetMetricRegistrationTestCommandIterationUpdate : public itk::Command
{
public:
  typedef itkExpectationBasedPointSetMetricRegistrationTestCommandIterationUpdate   Self;

  typedef itk::Command             Superclass;
  typedef itk::SmartPointer<Self>  Pointer;
  itkNewMacro( Self );

protected:
  itkExpectationBasedPointSetMetricRegistrationTestCommandIterationUpdate() {};

public:

  void Execute(itk::Object *caller, const itk::EventObject & event)
    {
    Execute( (const itk::Object *) caller, event);
    }

  void Execute(const itk::Object * object, const itk::EventObject & event)
    {
    if( typeid( event ) != typeid( itk::IterationEvent ) )
      {
      return;
      }
    const TFilter *optimizer = dynamic_cast< const TFilter * >( object );

    if( !optimizer )
      {
      itkGenericExceptionMacro( "Error dynamic_cast failed" );
      }
    std::cout << "It: " << optimizer->GetCurrentIteration() << " metric value: " << optimizer->GetValue();
    std::cout << std::endl;
    }
};

int itkExpectationBasedPointSetMetricRegistrationTest( int argc, char *argv[] )
{
  const unsigned int Dimension = 2;

  unsigned int numberOfIterations = 10;
  if( argc > 1 )
    {
    numberOfIterations = atoi( argv[1] );
    }

  typedef itk::PointSet<unsigned char, Dimension> PointSetType;

  typedef PointSetType::PointType PointType;

  PointSetType::Pointer fixedPoints = PointSetType::New();
  fixedPoints->Initialize();

  PointSetType::Pointer movingPoints = PointSetType::New();
  movingPoints->Initialize();


  // two ellipses, one rotated slightly
/*
  // Having trouble with these, as soon as there's a slight rotation added.
  unsigned long count = 0;
  for( float theta = 0; theta < 2.0 * vnl_math::pi; theta += 0.1 )
    {
    float radius = 100.0;
    PointType fixedPoint;
    fixedPoint[0] = 2 * radius * vcl_cos( theta );
    fixedPoint[1] = radius * vcl_sin( theta );
    fixedPoints->SetPoint( count, fixedPoint );

    PointType movingPoint;
    movingPoint[0] = 2 * radius * vcl_cos( theta + (0.02 * vnl_math::pi) ) + 2.0;
    movingPoint[1] = radius * vcl_sin( theta + (0.02 * vnl_math::pi) ) + 2.0;
    movingPoints->SetPoint( count, movingPoint );

    count++;
    }
*/

  // two circles with a small offset
  PointType offset;
  for( unsigned int d=0; d < Dimension; d++ )
    {
    offset[d] = 2.0;
    }
  unsigned long count = 0;
  for( float theta = 0; theta < 2.0 * vnl_math::pi; theta += 0.1 )
    {
    PointType fixedPoint;
    float radius = 100.0;
    fixedPoint[0] = radius * vcl_cos( theta );
    fixedPoint[1] = radius * vcl_sin( theta );
    if( Dimension > 2 )
      {
      fixedPoint[2] = radius * vcl_sin( theta );
      }
    fixedPoints->SetPoint( count, fixedPoint );

    PointType movingPoint;
    movingPoint[0] = fixedPoint[0] + offset[0];
    movingPoint[1] = fixedPoint[1] + offset[1];
    if( Dimension > 2 )
      {
      movingPoint[2] = fixedPoint[2] + offset[2];
      }
    movingPoints->SetPoint( count, movingPoint );

    count++;
    }

  typedef itk::AffineTransform<double, Dimension> AffineTransformType;
  AffineTransformType::Pointer transform = AffineTransformType::New();
  transform->SetIdentity();

  // Instantiate the metric
  typedef itk::ExpectationBasedPointSetToPointSetMetricv4<PointSetType> PointSetMetricType;
  PointSetMetricType::Pointer metric = PointSetMetricType::New();
  metric->SetFixedPointSet( fixedPoints );
  metric->SetMovingPointSet( movingPoints );
  metric->SetPointSetSigma( 2.0 );
  metric->SetEvaluationKNeighborhood( 10 );
  metric->SetMovingTransform( transform );
  metric->Initialize();

  // scales estimator
  typedef itk::RegistrationParameterScalesFromPhysicalShift< PointSetMetricType > RegistrationParameterScalesFromShiftType;
  RegistrationParameterScalesFromShiftType::Pointer shiftScaleEstimator = RegistrationParameterScalesFromShiftType::New();
  shiftScaleEstimator->SetMetric( metric );
  // needed with pointset metrics
  shiftScaleEstimator->SetVirtualDomainPointSet( metric->GetVirtualTransformedPointSet() );

  // optimizer
  typedef itk::GradientDescentOptimizerv4  OptimizerType;
  OptimizerType::Pointer  optimizer = OptimizerType::New();
  optimizer->SetMetric( metric );
  optimizer->SetNumberOfIterations( numberOfIterations );
  optimizer->SetScalesEstimator( shiftScaleEstimator );
  optimizer->SetMaximumStepSizeInPhysicalUnits( 3.0 );

  typedef itkExpectationBasedPointSetMetricRegistrationTestCommandIterationUpdate<OptimizerType> CommandType;
  CommandType::Pointer observer = CommandType::New();
  optimizer->AddObserver( itk::IterationEvent(), observer );

  optimizer->SetMinimumConvergenceValue( 0.0 );
  optimizer->SetConvergenceWindowSize( 10 );
  optimizer->StartOptimization();

  std::cout << "numberOfIterations: " << numberOfIterations << std::endl;
  std::cout << "Moving-source final value: " << optimizer->GetValue() << std::endl;
  std::cout << "Moving-source final position: " << optimizer->GetCurrentPosition() << std::endl;
  std::cout << "Optimizer scales: " << optimizer->GetScales() << std::endl;
  std::cout << "Optimizer learning rate: " << optimizer->GetLearningRate() << std::endl;

  // applying the resultant transform to moving points and verify result
  std::cout << "Fixed\tMoving\tMoving Transformed\tDiff" << std::endl;
  bool passed = true;
  PointType::ValueType tolerance = 1e-4;
  AffineTransformType::InverseTransformBasePointer movingInverse = metric->GetMovingTransform()->GetInverseTransform();
  AffineTransformType::InverseTransformBasePointer fixedInverse = metric->GetFixedTransform()->GetInverseTransform();
  for( unsigned int n=0; n < metric->GetNumberOfComponents(); n++ )
    {
    // compare the points in virtual domain
    PointType transformedMovingPoint = movingInverse->TransformPoint( movingPoints->GetPoint( n ) );
    PointType transformedFixedPoint = fixedInverse->TransformPoint( fixedPoints->GetPoint( n ) );
    PointType difference;
    difference[0] = transformedMovingPoint[0] - transformedFixedPoint[0];
    difference[1] = transformedMovingPoint[1] - transformedFixedPoint[1];
    std::cout << fixedPoints->GetPoint( n ) << "\t" << movingPoints->GetPoint( n )
          << "\t" << transformedMovingPoint << "\t" << transformedFixedPoint << "\t" << difference << std::endl;
    if( fabs( difference[0] ) > tolerance || fabs( difference[1] ) > tolerance )
      {
      passed = false;
      }
    }
  if( ! passed )
    {
    std::cerr << "Results do not match truth within tolerance." << std::endl;
    return EXIT_FAILURE;
    }


  return EXIT_SUCCESS;
}
