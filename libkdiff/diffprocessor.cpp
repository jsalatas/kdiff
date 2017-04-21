//
// Created by john on 4/2/17.
//

#include <klocalizedstring.h>
#include "diffprocessor.h"
#include "parser.h"

DiffProcessor::DiffProcessor(QObject *parent)
    : QObject(parent)
    , m_tmpDiffModelMap(nullptr)
{}
DiffProcessor::~DiffProcessor()
{}

int DiffProcessor::parseDiffOutput(const QString& diff)
{
    QStringList diffLines = split(diff);

    Parser* parser = new Parser();
    bool malformed = false;
    models(parser->parse(diffLines, &malformed));

    comparisonInfo()->generator = parser->generator();
    comparisonInfo()->format = parser->format();

    delete parser;

    if(models()) {
        if(malformed) {
            emit message(i18n("The diff is malformed. Some lines could not be parsed and will not be displayed in the diff view."), Warning, false);
            // proceed anyway with the lines which have been parsed
        }
    } else {
        return -1;
    }

    return 0;
}

QStringList DiffProcessor::split(const QString& fileContents)
{
    QString contents = fileContents;
    QStringList list;

    int pos = 0;
    int oldpos = 0;
    // split that does not strip the split char
#ifdef QT_OS_MAC
    const char split = '\r';
#else
    const char split = '\n';
#endif
    while ((pos = contents.indexOf(split, oldpos)) >= 0) {
        list.append(contents.mid(oldpos, pos - oldpos + 1));
        oldpos = pos + 1;
    }

    if(contents.length() > oldpos) {
        list.append(contents.right(contents.length() - oldpos));
    }

    return list;
}

bool DiffProcessor::blendOriginalIntoModelList(const QString&)
{
    // Do nothing
    return true;
}
const DiffModelMap* DiffProcessor::models() const
{
    return m_tmpDiffModelMap;
}
void DiffProcessor::models(DiffModelMap* models)
{
    m_tmpDiffModelMap = models;
}

