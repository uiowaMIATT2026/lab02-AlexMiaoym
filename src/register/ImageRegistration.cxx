// ITK 2D registration: translation transform, mean-squares metric, regular-step gradient descent.
#include <string>

#include "itkImageRegistrationMethodv4.h"
#include "itkTranslationTransform.h"
#include "itkMeanSquaresImageToImageMetricv4.h"
#include "itkRegularStepGradientDescentOptimizerv4.h"

#include "itkImageFileReader.h"
#include "itkImageFileWriter.h"

#include "itkResampleImageFilter.h"
#include "itkCastImageFilter.h"
#include "itkRescaleIntensityImageFilter.h"
#include "itkSubtractImageFilter.h"

// Prints iteration index, metric value, and optimizer parameters on each iteration event.
class CommandIterationUpdate : public itk::Command
{
public:
  using Self = CommandIterationUpdate;
  using Superclass = itk::Command;
  using Pointer = itk::SmartPointer<Self>;
  itkNewMacro(Self);

protected:
  CommandIterationUpdate() = default;

public:
  using OptimizerType = itk::RegularStepGradientDescentOptimizerv4<double>;
  using OptimizerPointer = const OptimizerType *;

  void
  Execute(itk::Object * caller, const itk::EventObject & event) override
  {
    Execute((const itk::Object *)caller, event);
  }

  void
  Execute(const itk::Object * object, const itk::EventObject & event) override
  {
    auto optimizer = static_cast<OptimizerPointer>(object);

    if (!itk::IterationEvent().CheckEvent(&event))
    {
      return;
    }

    std::cout << optimizer->GetCurrentIteration() << " = ";
    std::cout << optimizer->GetValue() << " : ";
    std::cout << optimizer->GetCurrentPosition() << std::endl;
  }
};

namespace
{
// Same physical frame as CreateImage.cxx (spacing 0.5 mm; origin so grid center is cx, cy).
void
ApplyCreateImageGeometry(itk::Image<float, 2> * img, double cx, double cy)
{
  constexpr double kSpacingMm = 0.5;
  constexpr unsigned int Dimension = 2;
  const itk::Size<Dimension> sz = img->GetLargestPossibleRegion().GetSize();
  const double               hx = 0.5 * static_cast<double>(sz[0] - 1);
  const double               hy = 0.5 * static_cast<double>(sz[1] - 1);
  itk::Image<float, Dimension>::SpacingType sp;
  sp.Fill(kSpacingMm);
  img->SetSpacing(sp);
  itk::Image<float, Dimension>::PointType org;
  org[0] = cx - hx * kSpacingMm;
  org[1] = cy - hy * kSpacingMm;
  img->SetOrigin(org);
}
} // namespace

