#include <stdlib.h>
#include <stdio.h>
#include <iostream>

#include <dtkCore/dtkPluginManager.h>

#include <medSql/medDatabaseController.h>
#include <medSql/medDatabaseImporter.h>
#include <medCore/medStorage.h>
#include <medCore/medDataIndex.h>

#include <QtCore>

bool checkDirectoryHierarchyAndThumbnails(QString dbPath, QString patientName, QString studyName, QString sessionName, unsigned int thumbnailsCount);
int countRowsInTable(QSqlDatabase db, QString tableName);
bool checkIfRowExists(QSqlDatabase db, QString tableName, QString columnName, QString value);
bool checkIfRowExists(QSqlDatabase db, QString tableName, QHash<QString, QString> columnValues);

int medDatabaseControllerImplTest(int argc, char* argv[])
{
    // this test requires medinria plugins path and medinria data path
    if (argc < 3)
      return EXIT_FAILURE;

    // get the input directories
    QString pluginsPath = argv[1];
    QString dataDir = argv[2];

    // load medinria plugins, we will need data types, readers and writers
    dtkPluginManager::instance()->setPath(pluginsPath);
    dtkPluginManager::instance()->initialize();

    // create temporary directory for new database
    QString dbPath = QDir::tempPath() + "/medinria-test-db";

    medStorage::setDataLocation(dbPath);

    QPointer<medDatabaseControllerImpl> dbc = medDatabaseController::instance();

    // create connection and create database schema
    if(!dbc->createConnection())
    {
        qWarning() << "Unable to create a connection to the database";
        return EXIT_FAILURE;
    }

    QString file = dataDir + QString("/Clatz/I1.nhdr");
    QFileInfo info(file);
    bool indexWithoutCopying = false;

    // import
    medDatabaseImporter *importer = new medDatabaseImporter(info.absoluteFilePath(), indexWithoutCopying);
    importer->run();

    /* CHECK
     * We won't do a full and exhaustive check in here, we will
     * see if the appropriate folders were created, we will
     * check thumbnails image files (only by name) and then we
     * will take a deep look at the database tables.
     */

    QString patientName = "John Doe";
    QString studyName = "EmptyStudy";
    QString sessionName = "I11";

    if(!checkDirectoryHierarchyAndThumbnails(dbPath, patientName, studyName, sessionName, 63))
        return EXIT_FAILURE;

    // super, so we reached a point that all the files we expect are there
    // however we didn't check the content, but still...
    // now onto db check

    QSqlDatabase db = *(dbc->database());

    // check that there is only one row in patient table
    if(1 != countRowsInTable(db, "patient"))
        return EXIT_FAILURE;

    // check that patient named John Doe exists
    // we do not check for all the other columns
    if(!checkIfRowExists(db, "patient", "name", patientName))
        return EXIT_FAILURE;

    // check that there is only one row in study table
    if(1 != countRowsInTable(db, "study"))
        return EXIT_FAILURE;

    // check that there is a study named EmptyStudy
    // we do not check for all the other columns
    if(!checkIfRowExists(db, "study", "name", studyName))
        return EXIT_FAILURE;

    // check that there is only one row in series table
    if(1 != countRowsInTable(db, "series"))
        return EXIT_FAILURE;

    // check that the proper path is in series table
    QString seriesPath = QDir::separator() + patientName + QDir::separator()  + studyName + QDir::separator() + sessionName + QString(".mha");
    if(!checkIfRowExists(db, "series", "path", seriesPath))
        return EXIT_FAILURE;

    int imageCount = 64;

    // check that there are #imageCount rows in image table
    if (imageCount != countRowsInTable(db, "image"))
        return EXIT_FAILURE;

    // check every row
    for (int i = 0; i < imageCount ; i++)
    {
        QHash<QString, QString> columnValues;
        columnValues.insert("size", QString::number(imageCount));
        columnValues.insert("name", "I1.nhdr" + QString::number(i));
        columnValues.insert("path", file);
        columnValues.insert("instance_path", seriesPath);
        QString thumbnailPath = QDir::separator() + patientName + QDir::separator()  + studyName + QDir::separator() + sessionName + QDir::separator() + QString::number(i) + ".png";
        columnValues.insert("thumbnail", thumbnailPath);

        if(!checkIfRowExists(db, "image", columnValues))
            return EXIT_FAILURE;
    }

    // clean
    medStorage::removeDir(dbPath, NULL);
    medDatabaseController::destroy();

    return EXIT_SUCCESS;
}

