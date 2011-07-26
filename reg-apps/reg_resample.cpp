/*
 *  reg_resample.cpp
 *
 *
 *  Created by Marc Modat on 18/05/2009.
 *  Copyright (c) 2009, University College London. All rights reserved.
 *  Centre for Medical Image Computing (CMIC)
 *  See the LICENSE.txt file in the nifty_reg root folder
 *
 */

#ifndef _MM_RESAMPLE_CPP
#define _MM_RESAMPLE_CPP

#include "_reg_resampling.h"
#include "_reg_globalTransformation.h"
#include "_reg_localTransformation.h"
#include "_reg_tools.h"

#ifdef _USE_NR_DOUBLE
    #define PrecisionTYPE double
#else
    #define PrecisionTYPE float
#endif

typedef struct{
    char *referenceImageName;
    char *sourceImageName;
    char *affineMatrixName;
    char *inputCPPName;
    char *inputDEFName;
    char *outputResultName;
    char *outputBlankName;
    PrecisionTYPE sourceBGValue;
}PARAM;
typedef struct{
    bool referenceImageFlag;
    bool sourceImageFlag;
    bool affineMatrixFlag;
    bool affineFlirtFlag;
    bool inputCPPFlag;
    bool inputDEFFlag;
    bool outputResultFlag;
    bool outputBlankFlag;
    bool NNInterpolationFlag;
    bool TRIInterpolationFlag;
}FLAG;


void PetitUsage(char *exec)
{
    fprintf(stderr,"Usage:\t%s -target <referenceImageName> -source <sourceImageName> [OPTIONS].\n",exec);
    fprintf(stderr,"\tSee the help for more details (-h).\n");
    return;
}
void Usage(char *exec)
{
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("Usage:\t%s -target <filename> -source <filename> [OPTIONS].\n",exec);
    printf("\t-target <filename>\tFilename of the target image (mandatory)\n");
    printf("\t-source <filename>\tFilename of the source image (mandatory)\n\n");
#ifdef _SVN_REV
    fprintf(stderr,"\n-v Print the subversion revision number\n");
#endif

    printf("* * OPTIONS * *\n");
    printf("\t*\tOnly one of the following tranformation is taken into account\n");
    printf("\t-aff <filename>\t\tFilename which contains an affine transformation (Affine*Target=Source)\n");
    printf("\t-affFlirt <filename>\t\tFilename which contains a radiological flirt affine transformation\n");
    printf("\t-cpp <filename>\t\tFilename of the control point grid image (from reg_f3d)\n");
    printf("\t-def <filename>\t\tFilename of the deformation field image (from reg_transform)\n");

    printf("\t*\tThere are no limit for the required output number from the following\n");
    printf("\t-result <filename> \tFilename of the resampled image [none]\n");
    printf("\t-blank <filename> \tFilename of the resampled blank grid [none]\n");

    printf("\t*\tOthers\n");
    printf("\t-NN \t\t\tUse a Nearest Neighbor interpolation for the source resampling (cubic spline by default)\n");
    printf("\t-TRI \t\t\tUse a Trilinear interpolation for the source resampling (cubic spline by default)\n");
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    return;
}

