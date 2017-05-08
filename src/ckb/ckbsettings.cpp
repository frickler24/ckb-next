#include "ckbsettings.h"
#include "ckbsettingswriter.h"
#include <QThread>
#include <QMutex>
#include <QFile>
#include <QDebug>

// Shared global QSettings object
static QSettings* _globalSettings = 0;
static QThread* globalThread = 0;
QAtomicInt cacheWritesInProgress(0);
// Global one-shot settings cache, to avoid reading/writing QSettings constantly
static QMap<QString, QVariant> globalCache;
// Mutexes for accessing settings
QMutex settingsMutex(QMutex::Recursive), settingsCacheMutex(QMutex::Recursive);
#define lockMutex           QMutexLocker locker(backing == _globalSettings ? &settingsMutex : 0)
#define lockMutexStatic     QMutexLocker locker(&settingsMutex)
#define lockMutexStatic2    QMutexLocker locker2(&settingsMutex)
#define lockMutexCache      QMutexLocker locker(&settingsCacheMutex)

#if QT_VERSION < QT_VERSION_CHECK(5, 5, 0)
#define qInfo       qDebug
#define qWarning    qDebug
#define qFatal      qDebug
#define qCritical   qDebug
#endif

///
/// \brief globalSettings
/// \return QSettings* _globalsettings
/// Opens the global settings organized by QSettings class (machine independent implementation).
/// Settings contain key definitions, colors, macros, settings for the profiles and so on.
///
static QSettings* globalSettings(){
    if(!_globalSettings){
        lockMutexStatic;
        if(!(volatile QSettings*)_globalSettings){   // Check again after locking mutex in case another thread created the object
            /// first check if QSettings structures can be written in the filesystem.
            /// Try to open the standard config file and to write something into it. Close the file.
            ///
            CkbSettings::setWritable(false);
            QSettings* testSettings = new QSettings;
            testSettings->setValue("testIfWritable", 42);
            testSettings->sync();

            QString settingsFileName = testSettings->fileName();
            delete testSettings;

            bool fileWriteAccess = false;
            int lastSlashInName = settingsFileName.lastIndexOf("/");
            if (lastSlashInName != -1) {
                QString newFileName = settingsFileName.left(settingsFileName.lastIndexOf("/")).append("/testFileToCheckWriteAccess");
                QFile testFile(newFileName);
                try {
                    if (testFile.open(QIODevice::WriteOnly | QIODevice::Text)) {
                        QTextStream out(&testFile);
                        out << "The magic number is: " << 42 << "\n";
                        testFile.close();
                        fileWriteAccess = QFile::remove(newFileName);
                    }
                } catch(int e) {
                    fileWriteAccess = false;
                }
            }

            /// Try to open the conf file again and to read the standard value. Delete the standard value afterwards.
            /// Because on linux QSettings does somehow caching, it reads the correct value even the file (not the directory) is read only.
            /// So we need to add the original isWritable() function.
            ///
            testSettings = new QSettings;
            if ((testSettings->value("testIfWritable").toInt() == 42) && testSettings->isWritable()) {
                CkbSettings::setWritable(fileWriteAccess);
                testSettings->remove("testIfWritable");
                testSettings->sync();
            }
            delete testSettings;
            testSettings = 0;

            /// Put the settings object in a separate thread so that it won't lock up the GUI when it syncs
            globalThread = new QThread;
            globalThread->start();
            _globalSettings = new QSettings;
            qInfo() << "Path  to settings is" << _globalSettings->fileName();
            _globalSettings->moveToThread(globalThread);
        }
    }
    return _globalSettings;
}

///
/// \brief CkbSettings::_writable
///
bool CkbSettings::_writable = false;

///
/// \brief CkbSettings::isWritable Remember if the config files are writable (boolean getter)
/// \return the value of CkbSettings::writable
///
bool CkbSettings::isWritable() { return _writable; }

///
/// \brief CkbSettings::setWritable Remember if the config files are writable (setter)
/// \param v
/// Set the value of CkbSettings::writable
///
void CkbSettings::setWritable(bool v) { _writable = v; }