bool checkDirectoryHierarchyAndThumbnails(QString dbPath, QString patientName, QString studyName, QString sessionName, unsigned int thumbnailsCount)
{
    // We do not check the content of the files
    // we just see whether they exist

    // Patient
    QString patientPath = dbPath + QDir::separator() + patientName;
    if (QDir(patientPath).exists())
    {
        QString studyPath = patientPath + QDir::separator() + studyName;
        if (QDir(studyPath).exists())
        {
            QString sessionPath = studyPath + QDir::separator() + sessionName;
            QString sessionFile = sessionPath + QString(".mha");
            if (QDir(sessionPath).exists() && QFile(sessionFile).exists())
            {
                // check if thumbnails exist
                for (int i = 0; i <= thumbnailsCount; i++) {
                    QFile thumbnail(sessionPath + QDir::separator() + QString::number(i) + QString(".png"));
                    if (!thumbnail.exists())
                        return false;
                }

                // finally check ref.png
                QFile ref(sessionPath + QDir::separator() + QString("ref.png"));
                if (!ref.exists())
                    return false;

                return true;
            }
            else
            {
                return false;
            }
        }
        else
        {
            return false;
        }
    }
    else
    {
        return false;
    }
}

int countRowsInTable(QSqlDatabase db, QString tableName)
{
    QSqlQuery query(db);

    query.prepare("SELECT COUNT(*) FROM " + tableName);

    if(!query.exec())
    {
        qDebug() << DTK_COLOR_FG_RED << query.lastError() << DTK_NO_COLOR;
        return -1;
    }

    if(query.first()) {
        QVariant rowsCount = query.value(0);
        return rowsCount.toInt();
    }
    else
        return -1;
}

bool checkIfRowExists(QSqlDatabase db, QString tableName, QString columnName, QString value)
{
    QSqlQuery query(db);

    query.prepare("SELECT id FROM " + tableName + " WHERE " + columnName + " = :value");
    query.bindValue(":value", value);

    if(!query.exec())
    {
        qDebug() << DTK_COLOR_FG_RED << query.lastError() << DTK_NO_COLOR;
        return false;
    }

    if(query.first()) {
        QVariant patientId = query.value(0);
        return true;
    }
    else
        return false;
}

bool checkIfRowExists(QSqlDatabase db, QString tableName, QHash<QString, QString> columnValues)
{
    // TODO this way of building the query string is just awful
    // try to find another...

	QString queryString = "SELECT id FROM " + tableName + " WHERE ";

	QHashIterator<QString, QString> it(columnValues);
    bool firstTime = true;

    while (it.hasNext())
    {
        it.next();

        if (!firstTime)
        {
            queryString.append(" AND ");
        }
        else
            firstTime = false;

        queryString.append(it.key()).append( " = :").append(it.key());
    }

    QSqlQuery query(db);
    query.prepare(queryString);

    it.toFront();

    while (it.hasNext())
    {
        it.next();
        query.bindValue(":"+it.key(), it.value());
    }

    if(!query.exec())
    {
        qDebug() << DTK_COLOR_FG_RED << query.lastError() << DTK_NO_COLOR;
        return false;
    }

	
    // see executed query
	//qDebug() << query.executedQuery().toStdString().c_str();

    if(query.first()) {
        QVariant patientId = query.value(0);
        return true;
    }
    else
        return false;
}
