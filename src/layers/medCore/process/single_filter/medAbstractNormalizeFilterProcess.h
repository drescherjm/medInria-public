/*=========================================================================

 medInria

 Copyright (c) INRIA 2013 - 2014. All rights reserved.
 See LICENSE.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

=========================================================================*/

#pragma once

#include <medAbstractSingleFilterOperationProcess.h>

#include <dtkCore>

#include <medCoreExport.h>

class MEDCORE_EXPORT medAbstractNormalizeFilterProcess: public medAbstractSingleFilterOperationProcess
{
    Q_OBJECT
public:
    medAbstractNormalizeFilterProcess(QObject *parent): medAbstractSingleFilterOperationProcess(parent) {}

protected:
    virtual QString outputNameAddon() const {return "normalize filter";}
};

DTK_DECLARE_PLUGIN_FACTORY(medAbstractNormalizeFilterProcess, MEDCORE_EXPORT)
