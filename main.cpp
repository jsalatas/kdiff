#include <cstdlib>

#include <QApplication>
#include <QCommandLineParser>

#include <KAboutData>
#include <KLocalizedString>

#include "mainwindow.h"

int main (int argc, char *argv[])
{
    QApplication app(argc, argv);
    KLocalizedString::setApplicationDomain("kdiff");

    KAboutData aboutData(
            QStringLiteral("kdiff"),
            i18n("KDiff"),
            QStringLiteral("0.1.0"),
            i18n("Compare Directories and Files using GNU's diff"),
            KAboutLicense::GPL,
            i18n("(c) 2017 John Salatas"),
            nullptr,
            QStringLiteral("https://github.com/jsalatas/kdiff"),
            QStringLiteral("https://github.com/jsalatas/kdiff/issues"));
    aboutData.addAuthor(i18n("John Salatas"), QString(), QStringLiteral("jsalatas@gmail.com"),
                        QStringLiteral("http://jsalatas.ictpro.gr/"));
    KAboutData::setApplicationData(aboutData);
    app.setWindowIcon(QIcon::fromTheme(QStringLiteral("kompare")));

    QCommandLineParser parser;
    // TODO: need to parse parameters from the command line
    parser.addHelpOption();
    parser.addVersionOption();
    aboutData.setupCommandLine(&parser);
    parser.process(app);
    aboutData.processCommandLine(&parser);

    KDiffMainWindow* window = new KDiffMainWindow();
    // TODO: need to pass parsed parameters from the command line

    window->show();

    return app.exec();
}