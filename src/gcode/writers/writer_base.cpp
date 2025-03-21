#include <QStringBuilder>

#include "gcode/writers/writer_base.h"
#include "managers/settings/settings_manager.h"
#include "utilities/mathutils.h"


namespace ORNL
{
    WriterBase::WriterBase(GcodeMeta meta, const QSharedPointer<SettingsBase>& sb) : m_sb(sb)
    {
       m_meta = meta;
       m_newline = '\n';
       m_empty_step_comment = "INTENTIONALLY BLANK - NO PATHING PRODUCED USING CURRENT SETTINGS";
       m_space = ' ';
       m_x = " X";
       m_y = " Y";
       m_z = " Z";
       m_w = " W";
       m_e = " E";
       m_f = " F";
       m_s = " S";
       m_p = " P";
       m_i = " I";
       m_j = " J";
       m_k = " K";
       m_r = " R";
       m_l = " L";
       m_q = " Q";
       m_a = " A";
       m_b = " B";
       m_G0 = "G0";
       m_G1 = "G1";
       m_G2 = "G2";
       m_G3 = "G3";
       m_G4 = "G4";
       m_G5 = "G5";
       m_M3 = "M3";
       m_M5 = "M5";
       m_start_point = Point(0, 0, 0);

       //allocate array to keep track of extruder(s) status
       int num_extruders = m_sb->setting<int>(Constants::ExperimentalSettings::MultiNozzle::kNozzleCount);
       m_extruders_on.resize(num_extruders); //sets vector size
    }

    QString WriterBase::comment(const QString& text)
    {
        return QString(m_meta.m_comment_starting_delimiter % text % m_meta.m_comment_ending_delimiter);
    }

    QString WriterBase::commentLine(const QString& text)
    {
        return QString(m_meta.m_comment_starting_delimiter % text % m_meta.m_comment_ending_delimiter % m_newline);
    }

    QString WriterBase::commentSpaceLine(const QString& text)
    {
        return QString(m_space % m_meta.m_comment_starting_delimiter % text % m_meta.m_comment_ending_delimiter % m_newline);
    }

    QVector3D WriterBase::getTravelLift()
    {
        /*
            If the slicing plane rotates (ie, is different for every layer), normal direction can't
            be fetched from global settings
        */

        //get slicing angle info for direction
        Angle slicing_plane_pitch = m_sb->setting<Angle>(Constants::ExperimentalSettings::SlicingAngle::kStackingDirectionPitch);
        Angle slicing_plane_yaw   = m_sb->setting<Angle>(Constants::ExperimentalSettings::SlicingAngle::kStackingDirectionYaw);
        Angle slicing_plane_roll  = m_sb->setting<Angle>(Constants::ExperimentalSettings::SlicingAngle::kStackingDirectionRoll);
        QQuaternion quaternion = MathUtils::CreateQuaternion(slicing_plane_pitch, slicing_plane_yaw, slicing_plane_roll);
        QVector3D normal_vector = quaternion.rotatedVector(QVector3D(0, 0, 1));

        //normalize and multiply by lift height
        normal_vector.normalize();
        Distance lift_distance = m_sb->setting< Distance >(Constants::ProfileSettings::Travel::kLiftHeight).to(micron);
        normal_vector *= lift_distance();

        return normal_vector;
    }

    QString WriterBase::writeSlicerHeader(const QString& syntax)
    {
        QString rv;
        QFile versions(":/configs/versions.conf");
        versions.open(QIODevice::ReadOnly);
        QString version_string = versions.readAll();
        fifojson version_data = json::parse(version_string.toStdString());
        double version = version_data["master_version"];

        if(syntax == Constants::PrinterSettings::SyntaxString::kIngersoll)
            rv += commentLine("---BEGIN HEADER");
        else if (syntax == Constants::PrinterSettings::SyntaxString::kMeltio)
            rv += "%\n";

        rv += commentLine(QString("Nedcam shaping technology")) %
               commentLine(QString("Copyright " % QString::number(QDate::currentDate().year()) % "")) %
               commentLine(QString("G-Code Syntax: ") % syntax) %
               m_newline;

        return rv;
    }

