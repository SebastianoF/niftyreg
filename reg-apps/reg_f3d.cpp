/*
 *  reg_f3d.cpp
 *
 *
 *  Created by Marc Modat on 26/03/2009.
 *  Copyright (c) 2009, University College London. All rights reserved.
 *  Centre for Medical Image Computing (CMIC)
 *  See the LICENSE.txt file in the nifty_reg root folder
 *
 */

#include "_reg_f3d.h"
#include "_reg_f3d2.h"
#ifdef _USE_CUDA
    #include "_reg_f3d_gpu.h"
#endif
#include "float.h"
#include <limits>

#ifdef _WINDOWS
    #include <time.h>
#endif

#ifdef _USE_NR_DOUBLE
    #define PrecisionTYPE double
#else
    #define PrecisionTYPE float
#endif

void HelpPenaltyTerm()
{
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("Additional help on the penalty term that have been implemented in F3D\n");
    printf("\t-be\t Bending Energy, sum of the second derivatives of the transformation T\n");
    printf("\t\t\t(d2T/dxx)^2 + (d2T/dyy)^2 + (d2T/dzz)^2 + 2*((d2T/dxy)^2 + (d2T/dyz)^2 + (d2T/dxz)^2)\n");
    printf("\t-le\t Linear Elasticity, 3 parameters weighted differently:\n");
    printf("\t\t\t 1: Squared member of the symmetric part of the Jacobian matrix\n");
    printf("\t\t\t 1: (dTx/dx)^2 + (dTy/dy)^2 + (dTz/dz)^2 + 1/2 * ( (dTx/dy+dTy/dx)^2 +  (dTx/dz+dTz/dx)^2 +  (dTy/dz+dTz/dy)^2 ) \n");
    printf("\t\t\t 2: Divergence\n");
    printf("\t\t\t 2: (dTx/dx)^2 + (dTy/dy)^2 + (dTz/dz)^2\n");
    printf("\t\t\t 2: Squared Eucliean distance of the displacement field D\n");
    printf("\t\t\t 3: (Dx)^2 + (Dy)^2 + (Dz)^2\n");
    printf("\t-jl\t Penalty term based on the Jacobian determiant |J(T)|. Squared log of the Jacobian determinant\n");
    printf("\t\t\t log^2(|J(T)|)\n");
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    return;
}
void PetitUsage(char *exec)
{
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    fprintf(stderr,"Fast Free-Form Deformation algorithm for non-rigid registration.\n");
    fprintf(stderr,"Usage:\t%s -target <targetImageName> -source <sourceImageName> [OPTIONS].\n",exec);
    fprintf(stderr,"\tSee the help for more details (-h).\n");
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    return;
}
void Usage(char *exec)
{
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("Fast Free-Form Deformation algorithm for non-rigid registration.\n");
    printf("This implementation is a re-factoring of Daniel Rueckert' 99 TMI work.\n");
    printf("The code is presented in Modat et al., \"Fast Free-Form Deformation using\n");
    printf("graphics processing units\", CMPB, 2010\n");
    printf("Cubic B-Spline are used to deform a source image in order to optimise a objective function\n");
    printf("based on the Normalised Mutual Information and a penalty term. The penalty term could\n");
    printf("be either the bending energy or the squared Jacobian determinant log.\n");
    printf("This code has been written by Marc Modat (m.modat@ucl.ac.uk), for any comment,\n");
    printf("please contact him.\n");
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("Usage:\t%s -target <filename> -source <filename> [OPTIONS].\n",exec);
    printf("\t-target <filename>\tFilename of the target image (mandatory)\n");
    printf("\t-source <filename>\tFilename of the source image (mandatory)\n");
    printf("\n***************\n*** OPTIONS ***\n***************\n");
#ifdef _SVN_REV
    fprintf(stderr,"\n-v Print the subversion revision number\n");
#endif

    printf("\n*** Initial transformation options (One option will be considered):\n");
    printf("\t-aff <filename>\t\tFilename which contains an affine transformation (Affine*Target=Source)\n");
    printf("\t-affFlirt <filename>\tFilename which contains a flirt affine transformation (Flirt from the FSL package)\n");
    printf("\t-incpp <filename>\tFilename of control point grid input\n\t\t\t\tThe coarse spacing is defined by this file.\n");

    printf("\n*** Output options:\n");
    printf("\t-cpp <filename>\t\tFilename of control point grid [outputCPP.nii]\n");
    printf("\t-result <filename> \tFilename of the resampled image [outputResult.nii]\n");

    printf("\n*** Input image options:\n");
    printf("\t-tmask <filename>\t\tFilename of a mask image in the target space\n");
    printf("\t-smooT <float>\t\t\tSmooth the target image using the specified sigma (mm) [0]\n");
    printf("\t-smooS <float>\t\t\tSmooth the source image using the specified sigma (mm) [0]\n");
    printf("\t-tbn <timepoint> <bin>\t\tNumber of bin to use for the joint histogram [64]\n");
    printf("\t-sbn <timepoint> <bin>\t\tNumber of bin to use for the joint histogram [64]\n");
    printf("\t-tLwTh <timepoint> <float>\tLower threshold to apply to the target image intensities [none]*\n");
    printf("\t-tUpTh <timepoint> <float>\tUpper threshold to apply to the target image intensities [none]*\n");
    printf("\t-sLwTh <timepoint> <float>\tLower threshold to apply to the source image intensities [none]*\n");
    printf("\t-sUpTh <timepoint> <float>\tUpper threshold to apply to the source image intensities [none]*\n");
    printf("\t* The scl_slope and scl_inter from the nifti header are taken into account for the thresholds\n");

    printf("\n*** Spline options:\n");
    printf("\t-sx <float>\t\tFinal grid spacing along the x axis in mm (in voxel if negative value) [5 voxels]\n");
    printf("\t-sy <float>\t\tFinal grid spacing along the y axis in mm (in voxel if negative value) [sx value]\n");
    printf("\t-sz <float>\t\tFinal grid spacing along the z axis in mm (in voxel if negative value) [sx value]\n");

    printf("\n*** Objective function options:\n");
    printf("\t-be <float>\t\tWeight of the bending energy penalty term [0.01]\n");
    printf("\t-le <float> <float><float>\tWeights of linear elasticity penalty term [0.0 0.0 0.0]\n");
    printf("\t-jl <float>\t\tWeight of log of the Jacobian determinant penalty term [0.0]\n");
    printf("\t-noAppJL\t\tTo not approximate the JL value only at the control point position\n");
    printf("\t-noConj\t\t\tTo not use the conjuage gradient optimisation but a simple gradient ascent\n");
    printf("\t-ssd\t\t\tTo use the SSD as the similiarity measure (NMI by default)\n");

    printf("\n*** Optimisation options:\n");
    printf("\t-maxit <int>\t\tMaximal number of iteration per level [300]\n");
    printf("\t-ln <int>\t\tNumber of level to perform [3]\n");
    printf("\t-lp <int>\t\tOnly perform the first levels [ln]\n");
    printf("\t-nopy\t\t\tDo not use a pyramidal approach\n");

#ifdef _NR_DEV
    printf("\n*** F3D2 options:\n");
    printf("\t-vel \t\t\tUse a velocity field integrationto generate the deformation\n");
    printf("\t-useSym \t\tUse similarity measure gradient symmetry\n");
    printf("\t-approxComp \t\tApproximate the composition step\n");
    printf("\t-step <int>\t\tNumber of composition step [6].\n");
#endif

    printf("\n*** Other options:\n");
    printf("\t-smoothGrad <float>\tTo smooth the metric derivative (in mm) [0]\n");
    printf("\t-pad <float>\t\tPadding value [nan]\n");
    printf("\t-voff\t\t\tTo turn verbose off\n");

#ifdef _USE_CUDA
    printf("\n*** GPU-related options:\n");
    printf("\t-mem\t\t\tDisplay an approximate memory requierment and exit\n");
    printf("\t-gpu \t\t\tTo use the GPU implementation [no]\n");
#endif
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("For further description of the penalty term, use: %s -helpPenalty\n", exec);
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    return;
}

