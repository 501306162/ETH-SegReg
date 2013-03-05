#include <stdio.h>
#include <iostream>
#include "Registration-Propagation-Modular.h"
#include "Registration-UpdateResidual.h"
#include "Registration-circles.h"
#include "Registration-Propagation-Indirect.h"
#include "Registration-Propagation-Indirect-Statismo.h"
#include "Registration-Propagation-Indirect-CircleWeighting.h"

using namespace std;



int main(int argc, char ** argv)
{
    typedef float PixelType;
    const unsigned int D=3;
    typedef itk::Image<PixelType,D> ImageType;
    //RegistrationUpdateResidual<ImageType> net;
    //RegistrationPropagationModular<ImageType> net;
   //RegistrationPropagationIndirect<ImageType> net;
    RegistrationPropagationIndirectCircleWeight<ImageType> net;
    //RegistrationPropagationIndirectStatismo<ImageType> net;
    //RegistrationCircles<ImageType> net;
    net.run(argc,argv);
    return 1;
}