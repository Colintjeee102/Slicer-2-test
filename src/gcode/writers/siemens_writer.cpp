#include <QStringBuilder>

#include "gcode/writers/siemens_writer.h"
#include "utilities/enums.h"
#include "utilities/mathutils.h"

namespace ORNL
{
    SiemensWriter::SiemensWriter(GcodeMeta meta, const QSharedPointer<SettingsBase>& sb) : WriterBase(meta, sb) {
        m_M64 = "M64";
        m_M65 = "M65";
    }

    QString SiemensWriter::writeInitialSetup(Distance minimum_x, Distance minimum_y, Distance maximum_x, Distance maximum_y, int num_layers)
    {
        m_current_z = m_sb->setting< Distance >(Constants::PrinterSettings::Dimensions::kZOffset);
        m_current_w = m_sb->setting< Distance >(Constants::PrinterSettings::Dimensions::kWMax);
        m_current_rpm = 0;
        m_extruders_on[0] = false;
        m_first_print = true;
        m_first_travel = true;
        m_layer_start = true;
        m_min_z = 0.0f;
        QString rv;
        if (m_sb->setting< int >(Constants::PrinterSettings::GCode::kEnableStartupCode))
        {
            rv += commentLine("START UP");
            rv += commentLine("G-CODE FOR AM FLEXBOT");
            rv += "G90" % commentSpaceLine("USE ABSOLUTE COORDINATES");
            rv += "G17" % commentSpaceLine("COORDINATE PLANE X/Y");
            rv += "G55" % commentSpaceLine("WORK OFFSET");
            rv += "CYCLE832(2,_finish,1)" % commentSpaceLine("ENABLE HIGH SPEED MACHINING CYCLE");
            rv += "EXTRUDERSPEED2(0,0,1)" % commentSpaceLine("SET THE EXTRUDERSPEED TO 0 RPM");
            Distance layer_width = m_sb->setting<Distance>(Constants::ProfileSettings::Layer::kBeadWidth);
            Distance layer_height = m_sb->setting<Distance>(Constants::ProfileSettings::Layer::kLayerHeight);

            rv += "EXTRUDERSPEED2(" % QString::number(layer_width.to(m_meta.m_distance_unit), 'f', 4) % "," %
                  QString::number(layer_height.to(m_meta.m_distance_unit), 'f', 4) % ",1400)" %
                  commentSpaceLine("SET THE EXTRUDERSPEED") % m_newline;
        }

        if(m_sb->setting< int >(Constants::PrinterSettings::GCode::kEnableBoundingBox))
        {
            rv += "G1 Z2" % commentSpaceLine("RAISE Z TO DEMO BOUNDING BOX")
                % m_G1 % m_x % QString::number(minimum_x.to(m_meta.m_distance_unit), 'f', 4) % " Y" % QString::number(minimum_y.to(m_meta.m_distance_unit), 'f', 4) % commentSpaceLine("BOUNDING BOX")
                % m_G1 % m_x % QString::number(maximum_x.to(m_meta.m_distance_unit), 'f', 4) % " Y" % QString::number(minimum_y.to(m_meta.m_distance_unit), 'f', 4) % commentSpaceLine("BOUNDING BOX")
                % m_G1 % m_x % QString::number(maximum_x.to(m_meta.m_distance_unit), 'f', 4) % " Y" % QString::number(maximum_y.to(m_meta.m_distance_unit), 'f', 4) % commentSpaceLine("BOUNDING BOX")
                % m_G1 % m_x % QString::number(minimum_x.to(m_meta.m_distance_unit), 'f', 4) % " Y" % QString::number(maximum_y.to(m_meta.m_distance_unit), 'f', 4) % commentSpaceLine("BOUNDING BOX")
                % m_G1 % m_x % QString::number(minimum_x.to(m_meta.m_distance_unit), 'f', 4) % " Y" % QString::number(minimum_y.to(m_meta.m_distance_unit), 'f', 4) % commentSpaceLine("BOUNDING BOX")
                % "M0" % commentSpaceLine("WAIT FOR USER");

            m_start_point = Point(minimum_x, minimum_y, 0);
        }

        if(m_sb->setting< QString >(Constants::PrinterSettings::GCode::kStartCode) != "")
            rv += m_sb->setting< QString >(Constants::PrinterSettings::GCode::kStartCode);

        rv += m_newline;

        rv += commentLine("LAYER COUNT: " % QString::number(num_layers));

        return rv;
    }

    QString SiemensWriter::writeBeforeLayer(float new_min_z,QSharedPointer<SettingsBase> sb)
    {
        m_spiral_layer = sb->setting<bool>(Constants::ProfileSettings::SpecialModes::kEnableSpiralize);
        m_layer_start = true;
        QString rv;
        return rv;
    }

