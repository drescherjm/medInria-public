/*=========================================================================

 medInria

 Copyright (c) INRIA 2013 - 2014. All rights reserved.
 See LICENSE.txt for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.

=========================================================================*/

#include <medAddImageProcessNode.h>

#include <medCore.h>

medAddImageProcessNode::medAddImageProcessNode()
{
    this->setFactory(medCore::arithmeticOperation::addImage::pluginFactory());
}


