#include "medTest.h"

#include <medPluginManager.h>
#include <medDataManager.h>
#include <medDataIndex.h>

namespace medTest
{
    bool envString (const QString &var, QString &value)
    {
        value = qgetenv (var.toLatin1().constData()).constData();
        if (value.isEmpty()) {
            return false;
            qDebug() << var + " is not defined";
        }
        return true;
    }

    void initializePlugins ()
    {
        medPluginManager::instance()->setVerboseLoading (true);
        QString paths;
        QVERIFY(medTest::envString ("MEDINRIA_PLUGIN_PATH", paths)); // returns if env. variable is not defined
        medPluginManager::instance()->initialize();
        medPluginManager::instance()->printPlugins();
    }

    void uninitializePlugins ()
    {
        medPluginManager::instance()->uninitialize();
    }

    void initializeResources (const QString &var)
    {
        QString value;
        QVERIFY(medTest::envString (var, value));
        QResource::registerResource(value.toLatin1().constData());
    }

    dtkSmartPointer<dtkAbstractData> readData (const QString &file)
    {
        /*
        medSynchronousDbImporter importer (file);
        importer.setTimeout(0);
        medDataIndex index = importer.synchImport();

        return medDataManager::instance()->data(index);
        */
    }

} // end of namespace