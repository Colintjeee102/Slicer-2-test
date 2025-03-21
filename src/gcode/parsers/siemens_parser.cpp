#include "gcode/parsers/siemens_parser.h"

#include <QString>
#include <QStringList>
#include <QVector>

#include "exceptions/exceptions.h"
#include "units/unit.h"

namespace ORNL
{
    SiemensParser::SiemensParser(GcodeMeta meta, bool allowLayerAlter, QStringList& lines, QStringList& upperLines)
        : CommonParser(meta, allowLayerAlter, lines, upperLines)
    {
        config();
    }

    void SiemensParser::config()
    {
        CommonParser::config();

        addCommandMapping(
            "BEAD_AREA",
            std::bind(
                &SiemensParser::BeadAreaHandler, this, std::placeholders::_1));
        addCommandMapping(
            "WHEN TRUE DO EXTR_END=2.0",
            std::bind(
                &SiemensParser::ExtruderOffHandler, this, std::placeholders::_1));
    }

    void SiemensParser::BeadAreaHandler(QVector<QStringRef> params)
    {
        //redirect - essentially M3 command
        CommonParser::M3Handler(params);
    }

    void SiemensParser::ExtruderOffHandler(QVector<QStringRef> params)
    {
        //redirect - essentially M5 command
        CommonParser::M5Handler(params);
    }
    void SiemensParser::G1Handler(QVector<QStringRef> params)
    {
        CommonParser::G1Handler(params); // Call the original function

        // ✅ Force a space before EM=1
        QString currentComment = m_current_gcode_command.getComment();
        m_current_gcode_command.setComment(currentComment + "1");
    }
}  // namespace ORNL
