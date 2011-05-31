/*
 *  _reg_localTransformation_be.cpp
 *  
 *
 *  Created by Marc Modat on 10/05/2011.
 *  Copyright (c) 2009, University College London. All rights reserved.
 *  Centre for Medical Image Computing (CMIC)
 *  See the LICENSE.txt file in the nifty_reg root folder
 *
 */

#include "_reg_localTransformation.h"

/* *************************************************************** */
/* *************************************************************** */
template<class SplineTYPE>
double reg_bspline_bendingEnergyApproxValue2D(nifti_image *splineControlPoint)
{
    SplineTYPE *controlPointPtrX = static_cast<SplineTYPE *>(splineControlPoint->data);
    SplineTYPE *controlPointPtrY = &controlPointPtrX[splineControlPoint->nx*splineControlPoint->ny];

    // As the contraint is only computed at the control point positions, the basis value of the spline are always the same
    SplineTYPE basisXX[9], basisYY[9], basisXY[9];
    SplineTYPE normal[3]={1.0/6.0, 2.0/3.0, 1.0/6.0};
    SplineTYPE first[3]={-0.5, 0, 0.5};
    SplineTYPE second[3]={1.0, -2.0, 1.0};
    int coord = 0;
    for(int b=0; b<3; b++){
        for(int a=0; a<3; a++){
            basisXX[coord] = second[a] * normal[b];
            basisYY[coord] = normal[a] * second[b];
            basisXY[coord] = first[a] * first[b];
            coord++;
        }
    }

    SplineTYPE constraintValue=0.0;

    SplineTYPE xControlPointCoordinates[9];
    SplineTYPE yControlPointCoordinates[9];

    for(int y=1;y<splineControlPoint->ny-1;y++){
        for(int x=1;x<splineControlPoint->nx-1;x++){

            get_GridValuesApprox<SplineTYPE>(x-1,
                                             y-1,
                                             splineControlPoint,
                                             controlPointPtrX,
                                             controlPointPtrY,
                                             xControlPointCoordinates,
                                             yControlPointCoordinates,
                                             false);

            SplineTYPE XX_x=0.0;
            SplineTYPE YY_x=0.0;
            SplineTYPE XY_x=0.0;
            SplineTYPE XX_y=0.0;
            SplineTYPE YY_y=0.0;
            SplineTYPE XY_y=0.0;

            for(int a=0; a<9; a++){
                XX_x += basisXX[a]*xControlPointCoordinates[a];
                YY_x += basisYY[a]*xControlPointCoordinates[a];
                XY_x += basisXY[a]*xControlPointCoordinates[a];

                XX_y += basisXX[a]*yControlPointCoordinates[a];
                YY_y += basisYY[a]*yControlPointCoordinates[a];
                XY_y += basisXY[a]*yControlPointCoordinates[a];
            }

            constraintValue += (double)(XX_x*XX_x + YY_x*YY_x + 2.0*XY_x*XY_x);
            constraintValue += (double)(XX_y*XX_y + YY_y*YY_y + 2.0*XY_y*XY_y);
        }
    }
    return constraintValue/(double)(2.0*splineControlPoint->nx*splineControlPoint->ny);
}
/* *************************************************************** */
template<class SplineTYPE>
double reg_bspline_bendingEnergyApproxValue3D(nifti_image *splineControlPoint)
{
    SplineTYPE *controlPointPtrX = static_cast<SplineTYPE *>
       (splineControlPoint->data);
    SplineTYPE *controlPointPtrY = static_cast<SplineTYPE *>
       (&controlPointPtrX[splineControlPoint->nx*splineControlPoint->ny*splineControlPoint->nz]);
    SplineTYPE *controlPointPtrZ = static_cast<SplineTYPE *>
       (&controlPointPtrY[splineControlPoint->nx*splineControlPoint->ny*splineControlPoint->nz]);

    // As the contraint is only computed at the control point positions, the basis value of the spline are always the same
    SplineTYPE basisXX[27], basisYY[27], basisZZ[27], basisXY[27], basisYZ[27], basisXZ[27];
    SplineTYPE normal[3]={1.0/6.0, 2.0/3.0, 1.0/6.0};
    SplineTYPE first[3]={-0.5, 0, 0.5};
    SplineTYPE second[3]={1.0, -2.0, 1.0};
    // There are six different values taken into account
    SplineTYPE tempXX[9], tempYY[9], tempZZ[9], tempXY[9], tempYZ[9], tempXZ[9];
    int coord=0;
    for(int c=0; c<3; c++){
        for(int b=0; b<3; b++){
            tempXX[coord]=normal[c]*normal[b];  // z * y
            tempYY[coord]=normal[c]*second[b];  // z * y"
            tempZZ[coord]=second[c]*normal[b];  // z"* y
            tempXY[coord]=normal[c]*first[b];   // z * y'
            tempYZ[coord]=first[c]*first[b];    // z'* y'
            tempXZ[coord]=first[c]*normal[b];   // z'* y
            coord++;
        }
    }
    coord=0;
    for(int bc=0; bc<9; bc++){
        for(int a=0; a<3; a++){
            basisXX[coord]=tempXX[bc]*second[a];    // z * y * x"
            basisYY[coord]=tempYY[bc]*normal[a];    // z * y"* x
            basisZZ[coord]=tempZZ[bc]*normal[a];    // z"* y * x
            basisXY[coord]=tempXY[bc]*first[a];     // z * y'* x'
            basisYZ[coord]=tempYZ[bc]*normal[a];    // z'* y'* x
            basisXZ[coord]=tempXZ[bc]*first[a];     // z'* y * x'
            coord++;
        }
    }

    double constraintValue=0.0;

    SplineTYPE xControlPointCoordinates[27];
    SplineTYPE yControlPointCoordinates[27];
    SplineTYPE zControlPointCoordinates[27];

    for(int z=1;z<splineControlPoint->nz-1;z++){
        for(int y=1;y<splineControlPoint->ny-1;y++){
            for(int x=1;x<splineControlPoint->nx-1;x++){

                get_GridValuesApprox<SplineTYPE>(x-1,
                                                 y-1,
                                                 z-1,
                                                 splineControlPoint,
                                                 controlPointPtrX,
                                                 controlPointPtrY,
                                                 controlPointPtrZ,
                                                 xControlPointCoordinates,
                                                 yControlPointCoordinates,
                                                 zControlPointCoordinates,
                                                 false);

                SplineTYPE XX_x=0.0, YY_x=0.0, ZZ_x=0.0;
                SplineTYPE XY_x=0.0, YZ_x=0.0, XZ_x=0.0;
                SplineTYPE XX_y=0.0, YY_y=0.0, ZZ_y=0.0;
                SplineTYPE XY_y=0.0, YZ_y=0.0, XZ_y=0.0;
                SplineTYPE XX_z=0.0, YY_z=0.0, ZZ_z=0.0;
                SplineTYPE XY_z=0.0, YZ_z=0.0, XZ_z=0.0;

                for(int a=0; a<27; a++){
                    XX_x += basisXX[a]*xControlPointCoordinates[a];
                    YY_x += basisYY[a]*xControlPointCoordinates[a];
                    ZZ_x += basisZZ[a]*xControlPointCoordinates[a];
                    XY_x += basisXY[a]*xControlPointCoordinates[a];
                    YZ_x += basisYZ[a]*xControlPointCoordinates[a];
                    XZ_x += basisXZ[a]*xControlPointCoordinates[a];

                    XX_y += basisXX[a]*yControlPointCoordinates[a];
                    YY_y += basisYY[a]*yControlPointCoordinates[a];
                    ZZ_y += basisZZ[a]*yControlPointCoordinates[a];
                    XY_y += basisXY[a]*yControlPointCoordinates[a];
                    YZ_y += basisYZ[a]*yControlPointCoordinates[a];
                    XZ_y += basisXZ[a]*yControlPointCoordinates[a];

                    XX_z += basisXX[a]*zControlPointCoordinates[a];
                    YY_z += basisYY[a]*zControlPointCoordinates[a];
                    ZZ_z += basisZZ[a]*zControlPointCoordinates[a];
                    XY_z += basisXY[a]*zControlPointCoordinates[a];
                    YZ_z += basisYZ[a]*zControlPointCoordinates[a];
                    XZ_z += basisXZ[a]*zControlPointCoordinates[a];
                }

                constraintValue += (double)(XX_x*XX_x + YY_x*YY_x + ZZ_x*ZZ_x + 2.0*(XY_x*XY_x + YZ_x*YZ_x + XZ_x*XZ_x));
                constraintValue += (double)(XX_y*XX_y + YY_y*YY_y + ZZ_y*ZZ_y + 2.0*(XY_y*XY_y + YZ_y*YZ_y + XZ_y*XZ_y));
                constraintValue += (double)(XX_z*XX_z + YY_z*YY_z + ZZ_z*ZZ_z + 2.0*(XY_z*XY_z + YZ_z*YZ_z + XZ_z*XZ_z));
            }
        }
    }

    return constraintValue/(double)(3.0*splineControlPoint->nx*splineControlPoint->ny*splineControlPoint->nz);
}
/* *************************************************************** */
extern "C++"
double reg_bspline_bendingEnergy(nifti_image *splineControlPoint)
{
    if(splineControlPoint->nz==1){
        switch(splineControlPoint->datatype){
            case NIFTI_TYPE_FLOAT32:
                return reg_bspline_bendingEnergyApproxValue2D<float>(splineControlPoint);
#ifdef _NR_DEV
            case NIFTI_TYPE_FLOAT64:
                    return reg_bspline_bendingEnergyApproxValue2D<double>(splineControlPoint);
#endif
            default:
                fprintf(stderr,"[NiftyReg ERROR] Only single or double precision is implemented for the bending energy\n");
                fprintf(stderr,"[NiftyReg ERROR] The bending energy is not computed\n");
                exit(1);
        }
    }
    else{
        switch(splineControlPoint->datatype){
            case NIFTI_TYPE_FLOAT32:
                    return reg_bspline_bendingEnergyApproxValue3D<float>(splineControlPoint);
#ifdef _NR_DEV
            case NIFTI_TYPE_FLOAT64:
                    return reg_bspline_bendingEnergyApproxValue3D<double>(splineControlPoint);
#endif
            default:
                fprintf(stderr,"[NiftyReg ERROR] Only single or double precision is implemented for the bending energy\n");
                fprintf(stderr,"[NiftyReg ERROR] The bending energy is not computed\n");
                exit(1);
        }

    }
}
/* *************************************************************** */
/* *************************************************************** */
template<class SplineTYPE>
void reg_bspline_approxBendingEnergyGradient2D(   nifti_image *splineControlPoint,
                                            nifti_image *targetImage,
                                            nifti_image *gradientImage,
                                            float weight)
{
    // As the contraint is only computed at the voxel position, the basis value of the spline are always the same
    SplineTYPE basisXX[9], basisYY[9], basisXY[9];
    SplineTYPE normal[3]={1.0/6.0, 2.0/3.0, 1.0/6.0};
    SplineTYPE first[3]={-0.5, 0, 0.5};
    SplineTYPE second[3]={1.0, -2.0, 1.0};
    int coord = 0;
    for(int b=0; b<3; b++){
        for(int a=0; a<3; a++){
            basisXX[coord] = second[a] * normal[b];
            basisYY[coord] = normal[a] * second[b];
            basisXY[coord] = first[a] * first[b];
            coord++;
        }
    }

    SplineTYPE nodeNumber = (SplineTYPE)(splineControlPoint->nx*splineControlPoint->ny);
    SplineTYPE *derivativeValues = (SplineTYPE *)calloc(6*(int)nodeNumber, sizeof(SplineTYPE));

    SplineTYPE *controlPointPtrX = static_cast<SplineTYPE *>(splineControlPoint->data);
    SplineTYPE *controlPointPtrY = static_cast<SplineTYPE *>(&controlPointPtrX[(int)nodeNumber]);

    SplineTYPE xControlPointCoordinates[9];
    SplineTYPE yControlPointCoordinates[9];

    SplineTYPE *derivativeValuesPtr = &derivativeValues[0];

    for(int y=1;y<splineControlPoint->ny-1;y++){
        derivativeValuesPtr = &derivativeValues[6*(y*splineControlPoint->nx+1)];
        for(int x=1;x<splineControlPoint->nx-1;x++){

            get_GridValuesApprox<SplineTYPE>(x-1,
                                             y-1,
                                             splineControlPoint,
                                             controlPointPtrX,
                                             controlPointPtrY,
                                             xControlPointCoordinates,
                                             yControlPointCoordinates,
                                             false);

            SplineTYPE XX_x=0.0;
            SplineTYPE YY_x=0.0;
            SplineTYPE XY_x=0.0;
            SplineTYPE XX_y=0.0;
            SplineTYPE YY_y=0.0;
            SplineTYPE XY_y=0.0;

            for(int a=0; a<9; a++){
                XX_x += basisXX[a]*xControlPointCoordinates[a];
                YY_x += basisYY[a]*xControlPointCoordinates[a];
                XY_x += basisXY[a]*xControlPointCoordinates[a];

                XX_y += basisXX[a]*yControlPointCoordinates[a];
                YY_y += basisYY[a]*yControlPointCoordinates[a];
                XY_y += basisXY[a]*yControlPointCoordinates[a];
            }
            *derivativeValuesPtr++ = (SplineTYPE)(2.0*XX_x);
            *derivativeValuesPtr++ = (SplineTYPE)(2.0*YY_x);
            *derivativeValuesPtr++ = (SplineTYPE)(4.0*XY_x);
            *derivativeValuesPtr++ = (SplineTYPE)(2.0*XX_y);
            *derivativeValuesPtr++ = (SplineTYPE)(2.0*YY_y);
            *derivativeValuesPtr++ = (SplineTYPE)(4.0*XY_y);
        }
    }

    SplineTYPE *gradientXPtr = static_cast<SplineTYPE *>(gradientImage->data);
    SplineTYPE *gradientYPtr = static_cast<SplineTYPE *>(&gradientXPtr[(int)nodeNumber]);

    SplineTYPE approxRatio= weight * (SplineTYPE)(targetImage->nx*targetImage->ny)
    / nodeNumber;

    SplineTYPE gradientValue[2];

    for(int y=0;y<splineControlPoint->ny;y++){
        for(int x=0;x<splineControlPoint->nx;x++){

            gradientValue[0]=gradientValue[1]=0.0;

            unsigned int coord=0;
            for(int Y=y-1; Y<y+2; Y++){
                for(int X=x-1; X<x+2; X++){
                    if(-1<X && -1<Y && X<splineControlPoint->nx && Y<splineControlPoint->ny){
                        derivativeValuesPtr = &derivativeValues[6 * (Y*splineControlPoint->nx + X)];
                        gradientValue[0] += (*derivativeValuesPtr++) * basisXX[coord];
                        gradientValue[0] += (*derivativeValuesPtr++) * basisYY[coord];
                        gradientValue[0] += (*derivativeValuesPtr++) * basisXY[coord];

                        gradientValue[1] += (*derivativeValuesPtr++) * basisXX[coord];
                        gradientValue[1] += (*derivativeValuesPtr++) * basisYY[coord];
                        gradientValue[1] += (*derivativeValuesPtr++) * basisXY[coord];
                    }
                    coord++;
                }
            }
            // (Marc) I removed the normalisation by the voxel number as each gradient has to be normalised in the same way (NMI, BE, JAC)
            *gradientXPtr++ += (SplineTYPE)(approxRatio*gradientValue[0]);
            *gradientYPtr++ += (SplineTYPE)(approxRatio*gradientValue[1]);
        }
    }
    free(derivativeValues);
}
/* *************************************************************** */
template<class SplineTYPE>
void reg_bspline_approxBendingEnergyGradient3D( nifti_image *splineControlPoint,
                                                nifti_image *targetImage,
                                                nifti_image *gradientImage,
                                                float weight)
{
    // As the contraint is only computed at the voxel position, the basis value of the spline are always the same
    SplineTYPE basisXX[27], basisYY[27], basisZZ[27], basisXY[27], basisYZ[27], basisXZ[27];
    SplineTYPE normal[3]={1.0/6.0, 2.0/3.0, 1.0/6.0};
    SplineTYPE first[3]={-0.5, 0, 0.5};
    SplineTYPE second[3]={1.0, -2.0, 1.0};
    // There are six different values taken into account
    SplineTYPE tempXX[9], tempYY[9], tempZZ[9], tempXY[9], tempYZ[9], tempXZ[9];
    int coord=0;
    for(int c=0; c<3; c++){
        for(int b=0; b<3; b++){
            tempXX[coord]=normal[c]*normal[b];  // z * y
            tempYY[coord]=normal[c]*second[b];  // z * y"
            tempZZ[coord]=second[c]*normal[b];  // z"* y
            tempXY[coord]=normal[c]*first[b];   // z * y'
            tempYZ[coord]=first[c]*first[b];    // z'* y'
            tempXZ[coord]=first[c]*normal[b];   // z'* y
            coord++;
        }
    }
    coord=0;
    for(int bc=0; bc<9; bc++){
        for(int a=0; a<3; a++){
            basisXX[coord]=tempXX[bc]*second[a];    // z * y * x"
            basisYY[coord]=tempYY[bc]*normal[a];    // z * y"* x
            basisZZ[coord]=tempZZ[bc]*normal[a];    // z"* y * x
            basisXY[coord]=tempXY[bc]*first[a];     // z * y'* x'
            basisYZ[coord]=tempYZ[bc]*normal[a];    // z'* y'* x
            basisXZ[coord]=tempXZ[bc]*first[a];     // z'* y * x'
            coord++;
        }
    }

    SplineTYPE nodeNumber = (SplineTYPE)(splineControlPoint->nx*splineControlPoint->ny*splineControlPoint->nz);
    SplineTYPE *derivativeValues = (SplineTYPE *)calloc(18*(int)nodeNumber, sizeof(SplineTYPE));
    SplineTYPE *derivativeValuesPtr;

    SplineTYPE *controlPointPtrX = static_cast<SplineTYPE *>(splineControlPoint->data);
    SplineTYPE *controlPointPtrY = static_cast<SplineTYPE *>(&controlPointPtrX[(unsigned int)nodeNumber]);
    SplineTYPE *controlPointPtrZ = static_cast<SplineTYPE *>(&controlPointPtrY[(unsigned int)nodeNumber]);

    SplineTYPE xControlPointCoordinates[27];
    SplineTYPE yControlPointCoordinates[27];
    SplineTYPE zControlPointCoordinates[27];

    for(int z=1;z<splineControlPoint->nz-1;z++){
        for(int y=1;y<splineControlPoint->ny-1;y++){
            derivativeValuesPtr = &derivativeValues[18*((z*splineControlPoint->ny+y)*splineControlPoint->nx+1)];
            for(int x=1;x<splineControlPoint->nx-1;x++){

                get_GridValuesApprox<SplineTYPE>(x-1,
                                                 y-1,
                                                 z-1,
                                                 splineControlPoint,
                                                 controlPointPtrX,
                                                 controlPointPtrY,
                                                 controlPointPtrZ,
                                                 xControlPointCoordinates,
                                                 yControlPointCoordinates,
                                                 zControlPointCoordinates,
                                                 false);
                SplineTYPE XX_x=0.0;
                SplineTYPE YY_x=0.0;
                SplineTYPE ZZ_x=0.0;
                SplineTYPE XY_x=0.0;
                SplineTYPE YZ_x=0.0;
                SplineTYPE XZ_x=0.0;
                SplineTYPE XX_y=0.0;
                SplineTYPE YY_y=0.0;
                SplineTYPE ZZ_y=0.0;
                SplineTYPE XY_y=0.0;
                SplineTYPE YZ_y=0.0;
                SplineTYPE XZ_y=0.0;
                SplineTYPE XX_z=0.0;
                SplineTYPE YY_z=0.0;
                SplineTYPE ZZ_z=0.0;
                SplineTYPE XY_z=0.0;
                SplineTYPE YZ_z=0.0;
                SplineTYPE XZ_z=0.0;

                for(int a=0; a<27; a++){
                    XX_x += basisXX[a]*xControlPointCoordinates[a];
                    YY_x += basisYY[a]*xControlPointCoordinates[a];
                    ZZ_x += basisZZ[a]*xControlPointCoordinates[a];
                    XY_x += basisXY[a]*xControlPointCoordinates[a];
                    YZ_x += basisYZ[a]*xControlPointCoordinates[a];
                    XZ_x += basisXZ[a]*xControlPointCoordinates[a];

                    XX_y += basisXX[a]*yControlPointCoordinates[a];
                    YY_y += basisYY[a]*yControlPointCoordinates[a];
                    ZZ_y += basisZZ[a]*yControlPointCoordinates[a];
                    XY_y += basisXY[a]*yControlPointCoordinates[a];
                    YZ_y += basisYZ[a]*yControlPointCoordinates[a];
                    XZ_y += basisXZ[a]*yControlPointCoordinates[a];

                    XX_z += basisXX[a]*zControlPointCoordinates[a];
                    YY_z += basisYY[a]*zControlPointCoordinates[a];
                    ZZ_z += basisZZ[a]*zControlPointCoordinates[a];
                    XY_z += basisXY[a]*zControlPointCoordinates[a];
                    YZ_z += basisYZ[a]*zControlPointCoordinates[a];
                    XZ_z += basisXZ[a]*zControlPointCoordinates[a];
                }
                *derivativeValuesPtr++ = (SplineTYPE)(2.0*XX_x);
                *derivativeValuesPtr++ = (SplineTYPE)(2.0*XX_y);
                *derivativeValuesPtr++ = (SplineTYPE)(2.0*XX_z);
                *derivativeValuesPtr++ = (SplineTYPE)(2.0*YY_x);
                *derivativeValuesPtr++ = (SplineTYPE)(2.0*YY_y);
                *derivativeValuesPtr++ = (SplineTYPE)(2.0*YY_z);
                *derivativeValuesPtr++ = (SplineTYPE)(2.0*ZZ_x);
                *derivativeValuesPtr++ = (SplineTYPE)(2.0*ZZ_y);
                *derivativeValuesPtr++ = (SplineTYPE)(2.0*ZZ_z);
                *derivativeValuesPtr++ = (SplineTYPE)(4.0*XY_x);
                *derivativeValuesPtr++ = (SplineTYPE)(4.0*XY_y);
                *derivativeValuesPtr++ = (SplineTYPE)(4.0*XY_z);
                *derivativeValuesPtr++ = (SplineTYPE)(4.0*YZ_x);
                *derivativeValuesPtr++ = (SplineTYPE)(4.0*YZ_y);
                *derivativeValuesPtr++ = (SplineTYPE)(4.0*YZ_z);
                *derivativeValuesPtr++ = (SplineTYPE)(4.0*XZ_x);
                *derivativeValuesPtr++ = (SplineTYPE)(4.0*XZ_y);
                *derivativeValuesPtr++ = (SplineTYPE)(4.0*XZ_z);
            }
        }
    }

    SplineTYPE *gradientX = static_cast<SplineTYPE *>(gradientImage->data);
    SplineTYPE *gradientY = &gradientX[(int)nodeNumber];
    SplineTYPE *gradientZ = &gradientY[(int)nodeNumber];
    SplineTYPE *gradientXPtr = &gradientX[0];
    SplineTYPE *gradientYPtr = &gradientY[0];
    SplineTYPE *gradientZPtr = &gradientZ[0];

    SplineTYPE approxRatio = weight * (SplineTYPE)(targetImage->nx*targetImage->ny*targetImage->nz)
    / (SplineTYPE)(splineControlPoint->nx*splineControlPoint->ny*splineControlPoint->nz);

    SplineTYPE gradientValue[3];

    for(int z=0;z<splineControlPoint->nz;z++){
        for(int y=0;y<splineControlPoint->ny;y++){
            for(int x=0;x<splineControlPoint->nx;x++){

                gradientValue[0]=gradientValue[1]=gradientValue[2]=0.0;

                unsigned int coord=0;
                for(int Z=z-1; Z<z+2; Z++){
                    for(int Y=y-1; Y<y+2; Y++){
                        for(int X=x-1; X<x+2; X++){
                            if(-1<X && -1<Y && -1<Z && X<splineControlPoint->nx && Y<splineControlPoint->ny && Z<splineControlPoint->nz){
                                derivativeValuesPtr = &derivativeValues[18 * ((Z*splineControlPoint->ny + Y)*splineControlPoint->nx + X)];
                                gradientValue[0] += (*derivativeValuesPtr++) * basisXX[coord];
                                gradientValue[1] += (*derivativeValuesPtr++) * basisXX[coord];
                                gradientValue[2] += (*derivativeValuesPtr++) * basisXX[coord];

                                gradientValue[0] += (*derivativeValuesPtr++) * basisYY[coord];
                                gradientValue[1] += (*derivativeValuesPtr++) * basisYY[coord];
                                gradientValue[2] += (*derivativeValuesPtr++) * basisYY[coord];

                                gradientValue[0] += (*derivativeValuesPtr++) * basisZZ[coord];
                                gradientValue[1] += (*derivativeValuesPtr++) * basisZZ[coord];
                                gradientValue[2] += (*derivativeValuesPtr++) * basisZZ[coord];

                                gradientValue[0] += (*derivativeValuesPtr++) * basisXY[coord];
                                gradientValue[1] += (*derivativeValuesPtr++) * basisXY[coord];
                                gradientValue[2] += (*derivativeValuesPtr++) * basisXY[coord];

                                gradientValue[0] += (*derivativeValuesPtr++) * basisYZ[coord];
                                gradientValue[1] += (*derivativeValuesPtr++) * basisYZ[coord];
                                gradientValue[2] += (*derivativeValuesPtr++) * basisYZ[coord];

                                gradientValue[0] += (*derivativeValuesPtr++) * basisXZ[coord];
                                gradientValue[1] += (*derivativeValuesPtr++) * basisXZ[coord];
                                gradientValue[2] += (*derivativeValuesPtr++) * basisXZ[coord];
                            }
                            coord++;
                        }
                    }
                }
                // (Marc) I removed the normalisation by the voxel number as each gradient has to be normalised in the same way (NMI, BE, JAC)
                *gradientXPtr++ += (SplineTYPE)(approxRatio*gradientValue[0]);
                *gradientYPtr++ += (SplineTYPE)(approxRatio*gradientValue[1]);
                *gradientZPtr++ += (SplineTYPE)(approxRatio*gradientValue[2]);
            }
        }
    }

    free(derivativeValues);
}
/* *************************************************************** */
extern "C++"
void reg_bspline_bendingEnergyGradient( nifti_image *splineControlPoint,
                                        nifti_image *targetImage,
                                        nifti_image *gradientImage,
                                        float weight)
{
    if(splineControlPoint->datatype != gradientImage->datatype){
        fprintf(stderr,"[NiftyReg ERROR] The spline control point image and the gradient image were expected to have the same datatype\n");
        fprintf(stderr,"[NiftyReg ERROR] The bending energy gradient has not computed\n");
        exit(1);
    }
    if(splineControlPoint->nz==1){
        switch(splineControlPoint->datatype){
            case NIFTI_TYPE_FLOAT32:
                reg_bspline_approxBendingEnergyGradient2D<float>
                    (splineControlPoint, targetImage, gradientImage, weight);
                break;
#ifdef _NR_DEV
            case NIFTI_TYPE_FLOAT64:
                reg_bspline_approxBendingEnergyGradient2D<double>
                    (splineControlPoint, targetImage, gradientImage, weight);
                break;
#endif
            default:
                fprintf(stderr,"[NiftyReg ERROR] Only single or double precision is implemented for the bending energy gradient\n");
                fprintf(stderr,"[NiftyReg ERROR] The bending energy gradient has not been computed\n");
                exit(1);
        }
    }
    else{
        switch(splineControlPoint->datatype){
            case NIFTI_TYPE_FLOAT32:
                reg_bspline_approxBendingEnergyGradient3D<float>
                    (splineControlPoint, targetImage, gradientImage, weight);
                break;
#ifdef _NR_DEV
            case NIFTI_TYPE_FLOAT64:
                reg_bspline_approxBendingEnergyGradient3D<double>
                    (splineControlPoint, targetImage, gradientImage, weight);
                break;
#endif
            default:
                fprintf(stderr,"[NiftyReg ERROR] Only single or double precision is implemented for the bending energy gradient\n");
                fprintf(stderr,"[NiftyReg ERROR] The bending energy gradient has not been computed\n");
                exit(1);
        }
    }
}
/* *************************************************************** */
/* *************************************************************** */
template <class DTYPE>
void reg_bspline_linearEnergyApproxValue2D(nifti_image *splineControlPoint, double *constraintValue)
{
    unsigned int nodeNumber = splineControlPoint->nx*splineControlPoint->ny;

    nifti_image *dispControlPoint=nifti_copy_nim_info(splineControlPoint);
    dispControlPoint->data=(void *)malloc(dispControlPoint->nvox*dispControlPoint->nbyper);
    memcpy(dispControlPoint->data,splineControlPoint->data,dispControlPoint->nvox*dispControlPoint->nbyper);
    reg_getDisplacementFromDeformation(dispControlPoint);
    DTYPE *dispPointPtrX = static_cast<DTYPE *>(dispControlPoint->data);
    DTYPE *dispPointPtrY = &dispPointPtrX[nodeNumber];

    DTYPE *jacobianDetermiant=(DTYPE *)malloc(nodeNumber*sizeof(DTYPE));
    mat33 *jacobianMatrices=(mat33 *)malloc(nodeNumber*sizeof(mat33));
    computeApproximateJacobianMatrices_2D(splineControlPoint,
                                          jacobianMatrices,
                                          jacobianDetermiant);

    constraintValue[0]=constraintValue[1]=constraintValue[2]=0;
    mat33 jacobianMatrix;

    for(int y=1;y<splineControlPoint->ny-1;y++){
        unsigned int index=y*splineControlPoint->nx+1;
        for(int x=1;x<splineControlPoint->nx-1;x++){

            jacobianMatrix=jacobianMatrices[index];
            jacobianMatrix.m[0][0]--;
            jacobianMatrix.m[1][1]--;
            constraintValue[0] += (double)(jacobianMatrix.m[0][1]+jacobianMatrix.m[1][0])*(jacobianMatrix.m[0][1]+jacobianMatrix.m[1][0])/2.0;
            constraintValue[1] += (double)(jacobianMatrix.m[0][0]*jacobianMatrix.m[0][0] + jacobianMatrix.m[1][1]*jacobianMatrix.m[1][1]);
            constraintValue[2] += dispPointPtrX[index]*dispPointPtrX[index] + dispPointPtrY[index]*dispPointPtrY[index];
            index++;
        }
    }
    constraintValue[0] += constraintValue[1];
    constraintValue[0] /= (double)(splineControlPoint->nvox);
    constraintValue[1] /= (double)(splineControlPoint->nvox);
    constraintValue[2] /= (double)(splineControlPoint->nvox);
    nifti_image_free(dispControlPoint);
    free(jacobianDetermiant);
    free(jacobianMatrices);
}
/* *************************************************************** */
template <class DTYPE>
void reg_bspline_linearEnergyApproxValue3D(nifti_image *splineControlPoint, double *constraintValue)
{
    unsigned int nodeNumber = splineControlPoint->nx*splineControlPoint->ny*splineControlPoint->nz;

    nifti_image *dispControlPoint=nifti_copy_nim_info(splineControlPoint);
    dispControlPoint->data=(void *)malloc(dispControlPoint->nvox*dispControlPoint->nbyper);
    memcpy(dispControlPoint->data,splineControlPoint->data,dispControlPoint->nvox*dispControlPoint->nbyper);
    reg_getDisplacementFromDeformation(dispControlPoint);
    DTYPE *dispPointPtrX = static_cast<DTYPE *>(dispControlPoint->data);
    DTYPE *dispPointPtrY = &dispPointPtrX[nodeNumber];
    DTYPE *dispPointPtrZ = &dispPointPtrY[nodeNumber];

    DTYPE *jacobianDetermiant=(DTYPE *)malloc(nodeNumber*sizeof(DTYPE));
    mat33 *jacobianMatrices=(mat33 *)malloc(nodeNumber*sizeof(mat33));
    computeApproximateJacobianMatrices_3D(splineControlPoint,
                                          jacobianMatrices,
                                          jacobianDetermiant);

    constraintValue[0]=constraintValue[1]=constraintValue[2]=0;
    mat33 jacobianMatrix;

    for(int z=1;z<splineControlPoint->nz-1;z++){
        for(int y=1;y<splineControlPoint->ny-1;y++){
            unsigned int index=(z*splineControlPoint->ny+y)*splineControlPoint->nx+1;
            for(int x=1;x<splineControlPoint->nx-1;x++){

                jacobianMatrix=jacobianMatrices[index];
                jacobianMatrix.m[0][0]--;
                jacobianMatrix.m[1][1]--;
                jacobianMatrix.m[2][2]--;
                constraintValue[0] += (double).5 * ( (jacobianMatrix.m[0][1]+jacobianMatrix.m[1][0])*(jacobianMatrix.m[0][1]+jacobianMatrix.m[1][0]) +
                                                     (jacobianMatrix.m[0][2]+jacobianMatrix.m[2][0])*(jacobianMatrix.m[0][2]+jacobianMatrix.m[2][0]) +
                                                     (jacobianMatrix.m[1][2]+jacobianMatrix.m[2][1])*(jacobianMatrix.m[1][2]+jacobianMatrix.m[2][1]) );
                constraintValue[1] += (double)(jacobianMatrix.m[0][0]*jacobianMatrix.m[0][0] +
                                               jacobianMatrix.m[1][1]*jacobianMatrix.m[1][1] +
                                               jacobianMatrix.m[2][2]*jacobianMatrix.m[2][2]);
                constraintValue[2] += dispPointPtrX[index]*dispPointPtrX[index] +
                                      dispPointPtrY[index]*dispPointPtrY[index] +
                                      dispPointPtrZ[index]*dispPointPtrZ[index];
                index++;
            }
        }
    }
    constraintValue[0] += constraintValue[1];
    constraintValue[0] /= (double)(splineControlPoint->nvox);
    constraintValue[1] /= (double)(splineControlPoint->nvox);
    constraintValue[2] /= (double)(splineControlPoint->nvox);
    nifti_image_free(dispControlPoint);
    free(jacobianDetermiant);
    free(jacobianMatrices);
}
/* *************************************************************** */
void reg_bspline_linearEnergy(nifti_image *splineControlPoint, double *val)
{
    if(splineControlPoint->nz==1){
        switch(splineControlPoint->datatype){
            case NIFTI_TYPE_FLOAT32:
                reg_bspline_linearEnergyApproxValue2D<float>(splineControlPoint, val);
                break;
#ifdef _NR_DEV
            case NIFTI_TYPE_FLOAT64:
                    reg_bspline_linearEnergyApproxValue2D<double>(splineControlPoint, val);
                    break;
#endif
            default:
                fprintf(stderr,"[NiftyReg ERROR] Only single or double precision is implemented for the linear energy\n");
                fprintf(stderr,"[NiftyReg ERROR] The linear energy is not computed\n");
                exit(1);
        }
    }
    else{
        switch(splineControlPoint->datatype){
            case NIFTI_TYPE_FLOAT32:
                    reg_bspline_linearEnergyApproxValue3D<float>(splineControlPoint, val);
                    break;
#ifdef _NR_DEV
            case NIFTI_TYPE_FLOAT64:
                    reg_bspline_linearEnergyApproxValue3D<double>(splineControlPoint, val);
                    break;
#endif
            default:
                fprintf(stderr,"[NiftyReg ERROR] Only single or double precision is implemented for the linear energy\n");
                fprintf(stderr,"[NiftyReg ERROR] The linear energy is not computed\n");
                exit(1);
        }
    }
}
/* *************************************************************** */
/* *************************************************************** */
template <class DTYPE>
void reg_bspline_approxLinearEnergyGradient2D(nifti_image *splineControlPoint,
                                              nifti_image *referenceImage,
                                              nifti_image *gradientImage,
                                              float weight0,
                                              float weight1,
                                              float weight2
                                              )
{
    unsigned int nodeNumber = splineControlPoint->nx*splineControlPoint->ny;

    DTYPE *jacobianDetermiant=(DTYPE *)malloc(nodeNumber*sizeof(DTYPE));
    mat33 *jacobianMatrices=(mat33 *)malloc(nodeNumber*sizeof(mat33));
    computeApproximateJacobianMatrices_2D(splineControlPoint,
                                          jacobianMatrices,
                                          jacobianDetermiant);

    DTYPE *gradPtrX = static_cast<DTYPE *>(gradientImage->data);
    DTYPE *gradPtrY = &gradPtrX[nodeNumber];

    DTYPE approxRatio0 = 2.0 * (double)weight0 * (double)(referenceImage->nx*referenceImage->ny) / nodeNumber;
    DTYPE approxRatio1 = 2.0 * (double)weight1 * (double)(referenceImage->nx*referenceImage->ny) / nodeNumber;
    DTYPE approxRatio2 = 2.0 * (double)weight2 * (double)(referenceImage->nx*referenceImage->ny) / nodeNumber;

    nifti_image *dispControlPoint=nifti_copy_nim_info(splineControlPoint);
    dispControlPoint->data=(void *)malloc(dispControlPoint->nvox*dispControlPoint->nbyper);
    memcpy(dispControlPoint->data,splineControlPoint->data,dispControlPoint->nvox*dispControlPoint->nbyper);
    reg_getDisplacementFromDeformation(dispControlPoint);
    DTYPE *dispPointPtrX = static_cast<DTYPE *>(dispControlPoint->data);
    DTYPE *dispPointPtrY = &dispPointPtrX[nodeNumber];

    DTYPE basis[9], basisX[9], basisY[9];
    DTYPE normal[3]={1.0/6.0, 2.0/3.0, 1.0/6.0};
    DTYPE first[3]={-0.5, 0, 0.5};
    int coord = 0;
    for(int b=2; b>-1; --b){
        for(int a=2; a>-1; --a){
            basis[coord]  = normal[a] * normal[b];
            basisX[coord] = first[a] * normal[b];
            basisY[coord] = normal[a] * first[b];
            coord++;
        }
    }

    mat33 reorient, desorient, jacobianMatrix;
    getReorientationMatrix(splineControlPoint, &desorient, &reorient);

    for(int y=1;y<splineControlPoint->ny-1;y++){
        unsigned int index= y*splineControlPoint->nx+1;
        for(int x=1;x<splineControlPoint->nx-1;x++){
            DTYPE gradX0=0;
            DTYPE gradX1=0;
            DTYPE gradX2=0;
            DTYPE gradY0=0;
            DTYPE gradY1=0;
            DTYPE gradY2=0;
            coord=0;
            for(int b=y-1;b<y+2;b++){
                unsigned int currentIndex= b*splineControlPoint->nx+x-1;
                for(int a=x-1;a<x+2;a++){

                    jacobianMatrix=jacobianMatrices[currentIndex];

                    gradX0 +=  basisY[coord] * (jacobianMatrix.m[0][1]+jacobianMatrix.m[1][0]);
                    gradY0 +=  basisX[coord] * (jacobianMatrix.m[0][1]+jacobianMatrix.m[1][0]);

                    gradX1 += 2.0 * basisX[coord] * (jacobianMatrix.m[0][0]-1.0);
                    gradY1 += 2.0 * basisY[coord] * (jacobianMatrix.m[1][1]-1.0);

                    gradX2 += 2.0 * basis[coord] * dispPointPtrX[currentIndex];
                    gradY2 += 2.0 * basis[coord] * dispPointPtrY[currentIndex];
                    currentIndex++;
                    coord++;
                }
            }
            gradX0 += gradX1;gradY0 += gradY1;
            gradPtrX[index] += approxRatio0 * (desorient.m[0][0]*gradX0 + desorient.m[0][1]*gradY0) +
                               approxRatio1 * (desorient.m[0][0]*gradX1 + desorient.m[0][1]*gradY1) +
                               approxRatio2 * gradX2;
            gradPtrY[index] += approxRatio0 * (desorient.m[1][0]*gradX0 + desorient.m[1][1]*gradY0) +
                               approxRatio1 * (desorient.m[1][0]*gradX1 + desorient.m[1][1]*gradY1) +
                               approxRatio2 * gradY2;
            index++;
        }
    }
    nifti_image_free(dispControlPoint);
    free(jacobianMatrices);
    free(jacobianDetermiant);
}
/* *************************************************************** */
template <class DTYPE>
void reg_bspline_approxLinearEnergyGradient3D(nifti_image *splineControlPoint,
                                              nifti_image *referenceImage,
                                              nifti_image *gradientImage,
                                              float weight0,
                                              float weight1,
                                              float weight2
                                              )
{
    unsigned int nodeNumber = splineControlPoint->nx*splineControlPoint->ny*splineControlPoint->nz;

    DTYPE *jacobianDetermiant=(DTYPE *)malloc(nodeNumber*sizeof(DTYPE));
    mat33 *jacobianMatrices=(mat33 *)malloc(nodeNumber*sizeof(mat33));
    computeApproximateJacobianMatrices_3D(splineControlPoint,
                                          jacobianMatrices,
                                          jacobianDetermiant);

    DTYPE *gradPtrX = static_cast<DTYPE *>(gradientImage->data);
    DTYPE *gradPtrY = &gradPtrX[nodeNumber];
    DTYPE *gradPtrZ = &gradPtrY[nodeNumber];

    DTYPE approxRatio0 = 2.0 * (double)weight0 * (double)(referenceImage->nx*referenceImage->ny*referenceImage->nz) / nodeNumber;
    DTYPE approxRatio1 = 2.0 * (double)weight1 * (double)(referenceImage->nx*referenceImage->ny*referenceImage->nz) / nodeNumber;
    DTYPE approxRatio2 = 2.0 * (double)weight2 * (double)(referenceImage->nx*referenceImage->ny*referenceImage->nz) / nodeNumber;

    nifti_image *dispControlPoint=nifti_copy_nim_info(splineControlPoint);
    dispControlPoint->data=(void *)malloc(dispControlPoint->nvox*dispControlPoint->nbyper);
    memcpy(dispControlPoint->data,splineControlPoint->data,dispControlPoint->nvox*dispControlPoint->nbyper);
    reg_getDisplacementFromDeformation(dispControlPoint);
    DTYPE *dispPointPtrX = static_cast<DTYPE *>(dispControlPoint->data);
    DTYPE *dispPointPtrY = &dispPointPtrX[nodeNumber];
    DTYPE *dispPointPtrZ = &dispPointPtrY[nodeNumber];

    DTYPE basis[27], basisX[27], basisY[27], basisZ[27];
    DTYPE normal[3]={1.0/6.0, 2.0/3.0, 1.0/6.0};
    DTYPE first[3]={-0.5, 0, 0.5};
    int coord = 0;
    for(int c=2; c>-1; --c){
        for(int b=2; b>-1; --b){
            for(int a=2; a>-1; --a){
                basis[coord]  = normal[a] * normal[b] * normal[c];
                basisX[coord] = first[a] * normal[b] * normal[c];
                basisY[coord] = normal[a] * first[b] * normal[c];
                basisZ[coord] = normal[a] * normal[b] * first[c];
                coord++;
            }
        }
    }

    mat33 reorient, desorient, jacobianMatrix;
    getReorientationMatrix(splineControlPoint, &desorient, &reorient);

    for(int z=1;z<splineControlPoint->nz-1;z++){
        for(int y=1;y<splineControlPoint->ny-1;y++){
            unsigned int index= (z*splineControlPoint->ny+y)*splineControlPoint->nx+1;
            for(int x=1;x<splineControlPoint->nx-1;x++){
                DTYPE gradX0=0; DTYPE gradX1=0; DTYPE gradX2=0;
                DTYPE gradY0=0; DTYPE gradY1=0; DTYPE gradY2=0;
                DTYPE gradZ0=0; DTYPE gradZ1=0; DTYPE gradZ2=0;
                coord=0;
                for(int c=z-1;c<z+2;c++){
                    for(int b=y-1;b<y+2;b++){
                        unsigned int currentIndex= (c*splineControlPoint->ny+b)*splineControlPoint->nx+x-1;
                        for(int a=x-1;a<x+2;a++){

                            jacobianMatrix=jacobianMatrices[currentIndex];

                            gradX0 +=  basisY[coord] * (jacobianMatrix.m[0][1]+jacobianMatrix.m[1][0]) +
                                       basisZ[coord] * (jacobianMatrix.m[0][2]+jacobianMatrix.m[2][0]);
                            gradY0 +=  basisX[coord] * (jacobianMatrix.m[0][1]+jacobianMatrix.m[1][0]) +
                                       basisZ[coord] * (jacobianMatrix.m[2][1]+jacobianMatrix.m[1][2]);
                            gradZ0 +=  basisX[coord] * (jacobianMatrix.m[0][2]+jacobianMatrix.m[2][0]) +
                                       basisY[coord] * (jacobianMatrix.m[2][1]+jacobianMatrix.m[1][2]);

                            gradX1 += 2.0 * basisX[coord] * (jacobianMatrix.m[0][0]-1.0);
                            gradY1 += 2.0 * basisY[coord] * (jacobianMatrix.m[1][1]-1.0);
                            gradZ1 += 2.0 * basisZ[coord] * (jacobianMatrix.m[2][2]-1.0);

                            gradX2 += 2.0 * basis[coord] * dispPointPtrX[currentIndex];
                            gradY2 += 2.0 * basis[coord] * dispPointPtrY[currentIndex];
                            gradZ2 += 2.0 * basis[coord] * dispPointPtrZ[currentIndex];

                            currentIndex++;
                            coord++;
                        }
                    }
                }
                gradX0 += gradX1;gradY0 += gradY1;gradZ0 += gradZ1;
                gradPtrX[index] += approxRatio0 * (desorient.m[0][0]*gradX0 + desorient.m[0][1]*gradY0 + desorient.m[0][2]*gradZ0) +
                                   approxRatio1 * (desorient.m[0][0]*gradX1 + desorient.m[0][1]*gradY1 + desorient.m[0][2]*gradZ1) +
                                   approxRatio2 * gradX2;
                gradPtrY[index] += approxRatio0 * (desorient.m[1][0]*gradX0 + desorient.m[1][1]*gradY0 + desorient.m[1][2]*gradZ0) +
                                   approxRatio1 * (desorient.m[1][0]*gradX1 + desorient.m[1][1]*gradY1 + desorient.m[1][2]*gradZ1) +
                                   approxRatio2 * gradY2;
                gradPtrZ[index] += approxRatio0 * (desorient.m[2][0]*gradX0 + desorient.m[2][1]*gradY0 + desorient.m[2][2]*gradZ0) +
                                   approxRatio1 * (desorient.m[2][0]*gradX1 + desorient.m[2][1]*gradY1 + desorient.m[2][2]*gradZ1) +
                                   approxRatio2 * gradZ2;
                index++;
            }
        }
    }
    nifti_image_free(dispControlPoint);
    free(jacobianMatrices);
    free(jacobianDetermiant);
}
/* *************************************************************** */
void reg_bspline_linearEnergyGradient(nifti_image *splineControlPoint,
                                      nifti_image *targetImage,
                                      nifti_image *gradientImage,
                                      float weight0,
                                      float weight1,
                                      float weight2
                                      )
{
    if(splineControlPoint->datatype != gradientImage->datatype){
        fprintf(stderr,"[NiftyReg ERROR] The spline control point image and the gradient image were expected to have the same datatype\n");
        fprintf(stderr,"[NiftyReg ERROR] The bending energy gradient has not computed\n");
        exit(1);
    }
    if(splineControlPoint->nz==1){
        switch(splineControlPoint->datatype){
            case NIFTI_TYPE_FLOAT32:
                reg_bspline_approxLinearEnergyGradient2D<float>
                    (splineControlPoint, targetImage, gradientImage, weight0, weight1, weight2);
                break;
#ifdef _NR_DEV
            case NIFTI_TYPE_FLOAT64:
                reg_bspline_approxLinearEnergyGradient2D<double>
                    (splineControlPoint, targetImage, gradientImage, weight0, weight1, weight2);
                break;
#endif
            default:
                fprintf(stderr,"[NiftyReg ERROR] Only single or double precision is implemented for the bending energy gradient\n");
                fprintf(stderr,"[NiftyReg ERROR] The bending energy gradient has not been computed\n");
                exit(1);
        }
    }
    else{
        switch(splineControlPoint->datatype){
            case NIFTI_TYPE_FLOAT32:
                reg_bspline_approxLinearEnergyGradient3D<float>
                    (splineControlPoint, targetImage, gradientImage, weight0, weight1, weight2);
                break;
#ifdef _NR_DEV
            case NIFTI_TYPE_FLOAT64:
                reg_bspline_approxLinearEnergyGradient3D<double>
                    (splineControlPoint, targetImage, gradientImage, weight0, weight1, weight2);
                break;
#endif
            default:
                fprintf(stderr,"[NiftyReg ERROR] Only single or double precision is implemented for the bending energy gradient\n");
                fprintf(stderr,"[NiftyReg ERROR] The bending energy gradient has not been computed\n");
                exit(1);
        }
    }
}
/* *************************************************************** */