int main(int argc, char **argv)
{

#ifndef NDEBUG
    printf("[NiftyReg DEBUG] *******************************************\n");
    printf("[NiftyReg DEBUG] *******************************************\n");
    printf("[NiftyReg DEBUG] NiftyReg has been compiled in DEBUG mode\n");
    printf("[NiftyReg DEBUG] Please rerun cmake to set the variable\n");
    printf("[NiftyReg DEBUG] CMAKE_BUILD_TYPE to \"Release\" if required\n");
    printf("[NiftyReg DEBUG] *******************************************\n");
    printf("[NiftyReg DEBUG] *******************************************\n");
#endif
    time_t start; time(&start);

    char *referenceName=NULL;
    char *floatingName=NULL;
    char *referenceMaskName=NULL;
    char *inputControlPointGridName=NULL;
    char *outputControlPointGridName=NULL;
    char *outputWarpedName=NULL;
    char *affineTransformationName=NULL;
    bool flirtAffine=false;
    PrecisionTYPE bendingEnergyWeight=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    PrecisionTYPE linearEnergyWeight0=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    PrecisionTYPE linearEnergyWeight1=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    PrecisionTYPE linearEnergyWeight2=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    PrecisionTYPE jacobianLogWeight=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    bool jacobianLogApproximation=true;
    int maxiterationNumber=-1;
    PrecisionTYPE referenceSmoothingSigma=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    PrecisionTYPE floatingSmoothingSigma=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    PrecisionTYPE referenceThresholdUp[10];
    PrecisionTYPE referenceThresholdLow[10];
    PrecisionTYPE floatingThresholdUp[10];
    PrecisionTYPE floatingThresholdLow[10];
    unsigned int referenceBinNumber[10];
    unsigned int floatingBinNumber[10];
    for(int i=0;i<10;i++){
        referenceThresholdUp[i]=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
        referenceThresholdLow[i]=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
        floatingThresholdUp[i]=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
        floatingThresholdLow[i]=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
        referenceBinNumber[i]=0;
        floatingBinNumber[i]=0;
    }
    PrecisionTYPE warpedPaddingValue=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    PrecisionTYPE spacing[3];
    spacing[0]=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    spacing[1]=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    spacing[2]=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    unsigned int levelNumber=0;
    unsigned int levelToPerform=0;
    PrecisionTYPE gradientSmoothingSigma=std::numeric_limits<PrecisionTYPE>::quiet_NaN();
    bool verbose=true;
    bool useConjugate=true;
    bool useSSD=false;
    bool noPyramid=0;
#ifdef _NR_DEV
    int stepNumber=-1;
    bool useVel=false;
    bool useSymmetry=false;
    bool approxComposition=false;
#endif
#ifdef _USE_CUDA
    bool useGPU=false;
    bool checkMem=false;
    int cardNumber=-1;
#endif
	
    /* read the input parameter */
    for(int i=1;i<argc;i++){
        if(strcmp(argv[i], "-help")==0 || strcmp(argv[i], "-Help")==0 ||
           strcmp(argv[i], "-HELP")==0 || strcmp(argv[i], "-h")==0 ||
           strcmp(argv[i], "--h")==0 || strcmp(argv[i], "--help")==0){
            Usage(argv[0]);
            return 0;
        }
#ifdef _SVN_REV
        if(strcmp(argv[i], "-version")==0 || strcmp(argv[i], "-Version")==0 ||
           strcmp(argv[i], "-V")==0 || strcmp(argv[i], "-v")==0 ||
           strcmp(argv[i], "--v")==0 || strcmp(argv[i], "--version")==0){
            printf("NiftyReg revision number: %i\n",_SVN_REV);
            return 0;
        }
#endif
        if(strcmp(argv[i], "-helpPenalty")==0){
            HelpPenaltyTerm();
            return 0;
        }
        else if(strcmp(argv[i], "-target") == 0){
            referenceName=argv[++i];
        }
        else if(strcmp(argv[i], "-source") == 0){
            floatingName=argv[++i];
        }
        else if(strcmp(argv[i], "-voff") == 0){
            verbose=false;
        }
        else if(strcmp(argv[i], "-aff") == 0){
            affineTransformationName=argv[++i];
        }
        else if(strcmp(argv[i], "-affFlirt") == 0){
            affineTransformationName=argv[++i];
            flirtAffine=1;
        }
        else if(strcmp(argv[i], "-incpp") == 0){
            inputControlPointGridName=argv[++i];
        }
        else if(strcmp(argv[i], "-tmask") == 0){
            referenceMaskName=argv[++i];
        }
        else if(strcmp(argv[i], "-result") == 0){
            outputWarpedName=argv[++i];
        }
        else if(strcmp(argv[i], "-cpp") == 0){
            outputControlPointGridName=argv[++i];
        }
        else if(strcmp(argv[i], "-maxit") == 0){
            maxiterationNumber=atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-sx") == 0){
            spacing[0]=(float)atof(argv[++i]);
        }
        else if(strcmp(argv[i], "-sy") == 0){
            spacing[1]=(float)atof(argv[++i]);
        }
        else if(strcmp(argv[i], "-sz") == 0){
            spacing[2]=(float)atof(argv[++i]);
        }
        else if(strcmp(argv[i], "-tbn") == 0){
            referenceBinNumber[atoi(argv[i+1])]=atoi(argv[i+2]);
            i+=2;
        }
        else if(strcmp(argv[i], "-sbn") == 0){
            floatingBinNumber[atoi(argv[i+1])]=atoi(argv[i+2]);
            i+=2;
        }
        else if(strcmp(argv[i], "-ln") == 0){
            levelNumber=atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-lp") == 0){
           levelToPerform=atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-be") == 0){
            bendingEnergyWeight=(PrecisionTYPE)(atof(argv[++i]));
        }
        else if(strcmp(argv[i], "-le") == 0){
            linearEnergyWeight0=(PrecisionTYPE)(atof(argv[++i]));
            linearEnergyWeight1=(PrecisionTYPE)(atof(argv[++i]));
            linearEnergyWeight2=(PrecisionTYPE)(atof(argv[++i]));
        }
        else if(strcmp(argv[i], "-jl") == 0){
            jacobianLogWeight=(PrecisionTYPE)(atof(argv[++i]));
        }
        else if(strcmp(argv[i], "-noAppJL") == 0){
            jacobianLogApproximation=false;
        }
        else if(strcmp(argv[i], "-smooT") == 0){
            referenceSmoothingSigma=(PrecisionTYPE)(atof(argv[++i]));
		}
        else if(strcmp(argv[i], "-smooS") == 0){
            floatingSmoothingSigma=(PrecisionTYPE)(atof(argv[++i]));
        }
        else if(strcmp(argv[i], "-tLwTh") == 0){
           referenceThresholdLow[atoi(argv[i+1])]=(PrecisionTYPE)(atof(argv[i+2]));
           i+=2;
        }
        else if(strcmp(argv[i], "-tUpTh") == 0){
            referenceThresholdUp[atoi(argv[i+1])]=(PrecisionTYPE)(atof(argv[i+2]));
            i+=2;
        }
        else if(strcmp(argv[i], "-sLwTh") == 0){
            floatingThresholdLow[atoi(argv[i+1])]=(PrecisionTYPE)(atof(argv[i+2]));
            i+=2;
        }
        else if(strcmp(argv[i], "-sUpTh") == 0){
            floatingThresholdUp[atoi(argv[i+1])]=(PrecisionTYPE)(atof(argv[i+2]));
            i+=2;
        }
        else if(strcmp(argv[i], "-smoothGrad") == 0){
            gradientSmoothingSigma=(PrecisionTYPE)(atof(argv[++i]));
        }
        else if(strcmp(argv[i], "-ssd") == 0){
            useSSD=true;
        }
        else if(strcmp(argv[i], "-pad") == 0){
            warpedPaddingValue=(PrecisionTYPE)(atof(argv[++i]));
        }
        else if(strcmp(argv[i], "-nopy") == 0){
            noPyramid=1;
        }
        else if(strcmp(argv[i], "-noConj") == 0){
           useConjugate=false;
        }
#ifdef _USE_CUDA
        else if(strcmp(argv[i], "-gpu") == 0){
            useGPU=true;
        }
        else if(strcmp(argv[i], "-card") == 0){
            cardNumber=atoi(argv[++i]);
        }
        else if(strcmp(argv[i], "-mem") == 0){
            checkMem=true;
        }
#endif
#ifdef _NR_DEV
        else if(strcmp(argv[i], "-vel") == 0){
            useVel=true;
        }
        else if(strcmp(argv[i], "-approxComp") ==0){
            approxComposition=true;
            useVel=true;
        }
        else if(strcmp(argv[i], "-useSym") ==0){
            useSymmetry=true;
            useVel=true;
        }
        else if(strcmp(argv[i], "-step") == 0){
           stepNumber=atoi(argv[++i]);
           useVel=true;
        }
#endif
        else{
            fprintf(stderr,"Err:\tParameter %s unknown.\n",argv[i]);
            PetitUsage(argv[0]);
            return 1;
        }
    }

    // Output the command line
#ifdef NDEBUG
    if(verbose==true){
#endif
        printf("\n[NiftyReg F3D] Command line:\n\t");
        for(int i=0;i<argc;i++)
            printf(" %s", argv[i]);
        printf("\n\n");
#ifdef NDEBUG
    }
#endif

    // Read the reference image
    if(referenceName==NULL){
        fprintf(stderr, "Error. No reference image has been defined\n");
        return 1;
    }
    nifti_image *referenceImage = nifti_image_read(referenceName,true);
    if(referenceImage==NULL){
        fprintf(stderr, "Error when reading the reference image %s\n",referenceName);
        return 1;
    }
    reg_checkAndCorrectDimension(referenceImage);

    // Read the floating image
    if(floatingName==NULL){
        fprintf(stderr, "Error. No floating image has been defined\n");
        return 1;
    }
    nifti_image *floatingImage = nifti_image_read(floatingName,true);
    if(floatingImage==NULL){
        fprintf(stderr, "Error when reading the floating image %s\n",floatingName);
        return 1;
    }
    reg_checkAndCorrectDimension(floatingImage);

    // Read the mask image
    nifti_image *referenceMaskImage=NULL;
    if(referenceMaskName!=NULL){
        referenceMaskImage=nifti_image_read(referenceMaskName,true);
        if(referenceMaskImage==NULL){
            fprintf(stderr, "Error when reading the reference mask image %s\n",referenceMaskName);
            return 1;
        }
        reg_checkAndCorrectDimension(referenceMaskImage);
    }

    // Read the input control point grid image
    nifti_image *controlPointGridImage=NULL;
    if(inputControlPointGridName!=NULL){
        controlPointGridImage=nifti_image_read(inputControlPointGridName,true);
        if(controlPointGridImage==NULL){
            fprintf(stderr, "Error when reading the input control point grid image %s\n",inputControlPointGridName);
            return 1;
        }
        reg_checkAndCorrectDimension(controlPointGridImage);
#ifdef _NR_DEV
        if(controlPointGridImage->pixdim[5]>1)
            useVel=true;
#endif
    }

    // Read the affine transformation
    mat44 *affineTransformation=NULL;
    if(affineTransformationName!=NULL){
        affineTransformation = (mat44 *)malloc(sizeof(mat44));
        // Check first if the specified affine file exist
        if(FILE *aff=fopen(affineTransformationName, "r")){
            fclose(aff);
        }
        else{
            fprintf(stderr,"The specified input affine file (%s) can not be read\n",affineTransformationName);
            return 1;
        }
        reg_tool_ReadAffineFile(affineTransformation,
                                referenceImage,
                                floatingImage,
                                affineTransformationName,
                                flirtAffine);
    }

    // Create the reg_f3d object
    reg_f3d<PrecisionTYPE> *REG=NULL;
#ifdef _USE_CUDA
    unsigned int gpuMemoryAvailable = 0;
    if(useGPU){

        if(linearEnergyWeight0==linearEnergyWeight0 ||
           linearEnergyWeight1==linearEnergyWeight1 ||
           linearEnergyWeight2==linearEnergyWeight2){
            printf("NiftyReg ERROR CUDA] The linear elasticity has not been implemented with CUDA yet. Exit.\n");
            exit(0);
        }

        if((referenceImage->dim[4]==1&&floatingImage->dim[4]==1) || (referenceImage->dim[4]==2&&floatingImage->dim[4]==2)){

            // The CUDA card is setup
            struct cudaDeviceProp deviceProp;
            int device_count = 0;
            cudaGetDeviceCount( &device_count );
            int device=cardNumber;
            if(cardNumber==-1){
                // following code is from cutGetMaxGflopsDeviceId()
                int max_gflops_device = 0;
                int max_gflops = 0;
                int current_device = 0;
                while( current_device < device_count ){
                    cudaGetDeviceProperties( &deviceProp, current_device );
                    int gflops = deviceProp.multiProcessorCount * deviceProp.clockRate;
                    if( gflops > max_gflops ){
                        max_gflops = gflops;
                        max_gflops_device = current_device;
                    }
                    ++current_device;
                }
                device = max_gflops_device;
            }
            NR_CUDA_SAFE_CALL(cudaSetDevice( device ));
            NR_CUDA_SAFE_CALL(cudaGetDeviceProperties(&deviceProp, device ));
            if (deviceProp.major < 1){
                printf("[NiftyReg ERROR CUDA] The specified graphical card does not exist.\n");
                return 1;
            }
#ifdef NDEBUG
            if(verbose==true){
#endif
                printf("[NiftyReg F3D] Graphical card memory[%i/%i] = %iMo avail\n", device+1, device_count,
                (int)floor(deviceProp.totalGlobalMem/1000000.0));
#ifdef NDEBUG
            }
#endif
            gpuMemoryAvailable = deviceProp.totalGlobalMem;

            // The CUDA reg_f3d object is created
            REG = new reg_f3d_gpu<PrecisionTYPE>(referenceImage->nt, floatingImage->nt);
#ifdef NDEBUG
            if(verbose==true){
#endif
                printf("\n[NiftyReg F3D] GPU implementation is used\n\n");
#ifdef NDEBUG
            }
#endif
        }
        else{
            fprintf(stderr,"[NiftyReg ERROR] The GPU implementation only handle 1 to 1 or 2 to 2 image(s) registration\n");
            exit(1);
        }
    }
    else
#endif
    {

#ifdef _NR_DEV
        if(useVel){
            REG = new reg_f3d2<PrecisionTYPE>(referenceImage->nt, floatingImage->nt);
#ifdef NDEBUG
            if(verbose==true){
  #endif
                printf("\n[NiftyReg F3D2] CPU implementation is used\n");
#ifdef NDEBUG
            }
#endif
        }
        else
#endif
        {
            REG = new reg_f3d<PrecisionTYPE>(referenceImage->nt, floatingImage->nt);
#ifdef NDEBUG
            if(verbose==true){
  #endif
                printf("\n[NiftyReg F3D] CPU implementation is used\n");
#ifdef NDEBUG
            }
#endif
        }
    }

    // Set the reg_f3d parameters
    REG->SetReferenceImage(referenceImage);
    REG->SetFloatingImage(floatingImage);

    if(verbose==false) REG->DoNotPrintOutInformation();
    else REG->PrintOutInformation();

    if(referenceMaskImage!=NULL)
       REG->SetReferenceMask(referenceMaskImage);

    if(controlPointGridImage!=NULL)
       REG->SetControlPointGridImage(controlPointGridImage);

    if(affineTransformation!=NULL)
       REG->SetAffineTransformation(affineTransformation);

    if(bendingEnergyWeight==bendingEnergyWeight)
        REG->SetBendingEnergyWeight(bendingEnergyWeight);

    if(linearEnergyWeight0==linearEnergyWeight0 || linearEnergyWeight1==linearEnergyWeight1 || linearEnergyWeight2==linearEnergyWeight2){
        if(linearEnergyWeight0!=linearEnergyWeight0) linearEnergyWeight0=0.0;
        if(linearEnergyWeight1!=linearEnergyWeight1) linearEnergyWeight1=0.0;
        if(linearEnergyWeight2!=linearEnergyWeight2) linearEnergyWeight2=0.0;
        REG->SetLinearEnergyWeights(linearEnergyWeight0,linearEnergyWeight1,linearEnergyWeight2);
    }

    if(jacobianLogWeight==jacobianLogWeight)
        REG->SetJacobianLogWeight(jacobianLogWeight);

    if(jacobianLogApproximation)
        REG->ApproximateJacobianLog();
    else REG->DoNotApproximateJacobianLog();

    if(maxiterationNumber>-1)
        REG->SetMaximalIterationNumber(maxiterationNumber);

    if(referenceSmoothingSigma==referenceSmoothingSigma)
        REG->SetReferenceSmoothingSigma(referenceSmoothingSigma);

    if(floatingSmoothingSigma==floatingSmoothingSigma)
        REG->SetFloatingSmoothingSigma(floatingSmoothingSigma);

    for(unsigned int t=0;t<(unsigned int)referenceImage->nt;t++)
        if(referenceThresholdUp[t]==referenceThresholdUp[t])
            REG->SetReferenceThresholdUp(t,referenceThresholdUp[t]);

    for(unsigned int t=0;t<(unsigned int)referenceImage->nt;t++)
        if(referenceThresholdLow[t]==referenceThresholdLow[t])
            REG->SetReferenceThresholdLow(t,referenceThresholdLow[t]);

    for(unsigned int t=0;t<(unsigned int)floatingImage->nt;t++)
        if(floatingThresholdUp[t]==floatingThresholdUp[t])
            REG->SetFloatingThresholdUp(t,floatingThresholdUp[t]);

    for(unsigned int t=0;t<(unsigned int)floatingImage->nt;t++)
        if(floatingThresholdLow[t]==floatingThresholdLow[t])
            REG->SetFloatingThresholdLow(t,floatingThresholdLow[t]);

    for(unsigned int t=0;t<(unsigned int)referenceImage->nt;t++)
        if(referenceBinNumber[t]>0)
            REG->SetReferenceBinNumber(t,referenceBinNumber[t]);

    for(unsigned int t=0;t<(unsigned int)floatingImage->nt;t++)
        if(floatingBinNumber[t]>0)
            REG->SetFloatingBinNumber(t,floatingBinNumber[t]);

    if(warpedPaddingValue==warpedPaddingValue)
        REG->SetWarpedPaddingValue(warpedPaddingValue);

    for(unsigned int s=0;s<3;s++)
        if(spacing[s]==spacing[s])
            REG->SetSpacing(s,spacing[s]);

    if(levelNumber>0)
        REG->SetLevelNumber(levelNumber);

    if(levelToPerform>0)
        REG->SetLevelToPerform(levelToPerform);

    if(gradientSmoothingSigma==gradientSmoothingSigma)
        REG->SetGradientSmoothingSigma(gradientSmoothingSigma);
	
    if(useSSD)
        REG->UseSSD();
    else REG->DoNotUseSSD();

    if(useConjugate==true)
        REG->UseConjugateGradient();
    else REG->DoNotUseConjugateGradient();

    if(noPyramid==1)
        REG->DoNotUsePyramidalApproach();

#ifdef _NR_DEV
    if(stepNumber>0)
        REG->SetCompositionStepNumber(stepNumber);

    if(approxComposition)
        REG->ApproximateComposition();

    if(useSymmetry)
        REG->UseSimilaritySymmetry();
#endif

    // Run the registration
#ifdef _USE_CUDA
    if(useGPU && checkMem){
        int requiredMemory = REG->CheckMemoryMB_f3d();
        printf("[NiftyReg F3D] The registration require %i MB on the GPU and %i MB are available\n", requiredMemory, (int)(gpuMemoryAvailable/1000000.f));
    }
    else{
#endif
        REG->Run_f3d();

        // Save the control point result
        nifti_image *outputControlPointGridImage = REG->GetControlPointPositionImage();
        if(outputControlPointGridName==NULL) outputControlPointGridName=(char *)"outputCPP.nii";
        nifti_set_filenames(outputControlPointGridImage, outputControlPointGridName, 0, 0);
        memset(outputControlPointGridImage->descrip, 0, 80);
        strcpy (outputControlPointGridImage->descrip,"Control point position from NiftyReg (reg_f3d)");
#ifdef _NR_DEV
        if(useVel)
            strcpy (outputControlPointGridImage->descrip,"Velocity field grid from NiftyReg (reg_f3d2)");
#endif
        nifti_image_write(outputControlPointGridImage);
        nifti_image_free(outputControlPointGridImage);outputControlPointGridImage=NULL;

        // Save the warped image result
        nifti_image *outputWarpedImage = REG->GetWarpedImage();
        if(outputWarpedName==NULL) outputWarpedName=(char *)"outputResult.nii";
        nifti_set_filenames(outputWarpedImage, outputWarpedName, 0, 0);
        memset(outputWarpedImage->descrip, 0, 80);
        strcpy (outputWarpedImage->descrip,"Warped image using NiftyReg (reg_f3d)");
#ifdef _NR_DEV
        if(useVel)
            strcpy (outputWarpedImage->descrip,"Warped image using NiftyReg (reg_f3d2)");
#endif
        nifti_image_write(outputWarpedImage);
        nifti_image_free(outputWarpedImage);outputWarpedImage=NULL;
#ifdef _USE_CUDA
    }
#endif
    // Erase the registration object
    delete REG;

    // Clean the allocated images
    if(referenceImage!=NULL) nifti_image_free(referenceImage);
    if(floatingImage!=NULL) nifti_image_free(floatingImage);
    if(controlPointGridImage!=NULL) nifti_image_free(controlPointGridImage);
    if(affineTransformation!=NULL) free(affineTransformation);
    if(referenceMaskImage!=NULL) nifti_image_free(referenceMaskImage);

#ifdef NDEBUG
    if(verbose){
#endif
        time_t end; time( &end );
        int minutes = (int)floorf(float(end-start)/60.0f);
        int seconds = (int)(end-start - 60*minutes);
        printf("[NiftyReg F3D] Registration Performed in %i min %i sec\n", minutes, seconds);
        printf("[NiftyReg F3D] Have a good day !\n");
#ifdef NDEBUG
    }
#endif
	return 0;
}
