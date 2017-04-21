//
// Created by john on 3/28/17.
//

#ifndef KDIFF_FILEOPERATIONS_H
#define KDIFF_FILEOPERATIONS_H

#include "kdiffparser_export.h"

class KDIFFPARSER_EXPORT FileUtils {
public:
    static bool copyFile(const QUrl file, const QUrl destination, bool showProgress = false);
    static bool copyDir(const QUrl source, const QUrl destination, bool showProgress = false);
    static bool makePath(const QUrl url, bool showProgress = false);
    static bool deleteFileOrDir(QUrl url, bool showProgress = false);
    static bool isDir(QUrl url, bool showProgress = false);
};

class KDIFFPARSER_EXPORT StringUtils {
public:
    static QString leftStrip(const QString string, const QString partToStrip);
    static QString rightStrip(const QString string, const QString partToStrip);
    static const QString rightCommonPartInPaths(const QString first, const QString second);
};

#endif //KDIFF_FILEOPERATIONS_H
