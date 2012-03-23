/*
 * Potentials.h
 *
 *  Created on: Nov 24, 2010
 *      Author: gasst
 */

#ifndef _SEGMENTATIONREGISTRATIONPAIRWISEPOTENTIAL_H_
#define _SEGMENTATIONREGISTRATIONPAIRWISEPOTENTIAL_H_
#include "itkObject.h"
#include "itkObjectFactory.h"
#include <utility>
#include "itkVector.h"
#include "itkLinearInterpolateImageFunction.h"
#include "itkNearestNeighborInterpolateImageFunction.h"
#include "itkStatisticsImageFilter.h"
#include "itkThresholdImageFilter.h"

namespace itk{



    template<class TImage>
    class PairwisePotentialCoherence : public itk::Object{
    public:
        //itk declarations
        typedef PairwisePotentialCoherence            Self;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;

        typedef	TImage ImageType;
        typedef typename ImageType::Pointer ImagePointerType;
        typedef typename ImageType::ConstPointer ConstImagePointerType;
        typedef typename itk::Image<float,ImageType::ImageDimension> FloatImageType;
        typedef typename FloatImageType::Pointer FloatImagePointerType;
        
        typedef typename  itk::Vector<float,ImageType::ImageDimension>  LabelType;
        typedef typename itk::Image<LabelType,ImageType::ImageDimension> LabelImageType;
        typedef typename LabelImageType::Pointer LabelImagePointerType;
        typedef typename ImageType::IndexType IndexType;
        typedef typename ImageType::SizeType SizeType;
        typedef typename ImageType::SpacingType SpacingType;
        typedef LinearInterpolateImageFunction<ImageType> ImageInterpolatorType;
        typedef typename ImageInterpolatorType::Pointer ImageInterpolatorPointerType;
        typedef LinearInterpolateImageFunction<FloatImageType> FloatImageInterpolatorType;
        typedef typename FloatImageInterpolatorType::Pointer FloatImageInterpolatorPointerType;

        typedef NearestNeighborInterpolateImageFunction<ImageType> SegmentationInterpolatorType;
        typedef typename SegmentationInterpolatorType::Pointer SegmentationInterpolatorPointerType;
        typedef typename ImageInterpolatorType::ContinuousIndexType ContinuousIndexType;
        
        SizeType m_targetSize,m_atlasSize;
        typedef typename itk::StatisticsImageFilter< FloatImageType > StatisticsFilterType;


    protected:
        ConstImagePointerType m_targetImage, m_atlasImage;
        SegmentationInterpolatorPointerType m_atlasSegmentationInterpolator;
        FloatImageInterpolatorPointerType m_atlasDistanceTransformInterpolator, m_atlasBackgroundDistanceTransformInterpolator;
        ImageInterpolatorPointerType  m_atlasInterpolator;
        LabelImagePointerType m_baseLabelMap;
        bool m_haveLabelMap;
        double m_asymm;
        FloatImagePointerType m_distanceTransform;
        double sigma1, sigma2, mean1, mean2, m_threshold,maxDist,minDist, mDistTarget,mDistSecondary;
        int m_nSegmentationLabels;
    public:
        /** Method for creation through the object factory. */
        itkNewMacro(Self);
        /** Standard part of every itk Object. */
        itkTypeMacro(PairwisePotentialCoherence, Object);

        PairwisePotentialCoherence(){
            m_haveLabelMap=false;
            m_asymm=1;
            m_threshold=9999999999.0;
        }
        virtual void freeMemory(){
        }
        void SetNumberOfSegmentationLabels(int n){m_nSegmentationLabels=n;}
        void SetBaseLabelMap(LabelImagePointerType blm){m_baseLabelMap=blm;m_haveLabelMap=true;}
        LabelImagePointerType GetBaseLabelMap(LabelImagePointerType blm){return m_baseLabelMap;}
        
