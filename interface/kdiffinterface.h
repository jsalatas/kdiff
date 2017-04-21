//
// Created by john on 3/9/17.
//

#ifndef KDIFF_KDIFFINTERFACE_H
#define KDIFF_KDIFFINTERFACE_H

#include <QObject>
#include "kdiffinterface_export.h"

enum Mode {
    Files,
    Diff,
    DiffAndFiles
};

class KDIFFINTERFACE_EXPORT KDiffInterface {
public:
    KDiffInterface();

    virtual ~KDiffInterface();

public:
    /**
     * Open and parse the diff file at url.
     */
    virtual void openDiff(const QUrl &diffUrl) = 0;

    /**
     * Open and parse the supplied diff output
     */
    virtual void openDiff(const QString &diffOutput) = 0;

    /**
     * Compare, with diff, source with destination, can also be used if you do not
     * know what source and destination are. The part will try to figure out what
     * they are (directory, file, diff output file) and call the
     * appropriate method(s)
     */
    virtual void openFiles(const QUrl &source, const QUrl &destination) = 0;

    /**
     * This will show the directory and the directory with the diff applied
     */
    virtual void openDiffAndFiles(const QUrl &diffFile, const QUrl &destination) = 0;

};

Q_DECLARE_INTERFACE(KDiffInterface, "com.kde.KDiff.KDiffInterface/1.0")

#endif //KDIFF_KDIFFINTERFACE_H
