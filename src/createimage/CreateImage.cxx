#include "itkImage.h"
#include "itkImageFileWriter.h"
#include "itkImageRegionIteratorWithIndex.h"

#include <iostream>
#include <string>

int
main(int argc, char * argv[])
{
  // createimage out.png DIAMETER CENTER        -> center (CENTER, CENTER) mm
  // createimage out.png DIAMETER CX CY         -> center (CX, CY) mm
  // 512×512 pixels; spacing 0.5 mm (1 mm per 2 pixels along each axis).
  if (argc != 4 && argc != 5)
  {
    std::cerr << "Usage: " << argv[0] << " output.png diameter_mm center_mm\n";
    std::cerr << "       " << argv[0] << " output.png diameter_mm center_x_mm center_y_mm\n";
    std::cerr << "Examples: " << argv[0] << " out.png 30 50\n";
    std::cerr << "          " << argv[0] << " out.png 60 200\n";
    std::cerr << "          " << argv[0] << " out.png 30 50 60\n";
    return EXIT_FAILURE;
  }

  constexpr unsigned int Dimension = 2;
  constexpr unsigned int kSize = 512;
  // 1 mm per 2 pixels => neighbor pixel centers are 0.5 mm apart.
  constexpr double       kSpacingMm = 0.5;

  const std::string      outPath = argv[1];
  const double           diameterMm = std::stod(argv[2]);
  const double           cx = std::stod(argv[3]);
  const double           cy = (argc == 4) ? cx : std::stod(argv[4]);
  const double           radiusMm = 0.5 * diameterMm;
  const double           r2 = radiusMm * radiusMm;

  using PixelType = unsigned char;
  using ImageType = itk::Image<PixelType, Dimension>;

  auto image = ImageType::New();

  ImageType::SizeType size;
  size.Fill(kSize);

  ImageType::RegionType region;
  region.SetSize(size);

  ImageType::SpacingType spacing;
  spacing.Fill(kSpacingMm);

  // Place grid so its physical center lies at (cx, cy).
  ImageType::PointType origin;
  const double         halfIdx = 0.5 * static_cast<double>(kSize - 1);
  const double         offset = halfIdx * spacing[0];
  origin[0] = cx - offset;
  origin[1] = cy - offset;

  image->SetRegions(region);
  image->SetSpacing(spacing);
  image->SetOrigin(origin);
  image->Allocate();
  image->FillBuffer(0);

  itk::ImageRegionIteratorWithIndex<ImageType> it(image, image->GetLargestPossibleRegion());
  for (it.GoToBegin(); !it.IsAtEnd(); ++it)
  {
    ImageType::PointType p;
    image->TransformIndexToPhysicalPoint(it.GetIndex(), p);
    const double dx = p[0] - cx;
    const double dy = p[1] - cy;
    if (dx * dx + dy * dy <= r2)
    {
      it.Set(255);
    }
  }

  std::cout << "512×512, spacing " << kSpacingMm << " mm/pixel (1 mm / 2 pixels)\n";
  std::cout << "Diameter " << diameterMm << " mm, center (" << cx << ", " << cy << ") mm\n";
  std::cout << "Origin (mm): " << origin << "\n";

  using WriterType = itk::ImageFileWriter<ImageType>;
  auto writer = WriterType::New();
  writer->SetFileName(outPath);
  writer->SetInput(image);
  writer->Update();
  std::cout << "Wrote " << outPath << std::endl;

  return EXIT_SUCCESS;
}