        void SetAtlasInterpolator(ImageInterpolatorPointerType atlasImage){
            m_atlasInterpolator=atlasImage;
        }
        void SetAtlasSegmentationInterpolator(SegmentationInterpolatorPointerType atlasSegmentation){
            m_atlasSegmentationInterpolator=atlasSegmentation;
        }
    	void SetAtlasImage(ConstImagePointerType atlasImage){
            m_atlasImage=atlasImage;
            m_atlasSize=m_atlasImage->GetLargestPossibleRegion().GetSize();
            m_atlasInterpolator=ImageInterpolatorType::New();
            m_atlasInterpolator->SetInputImage(m_atlasImage);//,m_atlasImage->GetLargestPossibleRegion());
            cout<<"atlasSize "<<m_atlasSize<<endl;
        }
        void SetTargetImage(ConstImagePointerType targetImage){
            m_targetImage=targetImage;
            m_targetSize=m_targetImage->GetLargestPossibleRegion().GetSize();
        }
        void SetAsymmetryWeight(double as){
            m_asymm=1-as;
        }
        void SetDistanceTransform(FloatImagePointerType dt){
            m_distanceTransform=dt;
            m_atlasDistanceTransformInterpolator=FloatImageInterpolatorType::New();
            m_atlasDistanceTransformInterpolator->SetInputImage(dt);
            typename StatisticsFilterType::Pointer filter=StatisticsFilterType::New();
            filter->SetInput(dt);
            filter->Update();
            sigma1=filter->GetSigma();
            mean1=filter->GetMean();
            cout<<"distance transform main segmentation sigma :"<<sigma1<<endl;

        }
        void SetBackgroundDistanceTransform(FloatImagePointerType dt){
            m_atlasBackgroundDistanceTransformInterpolator=FloatImageInterpolatorType::New();
            m_atlasBackgroundDistanceTransformInterpolator->SetInputImage(dt);
            typename StatisticsFilterType::Pointer filter=StatisticsFilterType::New();
            filter->SetInput(dt);
            filter->Update();
            sigma2=filter->GetSigma();
            mean2=fabs(filter->GetMean());
            cout<<"distance transform background segmentation sigma :"<<sigma2<<endl;
        }
        FloatImagePointerType GetDistanceTransform(){return  m_distanceTransform;}
        