int main(int argc, char **argv)
{
    PARAM *param = (PARAM *)calloc(1,sizeof(PARAM));
    FLAG *flag = (FLAG *)calloc(1,sizeof(FLAG));

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
        else if(strcmp(argv[i], "-target") == 0){
            param->referenceImageName=argv[++i];
            flag->referenceImageFlag=1;
        }
        else if(strcmp(argv[i], "-source") == 0){
            param->sourceImageName=argv[++i];
            flag->sourceImageFlag=1;
        }
        else if(strcmp(argv[i], "-aff") == 0){
            param->affineMatrixName=argv[++i];
            flag->affineMatrixFlag=1;
        }
        else if(strcmp(argv[i], "-affFlirt") == 0){
            param->affineMatrixName=argv[++i];
            flag->affineMatrixFlag=1;
            flag->affineFlirtFlag=1;
        }
        else if(strcmp(argv[i], "-result") == 0){
                param->outputResultName=argv[++i];
                flag->outputResultFlag=1;
        }
        else if(strcmp(argv[i], "-cpp") == 0){
            param->inputCPPName=argv[++i];
            flag->inputCPPFlag=1;
        }
        else if(strcmp(argv[i], "-def") == 0){
            param->inputDEFName=argv[++i];
            flag->inputDEFFlag=1;
        }
        else if(strcmp(argv[i], "-NN") == 0){
            flag->NNInterpolationFlag=1;
        }
        else if(strcmp(argv[i], "-TRI") == 0){
            flag->TRIInterpolationFlag=1;
        }
        else if(strcmp(argv[i], "-blank") == 0){
            param->outputBlankName=argv[++i];
            flag->outputBlankFlag=1;
        }
        else{
            fprintf(stderr,"Err:\tParameter %s unknown.\n",argv[i]);
            PetitUsage(argv[0]);
            return 1;
        }
    }

    if(!flag->referenceImageFlag || !flag->sourceImageFlag){
    fprintf(stderr,"[NiftyReg ERROR] The target and the source image have both to be defined.\n");
            PetitUsage(argv[0]);
            return 1;
    }
	
    /* Check the number of input images */
    if(((unsigned int)flag->affineMatrixFlag +
        (unsigned int)flag->inputCPPFlag +
        (unsigned int)flag->inputDEFFlag) > 1){
        fprintf(stderr,"[NiftyReg ERROR] Only one input transformation has to be assigned.\n");
        PetitUsage(argv[0]);
        return 1;
    }

	/* Read the target image */
    nifti_image *referenceImage = nifti_image_read(param->referenceImageName,false);
    if(referenceImage == NULL){
        fprintf(stderr,"[NiftyReg ERROR] Error when reading the target image: %s\n",
                param->referenceImageName);
        return 1;
    }
    reg_checkAndCorrectDimension(referenceImage);
	
    /* Read the source image */
    nifti_image *sourceImage = nifti_image_read(param->sourceImageName,true);
    if(sourceImage == NULL){
        fprintf(stderr,"[NiftyReg ERROR] Error when reading the source image: %s\n",
                param->sourceImageName);
        return 1;
    }
    reg_checkAndCorrectDimension(sourceImage);

    /* *********************************** */
    /* DISPLAY THE RESAMPLING PARAMETERS */
    /* *********************************** */
    printf("\n* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n");
    printf("Command line:\n");
    for(int i=0;i<argc;i++) printf(" %s", argv[i]);
    printf("\n\n");
    printf("Parameters\n");
    printf("Target image name: %s\n",referenceImage->fname);
    printf("\t%ix%ix%i voxels, %i volumes\n",referenceImage->nx,referenceImage->ny,referenceImage->nz,referenceImage->nt);
    printf("\t%gx%gx%g mm\n",referenceImage->dx,referenceImage->dy,referenceImage->dz);
    printf("Source image name: %s\n",sourceImage->fname);
    printf("\t%ix%ix%i voxels, %i volumes\n",sourceImage->nx,sourceImage->ny,sourceImage->nz,sourceImage->nt);
    printf("\t%gx%gx%g mm\n",sourceImage->dx,sourceImage->dy,sourceImage->dz);
    printf("* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * *\n\n");

    /* *********************** */
    /* READ THE TRANSFORMATION */
    /* *********************** */
    nifti_image *controlPointImage = NULL;
    nifti_image *deformationFieldImage = NULL;
    mat44 *affineTransformationMatrix = (mat44 *)calloc(1,sizeof(mat44));if(flag->inputCPPFlag){
#ifndef NDEBUG
        printf("[NiftyReg DEBUG] Name of the control point image: %s\n", param->inputCPPName);
#endif
        controlPointImage = nifti_image_read(param->inputCPPName,true);
        if(controlPointImage == NULL){
            fprintf(stderr,"[NiftyReg ERROR] Error when reading the control point image: %s\n",param->inputCPPName);
            return 1;
        }
        reg_checkAndCorrectDimension(controlPointImage);
    }
    else if(flag->inputDEFFlag){
#ifndef NDEBUG
        printf("[NiftyReg DEBUG] Name of the deformation field image: %s\n", param->inputDEFName);
#endif
        deformationFieldImage = nifti_image_read(param->inputDEFName,true);
        if(deformationFieldImage == NULL){
            fprintf(stderr,"[NiftyReg ERROR] Error when reading the deformation field image: %s\n",param->inputDEFName);
            return 1;
        }
        reg_checkAndCorrectDimension(deformationFieldImage);
    }
    else if(flag->affineMatrixFlag){
#ifndef NDEBUG
        printf("[NiftyReg DEBUG] Name of affine transformation: %s\n", param->affineMatrixName);
#endif
        // Check first if the specified affine file exist
        if(FILE *aff=fopen(param->affineMatrixName, "r")){
            fclose(aff);
        }
        else{
            fprintf(stderr,"The specified input affine file (%s) can not be read\n",param->affineMatrixName);
            return 1;
        }
        reg_tool_ReadAffineFile(	affineTransformationMatrix,
                                    referenceImage,
                                    sourceImage,
                                    param->affineMatrixName,
                                    flag->affineFlirtFlag);
    }
    else{
        // identity transformation is considered
        affineTransformationMatrix->m[0][0]=1.0;
        affineTransformationMatrix->m[1][1]=1.0;
        affineTransformationMatrix->m[2][2]=1.0;
        affineTransformationMatrix->m[3][3]=1.0;
    }

    // Allocate and copmute the deformation field if necessary
    if(!flag->inputDEFFlag){
#ifndef NDEBUG
        printf("[NiftyReg DEBUG] Allocation of the deformation field\n");
#endif
        // Allocate
        deformationFieldImage = nifti_copy_nim_info(referenceImage);
        deformationFieldImage->dim[0]=deformationFieldImage->ndim=5;
        deformationFieldImage->dim[1]=deformationFieldImage->nx=referenceImage->nx;
        deformationFieldImage->dim[2]=deformationFieldImage->ny=referenceImage->ny;
        deformationFieldImage->dim[3]=deformationFieldImage->nz=referenceImage->nz;
        deformationFieldImage->dim[4]=deformationFieldImage->nt=1;deformationFieldImage->pixdim[4]=deformationFieldImage->dt=1.0;
        if(referenceImage->nz>1) deformationFieldImage->dim[5]=deformationFieldImage->nu=3;
        else deformationFieldImage->dim[5]=deformationFieldImage->nu=2;
        deformationFieldImage->pixdim[5]=deformationFieldImage->du=1.0;
        deformationFieldImage->dim[6]=deformationFieldImage->nv=1;deformationFieldImage->pixdim[6]=deformationFieldImage->dv=1.0;
        deformationFieldImage->dim[7]=deformationFieldImage->nw=1;deformationFieldImage->pixdim[7]=deformationFieldImage->dw=1.0;
        deformationFieldImage->nvox=deformationFieldImage->nx*deformationFieldImage->ny*deformationFieldImage->nz*deformationFieldImage->nt*deformationFieldImage->nu;
        if(sizeof(PrecisionTYPE)==8) deformationFieldImage->datatype = NIFTI_TYPE_FLOAT64;
        else deformationFieldImage->datatype = NIFTI_TYPE_FLOAT32;
        deformationFieldImage->nbyper = sizeof(PrecisionTYPE);
        deformationFieldImage->data = (void *)calloc(deformationFieldImage->nvox, deformationFieldImage->nbyper);
        //Computation
        if(flag->inputCPPFlag){
#ifndef NDEBUG
            printf("[NiftyReg DEBUG] Computation of the deformation field from the CPP image\n");
#endif
            if(controlPointImage->pixdim[5]>1){
                reg_getDeformationFieldFromVelocityGrid(controlPointImage,
                                                        deformationFieldImage,
                                                        NULL, // intermediate
                                                        NULL, // mask
                                                        false // approximation
                                                        );
            }
            else{
                reg_spline_getDeformationField(controlPointImage,
                                               referenceImage,
                                               deformationFieldImage,
                                               NULL, // mask
                                               false, //composition
                                               true // bspline
                                               );
            }
        }
        else{
#ifndef NDEBUG
            printf("[NiftyReg DEBUG] Computation of the deformation field from the affine transformation\n");
#endif
            reg_affine_positionField(   affineTransformationMatrix,
                                        referenceImage,
                                        deformationFieldImage);
        }
    }

    /* ************************* */
    /* RESAMPLE THE SOURCE IMAGE */
    /* ************************* */
    if(flag->outputResultFlag){
        int inter=3;
        if(flag->TRIInterpolationFlag) inter=1;
        else if(flag->NNInterpolationFlag) inter=0;

        nifti_image *resultImage = nifti_copy_nim_info(referenceImage);
        resultImage->dim[0]=resultImage->ndim=sourceImage->dim[0];
        resultImage->dim[4]=resultImage->nt=sourceImage->dim[4];
        resultImage->cal_min=sourceImage->cal_min;
        resultImage->cal_max=sourceImage->cal_max;
        resultImage->scl_slope=sourceImage->scl_slope;
        resultImage->scl_inter=sourceImage->scl_inter;
        resultImage->datatype = sourceImage->datatype;
        resultImage->nbyper = sourceImage->nbyper;
        resultImage->nvox = resultImage->dim[1] * resultImage->dim[2] * resultImage->dim[3] * resultImage->dim[4];
        resultImage->data = (void *)calloc(resultImage->nvox, resultImage->nbyper);

        reg_resampleSourceImage(referenceImage,
                                        sourceImage,
                                        resultImage,
                                        deformationFieldImage,
                                        NULL,
                                        inter,
                                        0);
        nifti_set_filenames(resultImage, param->outputResultName, 0, 0);
        memset(resultImage->descrip, 0, 80);
        strcpy (resultImage->descrip,"Warped image using NiftyReg (reg_resample)");
        nifti_image_write(resultImage);
        printf("[NiftyReg] Resampled image has been saved: %s\n", param->outputResultName);
        nifti_image_free(resultImage);
    }

    /* *********************** */
    /* RESAMPLE A REGULAR GRID */
    /* *********************** */
    if(flag->outputBlankFlag){
        nifti_image *gridImage = nifti_copy_nim_info(sourceImage);
        gridImage->cal_min=0;
        gridImage->cal_max=255;
        gridImage->datatype = NIFTI_TYPE_UINT8;
        gridImage->nbyper = sizeof(unsigned char);
        gridImage->data = (void *)calloc(gridImage->nvox, gridImage->nbyper);
        unsigned char *gridImageValuePtr = static_cast<unsigned char *>(gridImage->data);
        for(int z=0; z<gridImage->nz;z++){
            for(int y=0; y<gridImage->ny;y++){
                for(int x=0; x<gridImage->nx;x++){
                    if(referenceImage->nz>1){
                        if( x/10==(float)x/10.0 || y/10==(float)y/10.0 || z/10==(float)z/10.0)
                            *gridImageValuePtr = 255;
                    }
                    else{
                        if( x/10==(float)x/10.0 || x==referenceImage->nx-1 || y/10==(float)y/10.0 || y==referenceImage->ny-1)
                            *gridImageValuePtr = 255;
                    }
                    gridImageValuePtr++;
                }
            }
        }

        nifti_image *resultImage = nifti_copy_nim_info(referenceImage);
        resultImage->dim[0]=resultImage->ndim=3;
        resultImage->dim[4]=resultImage->nt=1;
        resultImage->cal_min=sourceImage->cal_min;
        resultImage->cal_max=sourceImage->cal_max;
        resultImage->scl_slope=sourceImage->scl_slope;
        resultImage->scl_inter=sourceImage->scl_inter;
        resultImage->datatype =NIFTI_TYPE_UINT8;
        resultImage->nbyper = sizeof(unsigned char);
        resultImage->data = (void *)calloc(resultImage->nvox, resultImage->nbyper);
        reg_resampleSourceImage(referenceImage,
                                        gridImage,
                                        resultImage,
                                        deformationFieldImage,
                                        NULL,
                                        1,
                                       0);
        nifti_set_filenames(resultImage, param->outputBlankName, 0, 0);
        memset(resultImage->descrip, 0, 80);
        strcpy (resultImage->descrip,"Warped regular grid using NiftyReg (reg_resample)");
        nifti_image_write(resultImage);
        nifti_image_free(resultImage);
        nifti_image_free(gridImage);
        printf("[NiftyReg] Resampled grid has been saved: %s\n", param->outputBlankName);
    }

    nifti_image_free(referenceImage);
    nifti_image_free(sourceImage);
    nifti_image_free(controlPointImage);
    nifti_image_free(deformationFieldImage);
    free(affineTransformationMatrix);

	
	free(flag);
	free(param);
	
	return 0;
}

#endif