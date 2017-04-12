#include "mainwindow.h"
#include <QApplication>
#include <QDateTime>
#include <QSharedMemory>
#include <QCommandLineParser>
#include <cstring>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include "debug.h"
#include <QtGlobal>
#include <stdio.h>

QSharedMemory appShare("ckb");

#ifdef Q_OS_MACX
// App Nap is an OSX feature designed to save power by putting background apps to sleep.
// However, it interferes with ckb's animations, so we want it off
extern "C" void disableAppNap();
#endif

// Shared memory sizes
#define SHMEM_SIZE      128 * 1024
#define SHMEM_SIZE_V015 16

// Command line options
enum CommandLineParseResults {
    CommandLineOK,
    CommandLineError,
    CommandLineVersionRequested,
    CommandLineHelpRequested,
    CommandLineClose,
    CommandLineBackground,
    CommandLineDebug
};

/**
 * parseCommandLine - Setup options and parse command line arguments.
 *
 * @param parser        parser instance to use for parse the arguments
 * @param errorMessage  argument parse error message
 *
 * @return  integer, representing the requested argument
 */
CommandLineParseResults parseCommandLine(QCommandLineParser &parser, QString *errorMessage) {
    // setup parser to interpret -abc as -a -b -c
    parser.setSingleDashWordOptionMode(QCommandLineParser::ParseAsCompactedShortOptions);

    /* add command line options */
    // add -v, --version
    const QCommandLineOption versionOption = parser.addVersionOption();
    // add -h, --help (/? on windows)
    const QCommandLineOption helpOption = parser.addHelpOption();
    // add -b, --background
    const QCommandLineOption backgroundOption(QStringList() << "b" << "background",
                                              "Starts in background, without displaying the main window.");
    parser.addOption(backgroundOption);
    // add -c, --close
    const QCommandLineOption closeOption(QStringList() << "c" << "close",
                                         "Causes already running instance (if any) to exit.");
    parser.addOption(closeOption);
    // add -d, --debug
    const QCommandLineOption debugOption(QStringList() << "d" << "debuglevel",
                                         "sets the debug level to <level> (0=min..7=max).", "level");
    parser.addOption(debugOption);

    /* parse arguments */
    if (!parser.parse(QCoreApplication::arguments())) {
        // set error, if there already are some
        *errorMessage = parser.errorText();
        return CommandLineError;
    }

    /* return requested operation/setup */
    if (parser.isSet(versionOption)) {
        // print version and exit
        return CommandLineVersionRequested;
    }

    if (parser.isSet(helpOption)) {
        // print help and exit
        return CommandLineHelpRequested;
    }

    if (parser.isSet(backgroundOption)) {
        // open application in background
        return CommandLineBackground;
    }

    if (parser.isSet(closeOption)) {
        // close already running application instances, if any
        return CommandLineClose;
    }

    if (parser.isSet(debugOption)) {
        // set the debug level to the given value
        return CommandLineDebug;
    }

    /* no explicit argument was passed */
    return CommandLineOK;
};

// Scan shared memory for an active PID
static bool pidActive(const QStringList& lines){
    foreach(const QString& line, lines){
        if(line.startsWith("PID ")){
            bool ok;
            pid_t pid;
            // Valid PID found?
            if((pid = line.split(" ")[1].toLong(&ok)) > 0 && ok){
                // kill -0 does nothing to the application, but checks if it's running
                return (kill(pid, 0) == 0 || errno != ESRCH);
            }
        }
    }
    // If the PID wasn't listed in the shmem, assume it is running
    return true;
}