        void SetReferenceSegmentation(ConstImagePointerType segImage, double scale=1.0){
            typename StatisticsFilterType::Pointer filter=StatisticsFilterType::New();
            m_atlasSegmentationInterpolator= SegmentationInterpolatorType::New();
            m_atlasSegmentationInterpolator->SetInputImage(segImage);
            //get distance transform to foreground label
            FloatImagePointerType dt1=getDistanceTransform(segImage, m_nSegmentationLabels - 1);
            //save image for debugging
            typedef itk::ThresholdImageFilter <FloatImageType>
                ThresholdImageFilterType;
            typename ThresholdImageFilterType::Pointer thresholdFilter
                = ThresholdImageFilterType::New();
            thresholdFilter->SetInput(dt1);
            thresholdFilter->ThresholdOutside(0, 1000);
            thresholdFilter->SetOutsideValue(1000);
            typedef itk::RescaleIntensityImageFilter<FloatImageType,ImageType> CasterType;
            typename CasterType::Pointer caster=CasterType::New();
            caster->SetOutputMinimum( numeric_limits<typename ImageType::PixelType>::min() );
            caster->SetOutputMaximum( numeric_limits<typename ImageType::PixelType>::max() );
                
            caster->SetInput(thresholdFilter->GetOutput());
            caster->Update();
            ImagePointerType output=caster->GetOutput();
            if (true){    
                if (ImageType::ImageDimension==2){
                    ImageUtils<ImageType>::writeImage("dt1.png",(output));    
                }
                if (ImageType::ImageDimension==3){
                    ImageUtils<ImageType>::writeImage("dt1.nii",(output));
                }
            }
            
            //feed DT into interpolator
            m_atlasDistanceTransformInterpolator=FloatImageInterpolatorType::New();
            m_atlasDistanceTransformInterpolator->SetInputImage(dt1);
            filter->SetInput(dt1);
            filter->Update();
            sigma1=filter->GetSigma();
            mean1=filter->GetMean();
            maxDist=filter->GetMaximumOutput()->Get();
            minDist=filter->GetMinimumOutput()->Get();
          
            mDistTarget=fabs(minDist);
            cout<<"MINDIST: "<<mDistTarget<<endl;
            m_distanceTransform=dt1;
            if (m_nSegmentationLabels>2){
                FloatImagePointerType dt2=getDistanceTransform(segImage, 1);
                m_atlasBackgroundDistanceTransformInterpolator=FloatImageInterpolatorType::New();
                m_atlasBackgroundDistanceTransformInterpolator->SetInputImage(dt2);
                thresholdFilter->SetInput(dt2);
                filter->SetInput(dt2);
                filter->Update();
                sigma2=filter->GetSigma();
                mean2=fabs(filter->GetMean());
                double di=filter->GetMinimumOutput()->Get();
                mDistSecondary=fabs(di);
                cout<<"MINDIST: "<<mDistSecondary<<endl;
                caster->SetInput(thresholdFilter->GetOutput());
                caster->Update();
                ImagePointerType output=caster->GetOutput();
                if (false){
                    if (ImageType::ImageDimension==2){
                        ImageUtils<ImageType>::writeImage("dt2.png",(output));    
                    }
                    if (ImageType::ImageDimension==3){
                        ImageUtils<ImageType>::writeImage("dt2.nii",(output));
                    }}
            }

            m_atlasSegmentationInterpolator=SegmentationInterpolatorType::New();
            //            m_atlasSegmentationInterpolator->SetInputImage((ImagePointerType)(const_cast<ImageType* >(&(*segImage))),segImage->GetLargestPossibleRegion());
            m_atlasSegmentationInterpolator->SetInputImage(segImage);
        }
        FloatImagePointerType getDistanceTransform(ConstImagePointerType segmentationImage, int value){
            typedef typename itk::SignedMaurerDistanceMapImageFilter< ImageType, FloatImageType > DistanceTransformType;
            typename DistanceTransformType::Pointer distanceTransform=DistanceTransformType::New();
            typedef typename  itk::ImageRegionConstIterator<ImageType> ImageConstIterator;
            typedef typename  itk::ImageRegionIterator<ImageType> ImageIterator;
            ImagePointerType newImage=ImageUtils<ImageType>::createEmpty(segmentationImage);
            ImageConstIterator imageIt(segmentationImage,segmentationImage->GetLargestPossibleRegion());        
            ImageIterator imageIt2(newImage,newImage->GetLargestPossibleRegion());        
            for (imageIt.GoToBegin(),imageIt2.GoToBegin();!imageIt.IsAtEnd();++imageIt, ++imageIt2){
                float val=imageIt.Get();
                imageIt2.Set(val==value);
                
            }
            //distanceTransform->InsideIsPositiveOn();
            distanceTransform->SetInput(newImage);
            distanceTransform->SquaredDistanceOff ();
            distanceTransform->UseImageSpacingOn();
            distanceTransform->Update();
            typedef typename  itk::ImageRegionIterator<FloatImageType> FloatImageIterator;

            FloatImagePointerType positiveDM=distanceTransform->GetOutput();
#if 0
            FloatImageIterator imageIt3(positiveDM,positiveDM->GetLargestPossibleRegion());        
            for (imageIt3.GoToBegin();!imageIt3.IsAtEnd();++imageIt3){
                imageIt3.Set(fabs(imageIt3.Get()));
            }
#endif
            return  positiveDM;
        }
        ImagePointerType getFGDT(){
            typedef itk::ThresholdImageFilter <FloatImageType>
                ThresholdImageFilterType;
            typename ThresholdImageFilterType::Pointer thresholdFilter
                = ThresholdImageFilterType::New();
            thresholdFilter->SetInput( m_distanceTransform);
            thresholdFilter->ThresholdOutside(0, 1000);
            thresholdFilter->SetOutsideValue(1000);
            typedef itk::RescaleIntensityImageFilter<FloatImageType,ImageType> CasterType;
            typename CasterType::Pointer caster=CasterType::New();
            caster->SetOutputMinimum( numeric_limits<typename ImageType::PixelType>::min() );
            caster->SetOutputMaximum( numeric_limits<typename ImageType::PixelType>::max() );
            caster->SetInput(thresholdFilter->GetOutput());
            caster->Update();
            return caster->GetOutput();
        }
        void SetThreshold(double t){m_threshold=t;}

#if 0        //edge from  segmentation to Registration
        virtual double getPotential(IndexType targetIndex1, IndexType targetIndex2,LabelType displacement, int segmentationLabel){
            double result=0;
            ContinuousIndexType idx2(targetIndex2);
            itk::Vector<float,ImageType::ImageDimension> disp=displacement;
            idx2+= disp;
            if (m_baseLabelMap){
                itk::Vector<float,ImageType::ImageDimension> baseDisp=m_baseLabelMap->GetPixel(targetIndex2);
                idx2+=baseDisp;
            }
            int deformedAtlasSegmentation=-1;
            double distanceToDeformedSegmentation;
            if (!m_atlasSegmentationInterpolator->IsInsideBuffer(idx2)){
                for (int d=0;d<ImageType::ImageDimension;++d){
                    if (idx2[d]>=this->m_atlasSegmentationInterpolator->GetEndContinuousIndex()[d]){
                        idx2[d]=this->m_atlasSegmentationInterpolator->GetEndContinuousIndex()[d]-0.5;
                    }
                    else if (idx2[d]<this->m_atlasSegmentationInterpolator->GetStartContinuousIndex()[d]){
                        idx2[d]=this->m_atlasSegmentationInterpolator->GetStartContinuousIndex()[d]+0.5;
                    }
                }
            }
            deformedAtlasSegmentation=floor(m_atlasSegmentationInterpolator->EvaluateAtContinuousIndex(idx2)+0.5);
            distanceToDeformedSegmentation= fabs(m_atlasDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2));
            result=0;
            if (segmentationLabel){
                if (deformedAtlasSegmentation!=segmentationLabel){
                    result=m_asymm;//*(1+distanceToDeformedSegmentation);
                }
            }else{
                if (deformedAtlasSegmentation){
                    result=1;//distanceToDeformedSegmentation;
                }
            }
          
