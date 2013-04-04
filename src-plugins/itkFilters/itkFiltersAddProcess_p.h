#ifndef ITKFILTERSADDPROCESS_P_H
#define ITKFILTERSADDPROCESS_P_H

#include <dtkCore/dtkAbstractData.h>
#include <dtkCore/dtkSmartPointer.h>

#include "itkFiltersProcessBase_p.h"

#include <medMetaDataKeys.h>

#include "itkImage.h"
#include "itkCommand.h"
#include "itkAddConstantToImageFilter.h"

class itkFiltersAddProcess;

class itkFiltersAddProcessPrivate : public itkFiltersProcessBasePrivate
{
public:
    itkFiltersAddProcessPrivate(itkFiltersAddProcess *q = 0) : itkFiltersProcessBasePrivate(q) {}
    itkFiltersAddProcessPrivate(const itkFiltersAddProcessPrivate& other) : itkFiltersProcessBasePrivate(other) {}

    virtual ~itkFiltersAddProcessPrivate(void) {}
    
    double addValue;
    
    template <class PixelType> void update ( void )
    {        
        typedef itk::Image< PixelType, 3 > ImageType;
        typedef itk::AddConstantToImageFilter< ImageType, double, ImageType >  AddFilterType;
        typename AddFilterType::Pointer addFilter = AddFilterType::New();
        
        addFilter->SetInput ( dynamic_cast<ImageType *> ( ( itk::Object* ) ( input->data() ) ) );
        addFilter->SetConstant ( addValue );
        
        callback = itk::CStyleCommand::New();
        callback->SetClientData ( ( void * ) this );
        callback->SetCallback ( itkFiltersProcessBasePrivate::eventCallback );
        
        addFilter->AddObserver ( itk::ProgressEvent(), callback );
        
        addFilter->Update();
        output->setData ( addFilter->GetOutput() );
        
        QString newSeriesDescription = input->metadata ( medMetaDataKeys::SeriesDescription.key() );
        newSeriesDescription += " add filter (" + QString::number(addValue) + ")";
        
        output->addMetaData ( medMetaDataKeys::SeriesDescription.key(), newSeriesDescription );
    }
};

DTK_IMPLEMENT_PRIVATE(itkFiltersAddProcess, itkFiltersProcessBase)

#endif // ITKFILTERSADDPROCESS_P_H