// Check if the application is running. Optionally write something to its shared memory.
static bool isRunning(const char* command){
    // Try to create shared memory (if successful, application was not already running)
    if(!appShare.create(SHMEM_SIZE) || !appShare.lock()){
        // Lock existing shared memory
        if(!appShare.attach() || !appShare.lock())
            return true;
        bool running = false;
        if(appShare.size() == SHMEM_SIZE_V015){
            // Old shmem - no PID listed so assume the process is running, and print the command directly to the buffer
            if(command){
                void* data = appShare.data();
                snprintf((char*)data, SHMEM_SIZE_V015, "%s", command);
            }
            running = true;
        } else {
            // New shmem. Scan the contents of the shared memory for a PID
            QStringList lines = QString((const char*)appShare.constData()).split("\n");
            if(pidActive(lines)){
                running = true;
                // Append the command
                if(command){
                    lines.append(QString("Option ") + command);
                    QByteArray newMem = lines.join("\n").left(SHMEM_SIZE).toLatin1();
                    // Copy the NUL byte as well as the string
                    memcpy(appShare.data(), newMem.constData(), newMem.length() + 1);
                }
            }
        }
        if(running){
            appShare.unlock();
            return true;
        }
    }
    // Not already running. Initialize the shared memory with our PID
    snprintf((char*)appShare.data(), appShare.size(), "PID %ld", (long)getpid());
    appShare.unlock();
    return false;
}

///
/// \brief myDebugLevel static definition;
/// If you  want to change the initializing value,
/// please change the assignment to DEBUGLEVEL_INITIAL in debug.h, not here.
static int myDebugLevel = DEBUGLEVEL_INITIAL;

///
/// \brief setMyDebugLevel C-function to set the current debugLevel to newDebugLevel
/// \param newDebugLevel Value between DEBUGLEVEL_FATAL and DEBUGLEVEL_MAX,
/// see \a debug.h and \a myMessageOutput().
///
void setMyDebugLevel (int newDebugLevel) {
    myDebugLevel = newDebugLevel;
}

///
/// \brief myMessageOutput C-Function to handle messages via qDebug, qWarning, qInfo, qCritical
/// (and somewhen later: qFatal).
/// This function is "loaded" in \a main() via \a qInstallMessageHandler().
/// Depending on the value of \a myDebugLevel we print either nothing, a short form or long form
/// of the message. If ckb is compiled without debug-mode, then QT_NO_DEBUG will be set
/// and we generate the short form only (because without debug-mode no code-info is available).
///
/// - Short form is "<type>: <msg>\n"
/// - Long form is  "<type>: <msg> (<file>:<line>, <function>)\n"
/// All three parameters are documented in \a qInstallMessageHandler().
/// \param type
/// \param context
/// \param msg
///
void myMessageOutput (QtMsgType type, const QMessageLogContext &context, const QString &msg) {
    QByteArray localMsg = msg.toLocal8Bit();
    char textBuffer [DEBUGBUFFERSIZE];
    char longForm [] = "%s (%s:%u, %s)";
    char shortForm [] = "%s";

    switch (myDebugLevel) {
    case DEBUGLEVEL_FATAL:
    case DEBUGLEVEL_CRITICAL:
    case DEBUGLEVEL_WARNING:
    case DEBUGLEVEL_INFO:
        snprintf(textBuffer, DEBUGBUFFERSIZE, shortForm, localMsg.constData());
        break;
    case DEBUGLEVEL_FATAL_LONG:
    case DEBUGLEVEL_CRITICAL_LONG:
    case DEBUGLEVEL_WARNING_LONG:
    case DEBUGLEVEL_ALL:
#ifdef QT_NO_DEBUG
        snprintf(textBuffer, DEBUGBUFFERSIZE, shortForm, localMsg.constData());
#else
        snprintf(textBuffer, DEBUGBUFFERSIZE, longForm, localMsg.constData(), context.file, context.line, context.function);
#endif
        break;
    default:
        // FATAL Error, because value of myDebugLevel should be 0..7
        snprintf(textBuffer, DEBUGBUFFERSIZE, longForm, "Fatal", "wrong value of myDebugLevel in myMessageOutput", context.file, context.line, context.function);
    }

    switch (type) {
    case QtDebugMsg:
        if (myDebugLevel == DEBUGLEVEL_ALL) fprintf(stderr, "%s: %s\n", "Debug", textBuffer);
        break;
    case QtInfoMsg:
        if (myDebugLevel >= DEBUGLEVEL_INFO) fprintf(stderr, "%s: %s\n", "Info", textBuffer);
        break;
    case QtWarningMsg:
        if (myDebugLevel >= DEBUGLEVEL_WARNING) fprintf(stderr, "%s: %s\n", "Warning", textBuffer);
        break;
    case QtCriticalMsg:
        if (myDebugLevel >= DEBUGLEVEL_CRITICAL) fprintf(stderr, "%s: %s\n", "Critical", textBuffer);
        break;
    case QtFatalMsg:
        fprintf(stderr, "%s: %s\n", "Fatal", textBuffer);
        abort();
    }
}