            return result;
        }
#endif
        //edge from registration to segmentation
        inline virtual  double getPotential(IndexType targetIndex1, IndexType targetIndex2,LabelType displacement, int segmentationLabel){
            double result=0;
            ContinuousIndexType idx2(targetIndex2);
            itk::Vector<float,ImageType::ImageDimension> disp=displacement;
            idx2+= disp;
            if (m_baseLabelMap){
                itk::Vector<float,ImageType::ImageDimension> baseDisp=m_baseLabelMap->GetPixel(targetIndex2);
                idx2+=baseDisp;
            }
            int deformedAtlasSegmentation=-1;
            double distanceToDeformedSegmentation;
            if (!m_atlasSegmentationInterpolator->IsInsideBuffer(idx2)){
                for (int d=0;d<ImageType::ImageDimension;++d){
                    if (idx2[d]>=this->m_atlasSegmentationInterpolator->GetEndContinuousIndex()[d]){
                        idx2[d]=this->m_atlasSegmentationInterpolator->GetEndContinuousIndex()[d]-0.5;
                    }
                    else if (idx2[d]<this->m_atlasSegmentationInterpolator->GetStartContinuousIndex()[d]){
                        idx2[d]=this->m_atlasSegmentationInterpolator->GetStartContinuousIndex()[d]+0.5;
                    }
                }
            }
            deformedAtlasSegmentation=int(m_atlasSegmentationInterpolator->EvaluateAtContinuousIndex(idx2));
#define THRESHOLDING
            if (segmentationLabel==m_nSegmentationLabels - 1){
                if (segmentationLabel!=deformedAtlasSegmentation){
                    distanceToDeformedSegmentation=m_atlasDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2);
#ifdef THRESHOLDING
                    distanceToDeformedSegmentation=distanceToDeformedSegmentation>m_threshold?maxDist:distanceToDeformedSegmentation;
#endif
                    result=fabs(distanceToDeformedSegmentation)/mDistTarget;//((sigma1+sigma2)/2);//1;
                }
            }
            else if (segmentationLabel){
                if (segmentationLabel!=deformedAtlasSegmentation){
                    distanceToDeformedSegmentation=m_atlasBackgroundDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2);
                    result=fabs(distanceToDeformedSegmentation)/mDistSecondary;//((sigma1+sigma2)/2);//1;
                }
            }
            else{
                if (deformedAtlasSegmentation==m_nSegmentationLabels - 1){
                    distanceToDeformedSegmentation=m_atlasDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2);
#ifdef THRESHOLDING
                    distanceToDeformedSegmentation=distanceToDeformedSegmentation>m_threshold?maxDist:distanceToDeformedSegmentation;
#endif
                    result=fabs(distanceToDeformedSegmentation)/mDistTarget;//((sigma1+sigma2)/2);//1;
                }
                else if(deformedAtlasSegmentation){
                    distanceToDeformedSegmentation=m_atlasBackgroundDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2);
                    result=fabs(distanceToDeformedSegmentation)/mDistSecondary;//((sigma1+sigma2)/2);//1;
                }
            }
            result*=result;
