set(DOCUMENTATION "This module contains filters that deconvolve images
that have been blurred with a shift-invariant kernel.")

itk_module(ITKDeconvolution
  DEPENDS
    ITKConvolution
    ITKImageSource
  TEST_DEPENDS
    ITKTestKernel
  DESCRIPTION
    "${DOCUMENTATION}"
)