///
/// \brief main
/// \param argc
/// \param argv
/// \return
///
int main(int argc, char *argv[]){
    // Setup main application
    QApplication a(argc, argv);

    /// Setup debug info handler and debugLevel
    qInstallMessageHandler(myMessageOutput); // Install the handler

    // Setup names and versions
    QCoreApplication::setOrganizationName("ckb");
    QCoreApplication::setApplicationVersion(CKB_VERSION_STR);
    QCoreApplication::setApplicationName("ckb");

    // Setup argument parser
    QCommandLineParser parser;
    QString errorMessage;
    parser.setApplicationDescription("Open Source Corsair Input Device Driver for Linux and OSX.");
    bool background = 0;

    // Although the daemon runs as root, the GUI needn't and shouldn't be, as it has the potential to corrupt settings data.
    if(getuid() == 0){
        printf("The ckb GUI should not be run as root.\n");
        return 0;
    }

    // Seed the RNG for UsbIds
    qsrand(QDateTime::currentMSecsSinceEpoch());
#ifdef Q_OS_MACX
    disableAppNap();
#endif
#if QT_VERSION >= QT_VERSION_CHECK(5, 3, 0)
    // Enable HiDPI support
    qApp->setAttribute(Qt::AA_UseHighDpiPixmaps);
#endif

    // Parse arguments
    switch (parseCommandLine(parser, &errorMessage)) {
    case CommandLineOK:
        // If launched with no argument
        break;
    case CommandLineError:
        fputs(qPrintable(errorMessage), stderr);
        fputs("\n\n", stderr);
        fputs(qPrintable(parser.helpText()), stderr);
        return 1;
    case CommandLineVersionRequested:
        // If launched with --version, print version info and then quit
        printf("%s %s\n", qPrintable(QCoreApplication::applicationName()),
               qPrintable(QCoreApplication::applicationVersion()));
        return 0;
    case CommandLineHelpRequested:
        // If launched with --help, print help and then quit
        parser.showHelp();
        return 0;
    case CommandLineClose:
        // If launched with --close, kill existing app
        if (isRunning("Close"))
            printf("Asking existing instance to close.\n");
        else
            printf("ckb is not running.\n");
        return 0;
    case CommandLineBackground:
        // If launched with --background, launch in background
        background = 1;
        break;
    case CommandLineDebug:
        // a value should be given
        setMyDebugLevel(parser.value("d").toUInt());
        printf ("DebugLevel is set to %d\n", parser.value("d").toUInt());
        break;
    }

    qDebug() << "Debug messages are shown";
    qInfo() << "Info messages are shown";
    qWarning() << "Warnings are shown";
    qCritical() << "Critical messages are shown";
    //qFatal() << "Meine erste Fatal Info";

    // Launch in background if requested, or if re-launching a previous session
    if(qApp->isSessionRestored())
            background = 1;
    if(isRunning(background ? 0 : "Open")){
        printf("ckb is already running. Exiting.\n");
        return 0;
    }
    MainWindow w;
    if(!background)
        w.show();

    return a.exec();
}