#if 0
            if (deformedAtlasSegmentation>0){
                if (segmentationLabel== 0 && deformedAtlasSegmentation == m_nSegmentationLabels - 1){
                    //trying to assign background label when atlas label is target anatomy
                    distanceToDeformedSegmentation=m_atlasDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2);
                    result=fabs(distanceToDeformedSegmentation)/mDistTarget;//((sigma1+sigma2)/2);//1;
                    
                }else if (segmentationLabel == 0 && deformedAtlasSegmentation ){
                    //trying to assign background label when atlas label is secondary foreground
                    distanceToDeformedSegmentation= m_atlasBackgroundDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2);
                    result=fabs(distanceToDeformedSegmentation)/mDistSecondary;//((sigma1+sigma2)/2);//2;
                    
                }
                else if (segmentationLabel==m_nSegmentationLabels - 1){
                    //trying to assign target anatomy label which is different from atlas label
                    distanceToDeformedSegmentation=fabs(m_atlasDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2))/mDistTarget;//((sigma1+sigma2)/2);
                    if (distanceToDeformedSegmentation>m_threshold)
                        result=maxDist/((sigma1+sigma2)/2);
                }
                else if (segmentationLabel){
                    //trying to assign secondary foreground label when atlas is foreground
                    result=fabs(m_atlasBackgroundDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2))/mDistSecondary;//((sigma1+sigma2)/2);
                }
                
            }else{
                if (segmentationLabel== m_nSegmentationLabels - 1){
                    //distanceToDeformedSegmentation= 1;
                    distanceToDeformedSegmentation=fabs(m_atlasDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2));
#if 1       
                    if (distanceToDeformedSegmentation>m_threshold)
                        result=maxDist/((sigma1+sigma2)/2);
                    else
#endif
                        result=(distanceToDeformedSegmentation)/mDistTarget;//((sigma1+sigma2)/2);//1;
                }else if (segmentationLabel ){
                    distanceToDeformedSegmentation= fabs(m_atlasBackgroundDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2));
#if 0
                    distanceToDeformedSegmentation=min(m_threshold,distanceToDeformedSegmentation);
#endif
                    result=(distanceToDeformedSegmentation)/mDistSecondary;//((sigma1+sigma2)/2);//2;
                }
            }
