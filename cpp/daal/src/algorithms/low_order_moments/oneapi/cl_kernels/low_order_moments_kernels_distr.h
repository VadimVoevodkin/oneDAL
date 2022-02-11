/* file: low_order_moments_kernels_distr.h */
/*******************************************************************************
* Copyright 2020 Intel Corporation
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
*     http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*******************************************************************************/

/*
//++
//  Implementation of low order moments kernels.
//--
*/

#ifndef __low_order_moments_kernels_distr__
#define __low_order_moments_kernels_distr__

static const char * low_order_moments_kernels_distr_cl =
    "\n"
    "#define CONCAT(n, suff) n ## suff\n"
    "#define FULLNAME(n, p) CONCAT(n, p)\n"
    "\n"
    "#define mergeDistrBlocks FULLNAME(mergeDistrBlocks, FNAMESUFF)\n"
    "#define finalize FULLNAME(finalize, FNAMESUFF)\n"
    "\n"
    "/* merge distributed blocks kernel */\n"
    "__kernel void mergeDistrBlocks(uint nFeatures, uint nBlocks, uint stride\n"
    "#if (defined _RMIN_)\n"
    "                               ,\n"
    "                               __global algorithmFPType * gMin\n"
    "#endif\n"
    "#if (defined _RMAX_)\n"
    "                               ,\n"
    "                               __global algorithmFPType * gMax\n"
    "#endif\n"
    "#if (defined _RSUM_) || (defined _RMEAN_) || (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "                               ,\n"
    "                               __global algorithmFPType * gSum\n"
    "#endif\n"
    "#if (defined _RSUM2_) || (defined _RSORM_)\n"
    "                               ,\n"
    "                               __global algorithmFPType * gSum2\n"
    "#endif\n"
    "#if (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "                               ,\n"
    "                               __global algorithmFPType * gSum2Cent\n"
    "#endif\n"
    "#if (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "                               ,\n"
    "                               const __global algorithmFPType * bNVec\n"
    "#endif\n"
    "#if (defined _RMIN_)\n"
    "                               ,\n"
    "                               const __global algorithmFPType * bMin\n"
    "#endif\n"
    "#if (defined _RMAX_)\n"
    "                               ,\n"
    "                               const __global algorithmFPType * bMax\n"
    "#endif\n"
    "#if (defined _RSUM_) || (defined _RMEAN_) || (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "                               ,\n"
    "                               const __global algorithmFPType * bSum\n"
    "#endif\n"
    "#if (defined _RSUM2_) || (defined _RSORM_)\n"
    "                               ,\n"
    "                               const __global algorithmFPType * bSum2\n"
    "#endif\n"
    "#if (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "                               ,\n"
    "                               const __global algorithmFPType * bSum2Cent\n"
    "#endif\n"
    ")\n"
    "{\n"
    "    const uint itemId = get_global_id(0);\n"
    "\n"
    "#if (defined _RMIN_)\n"
    "    algorithmFPType mrgMin = bMin[itemId];\n"
    "#endif\n"
    "#if (defined _RMAX_)\n"
    "    algorithmFPType mrgMax = bMax[itemId];\n"
    "#endif\n"
    "#if (defined _RSUM_) || (defined _RMEAN_) || (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "    algorithmFPType mrgSum = (algorithmFPType)0;\n"
    "#endif\n"
    "#if (defined _RSUM2_) || (defined _RSORM_)\n"
    "    algorithmFPType mrgSum2 = (algorithmFPType)0;\n"
    "#endif\n"
    "#if (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "    algorithmFPType mrgVectors  = (algorithmFPType)0;\n"
    "    algorithmFPType mrgSum2Cent = (algorithmFPType)0;\n"
    "    algorithmFPType mrgMean     = (algorithmFPType)0;\n"
    "#endif\n"
    "\n"
    "    for (uint i = 0; i < nBlocks; i++)\n"
    "    {\n"
    "        uint offset = i * stride + itemId;\n"
    "\n"
    "#if (defined _RMIN_)\n"
    "        algorithmFPType min = bMin[offset];\n"
    "#endif\n"
    "#if (defined _RMAX_)\n"
    "        algorithmFPType max = bMax[offset];\n"
    "#endif\n"
    "#if (defined _RSUM_) || (defined _RMEAN_) || (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "        algorithmFPType sum = bSum[offset];\n"
    "#endif\n"
    "#if (defined _RSUM2_) || (defined _RSORM_)\n"
    "        algorithmFPType sum2 = bSum2[offset];\n"
    "#endif\n"
    "#if (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "        algorithmFPType nVec     = bNVec[i];\n"
    "        algorithmFPType sum2Cent = bSum2Cent[offset];\n"
    "        algorithmFPType mean     = sum / nVec;\n"
    "\n"
    "        algorithmFPType sumN1N2    = mrgVectors + nVec;\n"
    "        algorithmFPType mulN1N2    = mrgVectors * nVec;\n"
    "        algorithmFPType deltaScale = mulN1N2 / sumN1N2;\n"
    "        algorithmFPType meanScale  = (algorithmFPType)1 / sumN1N2;\n"
    "        algorithmFPType delta      = mean - mrgMean;\n"
    "#endif\n"
    "\n"
    "#if (defined _RMIN_)\n"
    "        mrgMin = fmin(min, mrgMin);\n"
    "#endif\n"
    "#if (defined _RMAX_)\n"
    "        mrgMax = fmax(max, mrgMax);\n"
    "#endif\n"
    "#if (defined _RSUM_) || (defined _RMEAN_) || (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "        mrgSum += sum;\n"
    "#endif\n"
    "#if (defined _RSUM2_) || (defined _RSORM_)\n"
    "        mrgSum2 += sum2;\n"
    "#endif\n"
    "#if (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "        mrgSum2Cent = mrgSum2Cent + sum2Cent + delta * delta * deltaScale;\n"
    "        mrgMean     = (mrgMean * mrgVectors + mean * nVec) * meanScale;\n"
    "        mrgVectors  = sumN1N2;\n"
    "#endif\n"
    "    }\n"
    "\n"
    "#if (defined _RMIN_)\n"
    "    gMin[itemId] = mrgMin;\n"
    "#endif\n"
    "#if (defined _RMAX_)\n"
    "    gMax[itemId] = mrgMax;\n"
    "#endif\n"
    "#if (defined _RSUM_) || (defined _RMEAN_) || (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "    gSum[itemId] = mrgSum;\n"
    "#endif\n"
    "#if (defined _RSUM2_) || (defined _RSORM_)\n"
    "    gSum2[itemId] = mrgSum2;\n"
    "#endif\n"
    "#if (defined _RSUM2C_) || (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "    gSum2Cent[itemId] = mrgSum2Cent;\n"
    "#endif\n"
    "}\n"
    "/* finalize kernel */\n"
    "\n"
    "__kernel void finalize(const algorithmFPType     nObservations\n"
    "                      #if (defined _RMIN_)\n"
    "                          ,__global algorithmFPType* gMin\n"
    "                      #endif\n"
    "                      #if (defined _RMAX_)\n"
    "                          ,__global algorithmFPType* gMax\n"
    "                      #endif\n"
    "                      #if (defined _RMEAN_)\n"
    "                          ,__global algorithmFPType* gSum\n"
    "                      #endif\n"
    "                      #if (defined _RSORM_)\n"
    "                          ,__global algorithmFPType* gSum2\n"
    "                      #endif\n"
    "                      #if (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "                          ,__global algorithmFPType* gSum2Cent\n"
    "                      #endif\n"
    "                      #if (defined _RMEAN_)\n"
    "                          ,__global algorithmFPType* gMean\n"
    "                      #endif\n"
    "                      #if (defined _RSORM_)\n"
    "                          ,__global algorithmFPType* gSecondOrderRawMoment\n"
    "                      #endif\n"
    "                      #if (defined _RVARC_)\n"
    "                          ,__global algorithmFPType* gVariance\n"
    "                      #endif\n"
    "                      #if (defined _RSTDEV_)\n"
    "                          ,__global algorithmFPType* gStDev\n"
    "                      #endif\n"
    "                      #if (defined _RVART_)\n"
    "                          ,__global algorithmFPType* gVariation\n"
    "                      #endif\n"
    "                         )\n"
    "{\n"
    "    const uint tid  = get_global_id(0);\n"
    "\n"
    "#if (defined _RMEAN_)\n"
    "    algorithmFPType sum      = gSum [tid];  \n"
    "#endif\n"
    "#if (defined _RSORM_)\n"
    "    algorithmFPType sum2     = gSum2[tid];  \n"
    "#endif\n"
    "#if (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "    algorithmFPType sum2Cent = gSum2Cent[tid];  \n"
    "#endif\n"
    "#if (defined _RMEAN_) || (defined _RVART_)\n"
    "    algorithmFPType mean     = sum / nObservations;\n"
    "#endif\n"
    "#if (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "    algorithmFPType variance = (algorithmFPType)0;  \n"
    "#endif\n"
    "#if (defined _RSTDEV_) || (defined _RVART_)\n"
    "    algorithmFPType stDev    = (algorithmFPType)0;  \n"
    "#endif\n"
    "\n"
    "// common vars calculation\n"
    "#if (defined _RVARC_) || (defined _RSTDEV_) || (defined _RVART_)\n"
    "    variance      = sum2Cent / (nObservations - (algorithmFPType)1);\n"
    "#endif\n"
    "#if (defined _RSTDEV_) || (defined _RVART_)\n"
    "    stDev         = (algorithmFPType)sqrt(variance);\n"
    "#endif\n"
    "\n"
    "// output assignment\n"
    "#if (defined _RMEAN_)\n"
    "    gMean[tid] = mean;\n"
    "#endif\n"
    "#if (defined _RSORM_)\n"
    "    gSecondOrderRawMoment[tid] = sum2 / nObservations;\n"
    "#endif\n"
    "#if (defined _RVARC_)\n"
    "    gVariance[tid]  = variance;\n"
    "#endif\n"
    "#if (defined _RSTDEV_)\n"
    "    gStDev[tid]     = stDev; \n"
    "#endif\n"
    "#if (defined _RVART_)\n"
    "    gVariation[tid] = stDev / mean;\n"
    "#endif\n"
    "}\n"
    "\n";
#endif