///
/// \brief CkbSettings::checkIfWritable
/// If the local implementation of the config database is not yet writable,
/// bring up a popup to the user to informhim about it.
/// Bring up the information where he can find the info.
bool CkbSettings::informIfNonWritable() {
    if (isWritable()) return false;
    QMessageBox msgBox;
    msgBox.setIcon(QMessageBox::Warning);
    msgBox.setProperty("Title", "Profile is read only");
    msgBox.setText("Your profile information for ckb-next is nonwritable.");
    QString info = "This might happen if you did start the ckb-next program with root privileges earlier.\n\nOr did you copy it from somewhere?\n\nPlease have a look at\n"
            + _globalSettings->fileName();
    msgBox.setInformativeText(info);
    msgBox.exec();
    qDebug() << "Profile information for ckb-next is nonwritable. It is located at" << _globalSettings->fileName();
    return true;
}


bool CkbSettings::isBusy(){
    return cacheWritesInProgress.load() > 0;
}

void CkbSettings::cleanUp(){
    if(!_globalSettings)
        return;
    // Wait for all writers to finish
    while(cacheWritesInProgress.load() > 0)
        QThread::yieldCurrentThread();
    // Stop thread and delete objects
    globalThread->quit();
    globalThread->wait();
    delete globalThread;
    delete _globalSettings;
    globalThread = 0;
    _globalSettings = 0;
}

CkbSettings::CkbSettings() :
    backing(globalSettings()) {
}

CkbSettings::CkbSettings(const QString& basePath, bool eraseExisting) :
    backing(globalSettings()) {
    if(basePath.isEmpty()){
        if(eraseExisting)
            qDebug() << "CkbSettings created with basePath = \"\" and eraseExisting = true. This is a mistake.";
        return;
    }
    if(eraseExisting)
        remove(basePath);
    beginGroup(basePath);
}

CkbSettings::CkbSettings(QSettings& settings) :
    backing(&settings) {
}

void CkbSettings::beginGroup(const QString& prefix){
    groups.append(prefix);
}

void CkbSettings::endGroup(){
    groups.removeLast();
}

QStringList CkbSettings::childGroups() const {
    QString current = pwd();
    lockMutex;
    if(!current.isEmpty())
        backing->beginGroup(current);
    QStringList res = backing->childGroups();
    if(!current.isEmpty())
        backing->endGroup();
    return res;
}

QStringList CkbSettings::childKeys() const {
    QString current = pwd();
    lockMutex;
    if(!current.isEmpty())
        backing->beginGroup(current);
    QStringList res = backing->childKeys();
    if(!current.isEmpty())
        backing->endGroup();
    return res;
}

bool CkbSettings::contains(const QString& key) const {
    lockMutex;
    return backing->contains(pwd(key));
}

bool CkbSettings::containsGroup(const QString &group){
    QStringList components = group.split("/");
    if(components.length() > 1){
        // Find sub-group
        SGroup group(*this, components[0]);
        return containsGroup(QStringList(components.mid(1)).join('/'));
    }
    return childGroups().contains(group);
}

QVariant CkbSettings::value(const QString& key, const QVariant& defaultValue) const {
    lockMutex;
    return backing->value(pwd(key), defaultValue);
}

void CkbSettings::setValue(const QString& key, const QVariant& value){
    // Cache the write values, save them when the object is destroyed
    QString realKey = pwd(key);
    writeCache[realKey] = value;
    // Update global cache if needed
    lockMutexCache;
    if(globalCache.contains(realKey))
        globalCache[realKey] = value;
}

void CkbSettings::remove(const QString& key){
    removeCache.append(pwd(key));
}

CkbSettings::~CkbSettings(){
    if(removeCache.isEmpty() && writeCache.isEmpty())
        return;
    // Save the settings from the settings thread.
    // They have to be saved from that thread specifically to avoid performance issues
    CkbSettingsWriter* writer = new CkbSettingsWriter(backing, removeCache, writeCache);
    writer->moveToThread(globalThread);
    QObject::staticMetaObject.invokeMethod(writer, "run", Qt::QueuedConnection);
}

QVariant CkbSettings::get(const QString& key, const QVariant& defaultValue){
    // Store these settings in a memory cache
    lockMutexCache;
    if(globalCache.contains(key))
        return globalCache.value(key);
    // If it wasn't found in the memory cache, look for it in QSettings
    lockMutexStatic2;
    QSettings* settings = globalSettings();
    return globalCache[key] = settings->value(key, defaultValue);
}

void CkbSettings::set(const QString& key, const QVariant& value){
    {
        lockMutexCache;
        if(globalCache.value(key) == value)
            return;
        globalCache[key] = value;
    }
    lockMutexStatic;
    globalSettings()->setValue(key, value);
}
