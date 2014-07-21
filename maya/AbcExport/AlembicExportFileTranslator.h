#ifndef ABCEXPORT_ALEMBIC_EXPORT_FILE_TRANSLATOR_H_
#define ABCEXPORT_ALEMBIC_EXPORT_FILE_TRANSLATOR_H_

#include <maya/MPxFileTranslator.h>

class AlembicExportFileTranslator : public MPxFileTranslator
{
public:

    AlembicExportFileTranslator() : MPxFileTranslator() {}

    virtual ~AlembicExportFileTranslator() {}

    MStatus writer (const MFileObject& file,
                    const MString& optionsString,
                    MPxFileTranslator::FileAccessMode mode);

    MFileKind identifyFile(const MFileObject& filename,
                           const char* buffer,
                           short size) const;

    bool haveWriteMethod() const
    {
        return true;
    }

    MString defaultExtension() const
    {
        return "abc";
    }

    MString filter() const
    {
        return "*.abc";
    }

    static void* creator()
    {
        return new AlembicExportFileTranslator;
    }
};

#endif  // ABCEXPORT_ALEMBIC_EXPORT_FILE_TRANSLATOR_H_