#endif
            return result;
        }
    };//class

    template<class TImage>
    class PairwisePotentialBoneCoherence :public PairwisePotentialCoherence<TImage>{
    public:
        //itk declarations
        typedef PairwisePotentialBoneCoherence            Self;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;

        typedef	TImage ImageType;
        typedef typename ImageType::Pointer ImagePointerType;
        typedef typename ImageType::ConstPointer ConstImagePointerType;
        typedef typename itk::Image<float,ImageType::ImageDimension> FloatImageType;
        typedef typename FloatImageType::Pointer FloatImagePointerType;
        
        typedef typename  itk::Vector<float,ImageType::ImageDimension>  LabelType;
        typedef typename itk::Image<LabelType,ImageType::ImageDimension> LabelImageType;
        typedef typename LabelImageType::Pointer LabelImagePointerType;
        typedef typename ImageType::IndexType IndexType;
        typedef typename ImageType::SizeType SizeType;
        typedef typename ImageType::SpacingType SpacingType;
        typedef LinearInterpolateImageFunction<ImageType> ImageInterpolatorType;
        typedef typename ImageInterpolatorType::Pointer ImageInterpolatorPointerType;
        typedef LinearInterpolateImageFunction<FloatImageType> FloatImageInterpolatorType;
        typedef typename FloatImageInterpolatorType::Pointer FloatImageInterpolatorPointerType;

        typedef NearestNeighborInterpolateImageFunction<ImageType> SegmentationInterpolatorType;
        typedef typename SegmentationInterpolatorType::Pointer SegmentationInterpolatorPointerType;
        typedef typename ImageInterpolatorType::ContinuousIndexType ContinuousIndexType;
        
        typedef typename itk::StatisticsImageFilter< FloatImageType > StatisticsFilterType;
 
    public:
        itkNewMacro(Self);

        void SetReferenceSegmentation(ConstImagePointerType segImage, double scale=1.0){
            if (scale !=1.0 ){
                segImage=FilterUtils<ImageType>::NNResample(segImage,scale);
            }
            FloatImagePointerType dt1=getDistanceTransform(segImage, 1);
            this->m_atlasDistanceTransformInterpolator=FloatImageInterpolatorType::New();
            this->m_atlasDistanceTransformInterpolator->SetInputImage(dt1);
            typename StatisticsFilterType::Pointer filter=StatisticsFilterType::New();
            filter->SetInput(dt1);
            filter->Update();
            this->sigma1=filter->GetSigma();
            this->mean1=filter->GetMean();
            this->m_distanceTransform=dt1;

        
            typedef itk::ThresholdImageFilter <FloatImageType>
                ThresholdImageFilterType;
            typename ThresholdImageFilterType::Pointer thresholdFilter
                = ThresholdImageFilterType::New();
            thresholdFilter->SetInput(dt1);
            thresholdFilter->ThresholdOutside(0, 1000);
            thresholdFilter->SetOutsideValue(1000);
            typedef itk::RescaleIntensityImageFilter<FloatImageType,ImageType> CasterType;
            typename CasterType::Pointer caster=CasterType::New();
            caster->SetOutputMinimum( numeric_limits<typename ImageType::PixelType>::min() );
            caster->SetOutputMaximum( numeric_limits<typename ImageType::PixelType>::max() );
            caster->SetInput(thresholdFilter->GetOutput());
            caster->Update();
            ImagePointerType output=caster->GetOutput();
            if (false){
                if (ImageType::ImageDimension==2){
                    ImageUtils<ImageType>::writeImage("dt1.png",(output));    
                }
                if (ImageType::ImageDimension==3){
                    ImageUtils<ImageType>::writeImage("dt1.nii",(output));
                }
            }
        }

        inline virtual  double getPotential(IndexType targetIndex1, IndexType targetIndex2,LabelType displacement, int segmentationLabel){
            double result=0;
            ContinuousIndexType idx2(targetIndex2);
            itk::Vector<float,ImageType::ImageDimension> disp=displacement;
            idx2+= disp;
            if (this->m_baseLabelMap){
                itk::Vector<float,ImageType::ImageDimension> baseDisp=this->m_baseLabelMap->GetPixel(targetIndex2);
                idx2+=baseDisp;
            }
            int deformedAtlasSegmentation=-1;
            double distanceToDeformedSegmentation;
            if (!this->m_atlasSegmentationInterpolator->IsInsideBuffer(idx2)){
                for (int d=0;d<ImageType::ImageDimension;++d){
                    if (idx2[d]>=this->m_atlasSegmentationInterpolator->GetEndContinuousIndex()[d]){
                        idx2[d]=this->m_atlasSegmentationInterpolator->GetEndContinuousIndex()[d]-0.5;
                    }
                    else if (idx2[d]<this->m_atlasSegmentationInterpolator->GetStartContinuousIndex()[d]){
                        idx2[d]=this->m_atlasSegmentationInterpolator->GetStartContinuousIndex()[d]+0.5;
                    }
                }
            } 
            deformedAtlasSegmentation=int(this->m_atlasSegmentationInterpolator->EvaluateAtContinuousIndex(idx2));
            int bone=(300+1000)*255.0/2000;
            int tissue=(-500+1000)*255.0/2000;
            double atlasIntens=this->m_atlasInterpolator->EvaluateAtContinuousIndex(idx2);
            bool p_bone_SA=atlasIntens>bone;
            bool p_tiss_SA=atlasIntens<tissue;
            distanceToDeformedSegmentation=this->m_atlasDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2);

            if (deformedAtlasSegmentation>0){
                if (segmentationLabel== 0){
                    //atlas 
                    result=fabs(distanceToDeformedSegmentation)/((this->sigma1));//1;
                    
                }else if (segmentationLabel ==this->m_nSegmentationLabels - 1 ){
                    //agreement
                    result=0;
                }
                else if (segmentationLabel){
                    //never label different structure
                    if(p_bone_SA){
                        result=99999999999;
                    }
                }
                
            }else{
                //atlas is not segmented...
                if (segmentationLabel== this->m_nSegmentationLabels - 1){
                    //if we want to label foreground, cost is proportional to distance to atlas foreground
                    distanceToDeformedSegmentation=fabs(this->m_atlasDistanceTransformInterpolator->EvaluateAtContinuousIndex(idx2));
#if 1       
                    if (distanceToDeformedSegmentation>this->m_threshold){
                        result=1000;
                    }
                    else
#endif
                        {
                            result=(distanceToDeformedSegmentation)/1.0;//((this->sigma1));//1;
                            
                        }
                    if(p_bone_SA){
                        //never ever label as foreground bone in case the atlas is not labeled as bone, but has bone-like intensities!!
                        result=99999999999;
                    }
                }else if (segmentationLabel ){
                    //now we've got a non-target segmentation label and what we do with it will be decided based on the intensities of atlas and image
                    //double targetIntens=this->m_targetImage->GetPixel(targetIndex1);
                    if(false && p_tiss_SA){
                        //if atlas is clearly not bone, don;t allow any bone label
                        result=99999999999;
                    }else{
                        //otherwise, alternative bone label gets more probably the further the point is away from the deformed GT segmentation?
                        if (!distanceToDeformedSegmentation){
                            //distance is even zero, dont allow label 1 at all
                            result=99999999999;
                        }else{
                            result=1.0/(distanceToDeformedSegmentation);
                            //cout<<result<<endl;
                        }
                    }
                }
                //cout<<segmentationLabel<<" "<<result<<endl;
            }
            //cout<<result<<endl;
            return result;
        }
    };//class
    template<class TImage>
    class PairwisePotentialCoherenceBinary :public PairwisePotentialCoherence<TImage>{
    public:
        //itk declarations
        typedef PairwisePotentialCoherenceBinary            Self;
        typedef SmartPointer<Self>        Pointer;
        typedef SmartPointer<const Self>  ConstPointer;

        typedef	TImage ImageType;
        typedef typename ImageType::Pointer ImagePointerType;
        typedef typename ImageType::ConstPointer ConstImagePointerType;
        typedef typename itk::Image<float,ImageType::ImageDimension> FloatImageType;
        typedef typename FloatImageType::Pointer FloatImagePointerType;
        
        typedef typename  itk::Vector<float,ImageType::ImageDimension>  LabelType;
        typedef typename itk::Image<LabelType,ImageType::ImageDimension> LabelImageType;
        typedef typename LabelImageType::Pointer LabelImagePointerType;
        typedef typename ImageType::IndexType IndexType;
        typedef typename ImageType::SizeType SizeType;
        typedef typename ImageType::SpacingType SpacingType;
        typedef LinearInterpolateImageFunction<ImageType> ImageInterpolatorType;
        typedef typename ImageInterpolatorType::Pointer ImageInterpolatorPointerType;
        typedef LinearInterpolateImageFunction<FloatImageType> FloatImageInterpolatorType;
        typedef typename FloatImageInterpolatorType::Pointer FloatImageInterpolatorPointerType;

        typedef NearestNeighborInterpolateImageFunction<ImageType> SegmentationInterpolatorType;
        typedef typename SegmentationInterpolatorType::Pointer SegmentationInterpolatorPointerType;
        typedef typename ImageInterpolatorType::ContinuousIndexType ContinuousIndexType;
        
        typedef typename itk::StatisticsImageFilter< FloatImageType > StatisticsFilterType;
 
    public:
        itkNewMacro(Self);

        void SetReferenceSegmentation(ConstImagePointerType segImage, double scale=1.0){
            if (scale !=1.0 ){
                segImage=FilterUtils<ImageType>::NNResample(segImage,scale);
            }
          
        }

        inline virtual  double getPotential(IndexType targetIndex1, IndexType targetIndex2,LabelType displacement, int segmentationLabel){
            double result=0;
            ContinuousIndexType idx2(targetIndex2);
            itk::Vector<float,ImageType::ImageDimension> disp=displacement;
            idx2+= disp;
            if (this->m_baseLabelMap){
                itk::Vector<float,ImageType::ImageDimension> baseDisp=this->m_baseLabelMap->GetPixel(targetIndex2);
                idx2+=baseDisp;
            }
            int deformedAtlasSegmentation=-1;
            if (!this->m_atlasSegmentationInterpolator->IsInsideBuffer(idx2)){
                for (int d=0;d<ImageType::ImageDimension;++d){
                    if (idx2[d]>=this->m_atlasSegmentationInterpolator->GetEndContinuousIndex()[d]){
                        idx2[d]=this->m_atlasSegmentationInterpolator->GetEndContinuousIndex()[d]-0.5;
                    }
                    else if (idx2[d]<this->m_atlasSegmentationInterpolator->GetStartContinuousIndex()[d]){
                        idx2[d]=this->m_atlasSegmentationInterpolator->GetStartContinuousIndex()[d]+0.5;
                    }
                }
            } 
            deformedAtlasSegmentation=int(this->m_atlasSegmentationInterpolator->EvaluateAtContinuousIndex(idx2));
         
            if (deformedAtlasSegmentation>0){
                if (segmentationLabel!=deformedAtlasSegmentation){
                    result=1;
                    if (segmentationLabel==this->m_nSegmentationLabels - 1)
                        result=2;
                }
            }else{
                //atlas is not segmented...
                if (segmentationLabel== this->m_nSegmentationLabels - 1){
                    //if we want to label foreground, cost is proportional to distance to atlas foreground
                    result=2;
                }else if (segmentationLabel ){
                    result=1;
                }
            }
            return 1000*result;
        }
    };//class
   
    }//namespace
#endif /* POTENTIALS_H_ */