    QString WriterBase::writeSettingsHeader(GcodeSyntax syntax)
    {
        QString text = "";

        // Quick view of slicing parameters written to file header
        text += commentLine("Slicing Parameters");

        // Each machine prints a different set of comments depending on its design

        // TODO: take all not-empty switch cases (except for kBeam) and move them to the coresponding
        // writer's writeSettingsHeader, overriding that function in the header of that writer.
        // Once done, delete entire switch statement, except for the code isside the kBeam case.
        //

        if (syntax != GcodeSyntax::kCincinnati && syntax != GcodeSyntax::kHaasInch && syntax != GcodeSyntax::kMeld &&
            syntax != GcodeSyntax::kORNL && syntax != GcodeSyntax::kSheetLamination && syntax != GcodeSyntax::kSiemens &&
            syntax != GcodeSyntax::kSkyBaam)
        {
            text += commentLine(
                QString("Nozzle Diameter: %0mm")
                    .arg(m_sb->setting< Distance >(Constants::ProfileSettings::Layer::kNozzleDiameter).to(mm)));
        }
        else
        {
            text += commentLine(
                QString("Nozzle Diameter: %0mm")
                    .arg(m_sb->setting< Distance >(Constants::ProfileSettings::Layer::kNozzleDiameter).to(mm)));
        }
        if (m_sb->setting<int>(Constants::PrinterSettings::MachineSetup::kMachineType) == 1) // If filament machine type
        {
            text += commentLine(
                QString("Filament Diameter: %0mm")
                .arg(m_sb->setting< Distance >(Constants::MaterialSettings::Filament::kDiameter).to(mm)));
        }
        if (syntax != GcodeSyntax::kCincinnati && syntax != GcodeSyntax::kHaasInch && syntax != GcodeSyntax::kMeld &&
            syntax != GcodeSyntax::kORNL && syntax != GcodeSyntax::kSheetLamination && syntax != GcodeSyntax::kSiemens &&
            syntax != GcodeSyntax::kSkyBaam)
        {
            text += commentLine(
                QString("Printer Base Offset: %0mm")
                    .arg(m_sb->setting< Distance >(Constants::PrinterSettings::Dimensions::kZOffset).to(mm)));
        }
        else
        {
            text += commentLine(
                QString("Printer Base Offset: %0mm")
                    .arg(m_sb->setting< Distance >(Constants::PrinterSettings::Dimensions::kZOffset).to(mm)));
        }
        if(m_sb->setting< int >(Constants::PrinterSettings::Dimensions::kEnableW))
        {
            text += commentLine(
                QString("Minimum Table Value: %0mm")
                    .arg(m_sb->setting< Distance >(Constants::PrinterSettings::Dimensions::kWMin).to(mm)));
        }
        if (syntax != GcodeSyntax::kCincinnati && syntax != GcodeSyntax::kHaasInch && syntax != GcodeSyntax::kMeld &&
            syntax != GcodeSyntax::kORNL && syntax != GcodeSyntax::kSheetLamination && syntax != GcodeSyntax::kSiemens &&
            syntax != GcodeSyntax::kSkyBaam)
        {
            text += commentLine(
                QString("Layer Height: %0mm")
                    .arg(m_sb->setting< Distance >(Constants::ProfileSettings::Layer::kLayerHeight).to(mm)));
            text += commentLine(
                QString("Default Extrusion Width: %0mm")
                    .arg(m_sb->setting< Distance >(Constants::ProfileSettings::Layer::kBeadWidth).to(mm)));
        }
        else
        {
            text += commentLine(
                QString("Layer Height: %0mm")
                    .arg(m_sb->setting< Distance >(Constants::ProfileSettings::Layer::kLayerHeight).to(mm)));
            text += commentLine(
                QString("Default Extrusion Width: %0mm")
                    .arg(m_sb->setting< Distance >(Constants::ProfileSettings::Layer::kBeadWidth).to(mm)));
        }

        if(m_sb->setting< int >(Constants::ProfileSettings::SpecialModes::kEnableSpiralize))
        {
            text += commentLine(QString("Spiralize is turned ON"));
            if (m_sb->setting< int >(Constants::ProfileSettings::SpecialModes::kSmoothing))
                text += commentLine("Smoothing is turned ON");
            if (m_sb->setting< int >(Constants::ProfileSettings::SpecialModes::kEnableOversize))
            {
                text += commentLine(
                    QString("Oversize part by: %0mm")
                        .arg(m_sb->setting< Distance >(Constants::ProfileSettings::SpecialModes::kOversizeDistance).to(mm)));
            }
            text += m_newline;
            return text;
        }
        else {
            if(m_sb->setting< int >(Constants::ProfileSettings::Perimeter::kEnable))
                text += commentLine(QString("Perimeter Count: %0")
                            .arg(m_sb->setting< int >(Constants::ProfileSettings::Perimeter::kCount)));
            if(m_sb->setting< int >(Constants::ProfileSettings::Inset::kEnable))
                text += commentLine(QString("Inset Count: %0")
                            .arg(m_sb->setting< int >(Constants::ProfileSettings::Inset::kCount)));
            if(m_sb->setting< int >(Constants::ProfileSettings::Skin::kEnable))
                text += commentLine(QString("Upskin Count: %0")
                            .arg(m_sb->setting< int >(Constants::ProfileSettings::Skin::kTopCount)));
            if(m_sb->setting< int >(Constants::ProfileSettings::Skin::kEnable))
                text += commentLine(QString("Downskin Count: %0")
                            .arg(m_sb->setting< int >(Constants::ProfileSettings::Skin::kBottomCount)));
            if (m_sb->setting< int >(Constants::ProfileSettings::Skin::kEnable) && (m_sb->setting< int >(Constants::ProfileSettings::Skin::kTopCount) >0 || m_sb->setting< int >(Constants::ProfileSettings::Skin::kBottomCount) > 0))
            {
                int skinPattern = m_sb->setting< int >(Constants::ProfileSettings::Skin::kPattern);
                if (skinPattern)
                    text += commentLine(QString("Skin Pattern: Lines"));
                else
                    text += commentLine(QString("Skin Patern: Concentric"));
            }
            if (m_sb->setting< int >(Constants::ProfileSettings::Infill::kEnable))
            {
                if(m_sb->setting< int >(Constants::ProfileSettings::Infill::kManualLineSpacing))
                {
                    text += commentLine(
                        QString("Infill Percentage: %0%")
                            .arg(m_sb->setting< Distance >(Constants::ProfileSettings::Layer::kBeadWidth).to(mm) /
                                 m_sb->setting< Distance >(Constants::ProfileSettings::Infill::kLineSpacing).to(mm) * 100));
                }
                else
                {
                    text += commentLine(
                        QString("Infill Percentage: %0%")
                            .arg(m_sb->setting< double >(Constants::ProfileSettings::Infill::kDensity)));
                }


                int infill_pattern = m_sb->setting< int >(Constants::ProfileSettings::Infill::kPattern);
                switch (infill_pattern)
                {
                    case 0:
                        text += commentLine("Infill Pattern: Lines");
                        break;
                    case 1:
                        text += commentLine("Infill Pattern: Grid");
                        break;
                    case 2:
                        text += commentLine("Infill Pattern: Concentric");
                        break;
                    case 3:
                        text += commentLine("Infill Pattern: Inside Out Concentric");
                        break;
                    case 4:
                        text += commentLine("Infill Pattern: Triangles");
                        break;
                    case 5:
                        text += commentLine("Infill Pattern: Hexagons and Triangles");
                        break;
                    case 6:
                        text += commentLine("Infill Pattern: Honeycomb");
                        break;
                    case 7:
                        text += commentLine("Infill Pattern: Radial Hatch");
                        break;
                    default:
                        text += commentLine("Infill Pattern: Lines");
                        break;
                }
            }
            // TODO: This doesn't output the layer time, need to change the setting <int> to Time and convert the setting value to a time... somehow
            if (m_sb->setting< int >(Constants::MaterialSettings::Cooling::kForceMinLayerTime))
                text += commentLine(
                    QString("Forced Minimum / Maximum Layer Time: %0 %1 seconds")
                        .arg(m_sb->setting<Time>(Constants::MaterialSettings::Cooling::kMinLayerTime)())
                        .arg(m_sb->setting<Time>(Constants::MaterialSettings::Cooling::kMaxLayerTime)()));
            if (SettingsManager::getInstance()->getGlobal()->setting< bool >("useSmoothing"))
                text += commentLine("Smoothing is turned ON");
            if(m_sb->setting< Angle >(Constants::ExperimentalSettings::SlicingAngle::kStackingDirectionYaw) != 0 ||
                m_sb->setting< Angle >(Constants::ExperimentalSettings::SlicingAngle::kStackingDirectionPitch) != 0 ||
                m_sb->setting< Angle >(Constants::ExperimentalSettings::SlicingAngle::kStackingDirectionRoll) != 0)
            {
                text += commentLine("ANGLED SLICING ENABLED");
            }
            if (m_sb->setting< int >(Constants::ProfileSettings::SpecialModes::kEnableOversize))
            {
                text += commentLine(
                    QString("Oversize part by: %0mm")
                        .arg(m_sb->setting< Distance >(Constants::ProfileSettings::SpecialModes::kOversizeDistance).to(mm)));
            }
        }
        if(syntax == GcodeSyntax::kIngersoll)
            text += commentLine("---END HEADER");

        text += m_newline;
        return text;
    }

    QString WriterBase::writeLayerChange(uint layer_number)
    {
        return commentLine(QString("BEGINNING LAYER: ") % QString::number(layer_number + 1));
    }

    QString WriterBase::writeSettingsFooter()
    {
        QString rv;
        rv += m_newline % commentLine("Settings Footer");
        for(auto& el : m_sb->json().items())
        {
            rv += commentLine(QString::fromStdString(el.key()) % m_space % QString::fromStdString(el.value().dump()));
        }
        // Remove empty line from end of file that comes from the commentLine creating a new line
        rv.chop(1);
        return rv;
    }

    QString WriterBase::writeEmptyStep()
    {
        return commentLine(m_empty_step_comment);
    }

    QString WriterBase::writeCommentLine(QString comment)
    {
        return "\n" + commentLine(comment);
    }

    void WriterBase::setFeedrate(Velocity feedrate)
    {
        m_feedrate = feedrate;
    }

    Velocity WriterBase::getFeedrate() const
    {
        return m_feedrate;
    }

}  // namespace ORNL
