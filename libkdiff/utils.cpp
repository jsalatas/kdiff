//
// Created by john on 3/28/17.
//

#include <QDebug>
#include <QDir>
#include <KLocalizedString>
#include <KMessageBox>
#include <kio/copyjob.h>
#include <kio/statjob.h>
#include <kio/filecopyjob.h>
#include <kio/mkpathjob.h>
#include <kio/deletejob.h>
#include "utils.h"


using namespace KIO;

bool FileUtils::copyFile(const QUrl file, const QUrl destination, bool showProgress)
{
    JobFlags flags = Overwrite;
    if(!showProgress) {
        flags = flags | HideProgressInfo;
    }

    KIO::FileCopyJob* copyJob = file_copy(file, destination, -1, flags);
    if(!copyJob->exec()) {
        KMessageBox::error(0, i18n("<qt>Could not save file <b>%1</b> at <b>%2</b>.\nThe file has not been saved.</qt>",
                file.toDisplayString(), destination.toDisplayString()));
        return false;
    };

    return true;
}

bool FileUtils::copyDir(const QUrl source, const QUrl destination, bool showProgress)
{
    JobFlags flags = DefaultFlags;
    if(!showProgress) {
        flags = flags | HideProgressInfo;
    }

    KIO::CopyJob* copyJob = KIO::copy(source, destination, flags);
    if(!copyJob->exec()) {
        KMessageBox::error(0, i18n("<qt>Could not save file <b>%1</b> at <b>%2</b>.\nThe file has not been saved.</qt>",
                source.toDisplayString(), destination.toDisplayString()));
        return false;
    };

    return true;
}

bool FileUtils::makePath(const QUrl url, bool showProgress)
{
    JobFlags flags = DefaultFlags;
    if(!showProgress) {
        flags = flags | HideProgressInfo;
    }

    StatJob* statJob = KIO::stat(url);
    if(!statJob->exec()) {
        MkpathJob* mkpathJob = KIO::mkpath(url, QUrl(), flags);
        if(!mkpathJob->exec()) {
            KMessageBox::error(0, i18n("<qt>Could not create destination directory <b>%1</b>.\nThe file has not been saved.</qt>",
                    url.toDisplayString()));
            return false;
        }
    }
    return true;
}

bool FileUtils::deleteFileOrDir(QUrl url, bool showProgress)
{
    JobFlags flags = DefaultFlags;
    if(!showProgress) {
        flags = flags | HideProgressInfo;
    }

    DeleteJob* deleteJob = del(url, flags);
    if(!deleteJob->exec()) {
        KMessageBox::error(0, i18n("<qt>Could not delete <b>%1</b>.</qt>", url.toDisplayString()));
        return false;
    };
    return true;
}


bool FileUtils::isDir(QUrl url, bool showProgress)
{
    JobFlags flags = DefaultFlags;
    if(!showProgress) {
        flags = flags | HideProgressInfo;
    }

    StatJob* statJob = KIO::stat(url, flags);
    KIO::UDSEntry node = statJob->statResult();

    return node.isDir();
}

QString StringUtils::leftStrip(const QString string, const QString partToStrip)
{
    if(!string.startsWith(partToStrip)) {
        return string;
    }

    return string.mid(partToStrip.length(), string.length());
}

QString StringUtils::rightStrip(const QString string, const QString partToStrip)
{
    if(!string.endsWith(partToStrip)) {
        return string;
    }

    return string.mid(0, string.length() - partToStrip.length());
}

const QString StringUtils::rightCommonPartInPaths(const QString first, const QString second)
{
    QStringList firstParts = first.split(QDir::separator(), QString::SkipEmptyParts);
    QStringList secondPathParts = second.split(QDir::separator(), QString::SkipEmptyParts);

    // Scan first and second paths' parts going backward and keeping only the common parts
    int firstIdx = firstParts.size() - 1;
    int secondIdx = secondPathParts.size() - 1;
    QString commonPath;

    while (firstIdx >= 0 && secondIdx >= 0 &&
           firstParts.at(firstIdx) == secondPathParts.at(secondIdx)) {
        if(!commonPath.isEmpty()) {
            commonPath = "/" + commonPath;
        }

        commonPath = firstParts.at(firstIdx) + commonPath;
        firstIdx--;
        secondIdx--;
    };

    return commonPath;
}