    QString SiemensWriter::writeBeforePart(QVector3D normal)
    {
        QString rv;
        return rv;
    }

    QString SiemensWriter::writeBeforeIsland()
    {
        QString rv;
        return rv;
    }

    QString SiemensWriter::writeBeforeRegion(RegionType type, int pathSize)
    {
        QString rv;
        return rv;
    }

    QString SiemensWriter::writeBeforePath(RegionType type)
    {
        QString rv;
        //Commented out on 12/29/20 by Alex because these G-Code lines need issued after travels, not at the start of the region
        /*if(!m_spiral_layer || m_first_print)
        {
            if (type == RegionType::kPerimeter)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterStart) % m_newline;
            }
            else if (type == RegionType::kInset)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInsetStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInsetStart) % m_newline;
            }
            else if(type == RegionType::kSkeleton)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkeletonStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkeletonStart) % m_newline;
            }
            else if (type == RegionType::kSkin)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkinStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkinStart) % m_newline;
            }
            else if (type == RegionType::kInfill)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInfillStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInfillStart) % m_newline;
            }
            else if (type == RegionType::kSupport)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSupportStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSupportStart) % m_newline;
            }
        }*/
        return rv;
    }

    QString SiemensWriter::writeTravel(Point start_location, Point target_location, TravelLiftType lType,
                                       QSharedPointer<SettingsBase> params)
    {
        QString rv;

        //If extruder is left on because of no end of path modifier, turn it off using Perimeter end of path G-Code
        //This syntax doesn't currently issue extruder on/off commands because they are manually input to the
        //start/end G-Code of the settings
        if(m_extruders_on[0])
        {
            m_extruders_on[0] = false;
            if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterEnd).isEmpty())
                rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterEnd) % m_newline;
        }

        Point new_start_location;

        //Use updated start location if this is the first travel
        if(m_first_travel)
            new_start_location = m_start_point;
        else
            new_start_location = start_location;

        Distance liftDist;
        liftDist = m_sb->setting< Distance >(Constants::ProfileSettings::Travel::kLiftHeight);

        bool travel_lift_required = liftDist > 0;// && !m_first_travel; //do not write a lift on first travel

        //Don't lift for short travel moves
        if(start_location.distance(target_location) < m_sb->setting< Distance >(Constants::ProfileSettings::Travel::kMinTravelForLift))
        {
            travel_lift_required = false;
        }

        //travel_lift vector in direction normal to the layer
        //with length = lift height as defined in settings
        QVector3D travel_lift = getTravelLift();

        //write the lift
        if (travel_lift_required && (lType == TravelLiftType::kBoth || lType == TravelLiftType::kLiftUpOnly))
        {
            Point lift_destination = new_start_location + travel_lift; //lift destination is above start location
            rv += m_G1 % writeCoordinates(lift_destination) % " EM=0" % commentSpaceLine("TRAVEL LIFT Z");
            setFeedrate(m_sb->setting< Velocity >(Constants::PrinterSettings::MachineSpeed::kZSpeed));
        }

        //write the travel
        Point travel_destination = target_location;
        if (travel_lift_required)
        {
            travel_destination = travel_destination + travel_lift; //travel destination is above the target point
        }

        rv += m_G1 % writeCoordinates(travel_destination) % " EM=0" % commentSpaceLine("TRAVEL");
        setFeedrate(m_sb->setting< Velocity >(Constants::ProfileSettings::Travel::kSpeed));

        //write the travel lower (undo the lift)
        if (travel_lift_required && (lType == TravelLiftType::kBoth || lType == TravelLiftType::kLiftLowerOnly))
        {
            rv += m_G1 + writeCoordinates(target_location) % " EM=0" % commentSpaceLine("TRAVEL LOWER Z");
            setFeedrate(m_sb->setting<Velocity>(Constants::PrinterSettings::MachineSpeed::kZSpeed));
        }

        if (m_first_travel) //if this is the first travel
            m_first_travel = false; //update for next one

        return rv;
    }

    QString SiemensWriter::writeLine(const Point& start_point, const Point& target_point, const QSharedPointer<SettingsBase> params)
    {
        Velocity speed = params->setting<Velocity>(Constants::SegmentSettings::kSpeed);
        RegionType region_type = params->setting<RegionType>(Constants::SegmentSettings::kRegionType);
        PathModifiers path_modifiers = params->setting<PathModifiers>(Constants::SegmentSettings::kPathModifiers);

        QString rv;

        //This syntax doesn't currently issue extruder on/off commands because they are manually input to the
        //start/end G-Code of the setting
        //Write out the region starting G-Code if this is the first segment of the path
        //First segment of the path is signified by extruder being off and the modifier isn't one of five ending modifiers
        if(m_extruders_on[0] == false && path_modifiers != PathModifiers::kSlowDown && path_modifiers != PathModifiers::kForwardTipWipe
            && path_modifiers != PathModifiers::kReverseTipWipe && path_modifiers != PathModifiers::kCoasting
            && path_modifiers != PathModifiers::kSpiralLift)
        {
            m_extruders_on[0] = true;
            if (region_type == RegionType::kPerimeter)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterStart) % m_newline;
            }
            else if (region_type == RegionType::kInset)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInsetStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInsetStart) % m_newline;
            }
            else if(region_type == RegionType::kSkeleton)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkeletonStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkeletonStart) % m_newline;
            }
            else if (region_type == RegionType::kSkin)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkinStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkinStart) % m_newline;
            }
            else if (region_type == RegionType::kInfill)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInfillStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInfillStart) % m_newline;
            }
            else if (region_type == RegionType::kSupport)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSupportStart).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSupportStart) % m_newline;
            }
        }
        //Write out the region ending G-Code if this is the first segment of the ending path modifiers
        //First segment is signified by extruder being on and the modifier is one of five ending modifiers
        else if(m_extruders_on[0] == true && (path_modifiers == PathModifiers::kSlowDown || path_modifiers == PathModifiers::kForwardTipWipe
                                               || path_modifiers == PathModifiers::kReverseTipWipe || path_modifiers == PathModifiers::kCoasting
                                               || path_modifiers == PathModifiers::kSpiralLift))
        {
            m_extruders_on[0] = false;
            if (region_type == RegionType::kPerimeter)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterEnd) % m_newline;
            }
            else if (region_type == RegionType::kInset)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInsetEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInsetEnd) % m_newline;
            }
            else if (region_type == RegionType::kSkeleton)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkeletonEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkeletonEnd) % m_newline;
            }
            else if (region_type == RegionType::kSkin)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkinEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkinEnd) % m_newline;
            }
            else if (region_type == RegionType::kInfill)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInfillEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInfillEnd) % m_newline;
            }
            else if (region_type == RegionType::kSupport)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSupportEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSupportEnd) % m_newline;
            }
        }

        rv += m_G1;
        //update feedrate if needed
        if (getFeedrate() != speed || m_layer_start)
        {
            setFeedrate(speed);
            rv += m_f % QString::number(speed.to(m_meta.m_velocity_unit));
            m_layer_start = false;
        }

        //writes WXYZ to destination
        if (path_modifiers == PathModifiers::kForwardTipWipe || path_modifiers == PathModifiers::kReverseTipWipe)
        {
            rv += writeCoordinates(target_point) % " EM=0";
        }
        else
        {
            rv += writeCoordinates(target_point) % " EM=1";
        }

        //add comment for gcode parser
        if (path_modifiers != PathModifiers::kNone)
            rv += commentSpaceLine(toString(region_type) % m_space % toString(path_modifiers));
        else
            rv += commentSpaceLine(toString(region_type));

        m_first_print = false;

        return rv;
    }

    QString SiemensWriter::writeArc(const Point &start_point,
                                           const Point &end_point,
                                           const Point &center_point,
                                           const Angle &angle,
                                           const bool &ccw,
                                    const QSharedPointer<SettingsBase> params)
    {
        QString rv;

        Velocity speed = params->setting<Velocity>(Constants::SegmentSettings::kSpeed);
        int rpm = params->setting<int>(Constants::SegmentSettings::kExtruderSpeed);
        int material_number = params->setting<int>(Constants::SegmentSettings::kMaterialNumber);
        auto region_type = params->setting<RegionType>(Constants::SegmentSettings::kRegionType);
        auto path_modifiers = params->setting<PathModifiers>(Constants::SegmentSettings::kPathModifiers);

        // Turn on the extruder if it isn't already on
        if (!m_extruders_on[0] && rpm > 0)
        {
            rv += writeExtruderOn(region_type, rpm);
        }

        rv += ((ccw) ? m_G3 : m_G2);

        if (getFeedrate() != speed)
        {
            setFeedrate(speed);
            rv += m_f % QString::number(speed.to(m_meta.m_velocity_unit));
        }
        if (rpm != m_current_rpm)
        {
            rv += m_s % QString::number(rpm);
            m_current_rpm = rpm;
        }

        rv += m_i % QString::number(Distance(center_point.x() - start_point.x()).to(m_meta.m_distance_unit), 'f', 4) %
              m_j % QString::number(Distance(center_point.y() - start_point.y()).to(m_meta.m_distance_unit), 'f', 4) %
              m_x % QString::number(Distance(end_point.x()).to(m_meta.m_distance_unit), 'f', 4) %
              m_y % QString::number(Distance(end_point.y()).to(m_meta.m_distance_unit), 'f', 4) % " EM=1";
        // write vertical coordinate along the correct axis (Z or W) according to printer settings
        // only output Z/W coordinate if there was a change in Z/W
        Distance z_offset = m_sb->setting< Distance >(Constants::PrinterSettings::Dimensions::kZOffset);

        Distance target_z = end_point.z() + z_offset;
        if(qAbs(target_z - m_last_z) > 10)
        {
            rv += m_z % QString::number(Distance(target_z).to(m_meta.m_distance_unit), 'f', 4);
            m_current_z = target_z;
            m_last_z = target_z;
        }

        // Add comment for gcode parser
        if (path_modifiers != PathModifiers::kNone)
            rv += commentSpaceLine(toString(region_type) % m_space % toString(path_modifiers));
        else
            rv += commentSpaceLine(toString(region_type));

        return rv;
    }

    QString SiemensWriter::writeAfterPath(RegionType type)
    {
        QString rv;
        //Commented out on 12/29/20 by Alex because these G-Code lines need issued immediately after printing paths, not after the ending modifiers
        /*if(!m_spiral_layer)
        {
            if (type == RegionType::kPerimeter)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterEnd) % m_newline;
            }
            else if (type == RegionType::kInset)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInsetEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInsetEnd) % m_newline;
            }
            else if (type == RegionType::kSkeleton)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkeletonEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkeletonEnd) % m_newline;
            }
            else if (type == RegionType::kSkin)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkinEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSkinEnd) % m_newline;
            }
            else if (type == RegionType::kInfill)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInfillEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kInfillEnd) % m_newline;
            }
            else if (type == RegionType::kSupport)
            {
                if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSupportEnd).isEmpty())
                    rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kSupportEnd) % m_newline;
            }
        }*/
        return rv;
    }

    QString SiemensWriter::writeAfterRegion(RegionType type)
    {
        QString rv;
        return rv;
    }

    QString SiemensWriter::writeAfterIsland()
    {
        QString rv;
        return rv;
    }

    QString SiemensWriter::writeAfterPart()
    {
        QString rv;
        return rv;
    }

    QString SiemensWriter::writeAfterLayer()
    {
        QString rv;
        rv += m_sb->setting< QString >(Constants::PrinterSettings::GCode::kLayerCodeChange) % m_newline;
        return rv;
    }

    QString SiemensWriter::writeShutdown()
    {
        QString rv;

        // Voeg een comment toe voor de shutdown
        rv += comment("END PROGRAM") % m_newline;

        rv += m_sb->setting< QString >(Constants::PrinterSettings::GCode::kEndCode) % m_newline;

        rv += "EXTRUDERSPEED2(0,0,1)" % commentSpaceLine("EXTRUDERSPEED 0 RPM");

        Distance current_height = m_current_z;
        Distance new_height = current_height + 50000;

        rv += "G1 Z" % QString::number(new_height.to(m_meta.m_distance_unit), 'f', 4) % " F10000" %
              commentSpaceLine("MOVE UP 50MM");

        rv += "H[10]=1" % commentSpaceLine("EXTRUDER OFF");

        rv += "M30" % commentSpaceLine("END OF G-CODE");

        return rv;
    }

    QString SiemensWriter::writePurge(int RPM, int duration, int delay)
    {
        return {};
    }

    QString SiemensWriter::writeDwell(Time time)
    {
        if (time > 0)
            return m_G4 % m_p % QString::number(time.to(m_meta.m_time_unit), 'f', 4) % commentSpaceLine("DWELL");
        else
            return {};
    }

    QString SiemensWriter::writeTamperOn()
    {
        return {};
    }

    QString SiemensWriter::writeTamperOff()
    {
        return {};
    }

    QString SiemensWriter::writeExtruderOn(RegionType type, int rpm)
    {
        QString rv;
        return rv;
    }

    QString SiemensWriter::writeExtruderOff()
    {
        QString rv;
        return rv;
    }

    QString SiemensWriter::writeCoordinates(Point destination)
    {
        QString rv;

        //always specify X and Y
        rv += m_x % QString::number(Distance(destination.x()).to(m_meta.m_distance_unit), 'f', 4) %
              m_y % QString::number(Distance(destination.y()).to(m_meta.m_distance_unit), 'f', 4);

        //write vertical coordinate along the correct axis (Z or W) according to printer settings
        //only output Z/W coordinate if there was a change in Z/W
        Distance z_offset = m_sb->setting< Distance >(Constants::PrinterSettings::Dimensions::kZOffset);
        //move in Z only
        Distance target_z = destination.z() + z_offset;
        if(qAbs(target_z - m_last_z) > 10)
        {
            rv += m_z % QString::number(Distance(target_z).to(m_meta.m_distance_unit), 'f', 4);
            m_current_z = target_z;
            m_last_z = target_z;
        }
        return rv;
    }

}  // namespace Colin Otten