int main(int argc, char * argv[])
{
  if (argc < 4)
  {
    std::cerr << "Missing Parameters " << std::endl;
    std::cerr << "Usage: " << argv[0];
    std::cerr << " fixedImageFile  movingImageFile ";
    std::cerr << "outputImagefile [differenceImageAfter]";
    std::cerr << "[differenceImageBefore] [useEstimator]" << std::endl;
    return EXIT_FAILURE;
  }

  constexpr unsigned int Dimension = 2;
  using PixelType = float;

  // Fixed and moving images: 2D float scalar images.
  using FixedImageType = itk::Image<PixelType, Dimension>;
  using MovingImageType = itk::Image<PixelType, Dimension>;

  // Maps fixed-image space into moving-image space via translation only.
  using TransformType = itk::TranslationTransform<double, Dimension>;

  // Optimizer that steps through the transform parameters to minimize the metric.
  using OptimizerType = itk::RegularStepGradientDescentOptimizerv4<double>;

  // Mean squared difference between fixed and warped moving intensities.
  using MetricType = itk::MeanSquaresImageToImageMetricv4<FixedImageType, MovingImageType>;

  // Registration driver wiring metric, optimizer, transforms, and images.
  using RegistrationType = itk::ImageRegistrationMethodv4<FixedImageType, MovingImageType, TransformType>;

  // Instantiate metric, optimizer, and registration object.
  auto metric = MetricType::New();
  auto optimizer = OptimizerType::New();
  auto registration = RegistrationType::New();

  registration->SetMetric(metric);
  registration->SetOptimizer(optimizer);

  // Linear interpolation for subpixel intensity lookups on fixed and moving images.
  using FixedLinearInterpolatorType = itk::LinearInterpolateImageFunction<FixedImageType, double>;
  using MovingLinearInterpolatorType = itk::LinearInterpolateImageFunction<MovingImageType, double>;

  auto fixedInterpolator = FixedLinearInterpolatorType::New();
  auto movingInterpolator = MovingLinearInterpolatorType::New();
  
  metric->SetFixedInterpolator(fixedInterpolator);
  metric->SetMovingInterpolator(movingInterpolator);

  using FixedImageReaderType = itk::ImageFileReader<FixedImageType>;
  using MovingImageReaderType = itk::ImageFileReader<MovingImageType>;
  auto fixedImageReader = FixedImageReaderType::New();
  auto movingImageReader = MovingImageReaderType::New();

  // Read fixed and moving volumes from disk.
  fixedImageReader->SetFileName(argv[1]);
  movingImageReader->SetFileName(argv[2]);

  // PNG readers usually do not restore spacing/origin. Apply the same physical frame as
  // CreateImage: spacing 0.5 mm (1 mm per 2 pixels); origin so grid center is (cx, cy) mm:
  //   origin = center - 0.5 * (size/2 - 0.5) per axis = center - 0.25 * (size - 1)  [same as CreateImage]
  // Default pair (change constants if your createimage centers differ):
  //   fixed:  createimage ... 30 50   -> (50, 50) mm
  //   moving: createimage ... 60 200   -> (200, 200) mm
  fixedImageReader->Update();
  movingImageReader->Update();

  auto fixedImage = fixedImageReader->GetOutput();
  auto movingImage = movingImageReader->GetOutput();

  // Centers (mm) must match the arguments you passed to createimage for each phantom.
  constexpr double kFixedCenterMm = 50.0;
  constexpr double kMovingCenterMm = 200.0;

  ApplyCreateImageGeometry(fixedImage, kFixedCenterMm, kFixedCenterMm);
  ApplyCreateImageGeometry(movingImage, kMovingCenterMm, kMovingCenterMm);

  registration->SetFixedImage(fixedImage);
  registration->SetMovingImage(movingImage);

  // Mean squares over the full 512² grid is dominated by matching black (0) to black (0), so at
  // T=(0,0) the cost looks tiny and the gradient ~0 even though the circles do not line up.
  // Start near the correct translation: movingPoint = fixedPoint + T (ITK translation convention).
  auto initialTransform = TransformType::New();
  {
    TransformType::ParametersType p(2);
    p[0] = kMovingCenterMm - kFixedCenterMm;
    p[1] = kMovingCenterMm - kFixedCenterMm;
    initialTransform->SetParameters(p);
  }
  registration->SetInitialTransform(initialTransform);

  // Moving-image initial transform: start at zero translation (millimeters).
  auto movingInitialTransform = TransformType::New();

  TransformType::ParametersType initialParameters(movingInitialTransform->GetNumberOfParameters());
  initialParameters[0] = 0.0; // Initial offset in mm along X
  initialParameters[1] = 0.0; // Initial offset in mm along Y

  movingInitialTransform->SetParameters(initialParameters);

  registration->SetMovingInitialTransform(movingInitialTransform);

  auto identityTransform = TransformType::New();
  identityTransform->SetIdentity();

  registration->SetFixedInitialTransform(identityTransform);

  // Optimizer: learning rate, step size floor, relaxation, and iteration budget.
  optimizer->SetLearningRate(4);
  optimizer->SetMinimumStepLength(0.001);
  optimizer->SetRelaxationFactor(0.5);
  optimizer->SetNumberOfIterations(200);

  bool useEstimator = false;
  if (argc > 6)
  {
    useEstimator = std::stoi(argv[6]) != 0;
  }

  // Optionally estimate parameter scales and one-shot learning rate from physical shifts.
  if (useEstimator)
  {
    using ScalesEstimatorType =
      itk::RegistrationParameterScalesFromPhysicalShift<MetricType>;
    auto scalesEstimator = ScalesEstimatorType::New();
    scalesEstimator->SetMetric(metric);
    scalesEstimator->SetTransformForward(true);
    optimizer->SetScalesEstimator(scalesEstimator);
    optimizer->SetDoEstimateLearningRateOnce(true);
  }

  // Attach observer to print progress each iteration.
  auto observer = CommandIterationUpdate::New();
  optimizer->AddObserver(itk::IterationEvent(), observer);

  // Single-level registration: full resolution, no smoothing or additional shrink.
  constexpr unsigned int numberOfLevels = 1;

  RegistrationType::ShrinkFactorsArrayType shrinkFactorsPerLevel;
  shrinkFactorsPerLevel.SetSize(1);
  shrinkFactorsPerLevel[0] = 1;

  RegistrationType::SmoothingSigmasArrayType smoothingSigmasPerLevel;
  smoothingSigmasPerLevel.SetSize(1);
  smoothingSigmasPerLevel[0] = 0;

  registration->SetNumberOfLevels(numberOfLevels);
  registration->SetSmoothingSigmasPerLevel(smoothingSigmasPerLevel);
  registration->SetShrinkFactorsPerLevel(shrinkFactorsPerLevel);

  // Run registration; on failure print ITK exception and exit.
  try
  {
    registration->Update();
    std::cout << "Optimizer stop condition: "
              << registration->GetOptimizer()->GetStopConditionDescription()
              << std::endl;
  }
  catch (const itk::ExceptionObject & err)
  {
    std::cerr << "ExceptionObject caught !" << std::endl;
    std::cerr << err << std::endl;
    return EXIT_FAILURE;
  }

  // Resulting transform mapping moving into fixed space after optimization.
  const TransformType::ConstPointer transform = registration->GetTransform();

  // Read optimized translation components (mm) along X and Y.
  TransformType::ParametersType finalParameters = transform->GetParameters();
  const double                  TranslationAlongX = finalParameters[0];
  const double                  TranslationAlongY = finalParameters[1];

  // Iteration count actually completed when the optimizer stopped.
  const unsigned int numberOfIterations = optimizer->GetCurrentIteration();

  // Final metric value at the optimizer's last position.
  const double bestValue = optimizer->GetValue();

  // Report translation, iterations, and metric to stdout.
  std::cout << "Result = " << std::endl;
  std::cout << " Translation X = " << TranslationAlongX << std::endl;
  std::cout << " Translation Y = " << TranslationAlongY << std::endl;
  std::cout << " Iterations    = " << numberOfIterations << std::endl;
  std::cout << " Metric value  = " << bestValue << std::endl;

  // Compose moving initial transform with registration output for full resampling warp.
  using CompositeTransformType = itk::CompositeTransform<double, Dimension>;
  auto outputCompositeTransform = CompositeTransformType::New();
  outputCompositeTransform->AddTransform(movingInitialTransform);
  outputCompositeTransform->AddTransform(registration->GetModifiableTransform());

  // Resampler: warp moving image into fixed-image grid using the composite transform.
  using ResampleFilterType =
    itk::ResampleImageFilter<MovingImageType, FixedImageType>;

  auto resampler = ResampleFilterType::New();
  resampler->SetInput(movingImageReader->GetOutput());

  resampler->SetTransform(outputCompositeTransform);

  // Match output extent, origin, spacing, direction, and out-of-bounds fill to the fixed image.
  resampler->SetSize(fixedImage->GetLargestPossibleRegion().GetSize());
  resampler->SetOutputOrigin(fixedImage->GetOrigin());
  resampler->SetOutputSpacing(fixedImage->GetSpacing());
  resampler->SetOutputDirection(fixedImage->GetDirection());
  resampler->SetDefaultPixelValue(100);

  // Cast float resampled image to unsigned char for file writing.
  using OutputPixelType = unsigned char;

  using OutputImageType = itk::Image<OutputPixelType, Dimension>;

  using CastFilterType =
    itk::CastImageFilter<FixedImageType, OutputImageType>;

  using WriterType = itk::ImageFileWriter<OutputImageType>;

  auto writer = WriterType::New();
  auto caster = CastFilterType::New();

  writer->SetFileName(argv[3]);
  // Execute resample → cast → write pipeline for the registered moving image.
  caster->SetInput(resampler->GetOutput());
  writer->SetInput(caster->GetOutput());
  writer->Update();

  // Pixel-wise difference: fixed minus registered moving (float images).
  using DifferenceFilterType =
    itk::SubtractImageFilter<FixedImageType, FixedImageType, FixedImageType>;

  auto difference = DifferenceFilterType::New();

  difference->SetInput1(fixedImageReader->GetOutput());
  difference->SetInput2(resampler->GetOutput());

  // Rescale difference to 0–255 for visualization; lower default fill to preserve contrast.
  using RescalerType = itk::RescaleIntensityImageFilter<FixedImageType, OutputImageType>;

  auto intensityRescaler = RescalerType::New();

  intensityRescaler->SetInput(difference->GetOutput());
  intensityRescaler->SetOutputMinimum(0);
  intensityRescaler->SetOutputMaximum(255);

  resampler->SetDefaultPixelValue(1);

  auto writer2 = WriterType::New();
  writer2->SetInput(intensityRescaler->GetOutput());

  // Optional: write post-registration difference image if path provided.
  if (argc > 4)
  {
    writer2->SetFileName(argv[4]);
    writer2->Update();
  }

  // Resample moving image with identity only (alignment before registration) for comparison.
  resampler->SetTransform(identityTransform);

  // Optional: write pre-registration difference image if path provided.
  if (argc > 5)
  {
    writer2->SetFileName(argv[5]);
    writer2->Update();
  }

  return EXIT_SUCCESS;
}
