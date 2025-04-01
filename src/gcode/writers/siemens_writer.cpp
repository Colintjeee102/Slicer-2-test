#include <QStringBuilder>

#include "gcode/writers/siemens_writer.h"
#include "utilities/enums.h"
#include "utilities/mathutils.h"

namespace ORNL
{
    SiemensWriter::SiemensWriter(GcodeMeta meta, const QSharedPointer<SettingsBase>& sb) : WriterBase(meta, sb) {
        m_M64 = "M64";
        m_M65 = "M65";
        m_first_layer_written = false;
    }

    QString SiemensWriter::writeInitialSetup(Distance minimum_x, Distance minimum_y, Distance maximum_x, Distance maximum_y, int num_layers)
    {
        m_current_z = m_sb->setting<Distance>(Constants::PrinterSettings::Dimensions::kZOffset);
        m_current_w = m_sb->setting<Distance>(Constants::PrinterSettings::Dimensions::kWMax);
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
            rv += "EXTRUDERSPEED(0,0,1)" % commentSpaceLine("SET THE EXTRUDERSPEED TO 0 RPM");
            Distance current_height = m_current_z;
            Distance new_height = current_height + 200000;

            rv += "G1 Z" % QString::number(new_height.to(m_meta.m_distance_unit), 'f', 4) % " F3000" %
                  commentSpaceLine("MOVE UP 200MM");
            Distance layer_width = m_sb->setting<Distance>(Constants::ProfileSettings::Layer::kBeadWidth);
            Distance layer_height = m_sb->setting<Distance>(Constants::ProfileSettings::Layer::kLayerHeight);

            rv += "EXTRUDERSPEED2(" +
                  QString::number(layer_width.to(m_meta.m_distance_unit), 'f', 4) + "," +
                  QString::number(layer_height.to(m_meta.m_distance_unit), 'f', 4) + ",1400)" +
                  commentSpaceLine("SET THE EXTRUDERSPEED");
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

        rv += commentLine(" ")% commentLine("LAYER COUNT: " % QString::number(num_layers));

        return rv;
    }

    QString SiemensWriter::writeBeforeLayer(float new_min_z, QSharedPointer<SettingsBase> sb)
    {
        m_spiral_layer = sb->setting<bool>(Constants::ProfileSettings::SpecialModes::kEnableSpiralize);
        m_layer_start = true;
        QString rv;

        // If this is the first layer and we haven't written the first travel yet,
        // we'll handle it in the first writeTravel call

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
        return rv;
    }

    QString SiemensWriter::writeTravel(Point start_location, Point target_location, TravelLiftType lType,
                                       QSharedPointer<SettingsBase> params)
    {
        QString rv;

        // If extruder is left on because of no end of path modifier, turn it off
        if(m_extruders_on[0])
        {
            m_extruders_on[0] = false;
            if (!m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterEnd).isEmpty())
                rv += m_sb->setting< QString >(Constants::ProfileSettings::GCode::kPerimeterEnd) % m_newline;
        }

        // Special handling for first travel
        if (m_first_travel) {
            Point first_print_location = target_location;

            // Calculate a safe approach point
            Point offset_start = first_print_location + Point(0, -150000, 0); // 150mm to the left of first print point

            // Move to the approach point first at a safe speed
            rv += m_G1 % writeCoordinates(offset_start);

            // Apply the correct feedrate for the approach move
            Velocity approachSpeed = m_sb->setting<Velocity>(Constants::ProfileSettings::Travel::kSpeed);
            rv += m_f % QString::number(approachSpeed.to(m_meta.m_velocity_unit));

            rv += " EM=0" % commentSpaceLine("APPROACH TO PRINT START");

            // Then move to the actual print start point
            rv += m_G1 % writeCoordinates(first_print_location);
            rv += m_f % QString::number(approachSpeed.to(m_meta.m_velocity_unit));
            rv += " EM=1" % commentSpaceLine("MOVE TO PRINT START");

            m_first_travel = false;
            return rv;
        }

        // Normal travel handling for subsequent moves
        Point new_start_location = start_location;

        // Handle travel lift if needed
        Distance liftDist = m_sb->setting<Distance>(Constants::ProfileSettings::Travel::kLiftHeight);
        bool travel_lift_required = liftDist > 0;

        // Don't lift for short travel moves
        if(start_location.distance(target_location) < m_sb->setting<Distance>(Constants::ProfileSettings::Travel::kMinTravelForLift))
        {
            travel_lift_required = false;
        }

        // Get travel lift vector
        QVector3D travel_lift = getTravelLift();

        // Write the lift if needed
        if (travel_lift_required && (lType == TravelLiftType::kBoth || lType == TravelLiftType::kLiftUpOnly))
        {
            Point lift_destination = new_start_location + travel_lift;
            rv += m_G1 % writeCoordinates(lift_destination);

            // Apply the correct feedrate for Z movement
            Velocity zSpeed = m_sb->setting<Velocity>(Constants::PrinterSettings::MachineSpeed::kZSpeed);
            rv += m_f % QString::number(zSpeed.to(m_meta.m_velocity_unit));

            rv += " EM=0" % commentSpaceLine("TRAVEL LIFT Z");
            setFeedrate(zSpeed);
        }

        // Write the travel move
        Point travel_destination = target_location;
        if (travel_lift_required)
        {
            travel_destination = travel_destination + travel_lift;
        }

        rv += m_G1 % writeCoordinates(travel_destination);

        // Apply the correct feedrate for XY travel
        Velocity travelSpeed = m_sb->setting<Velocity>(Constants::ProfileSettings::Travel::kSpeed);
        rv += m_f % QString::number(travelSpeed.to(m_meta.m_velocity_unit));

        rv += " EM=0" % commentSpaceLine("TRAVEL");
        setFeedrate(travelSpeed);

        // Write the travel lower if needed
        if (travel_lift_required && (lType == TravelLiftType::kBoth || lType == TravelLiftType::kLiftLowerOnly))
        {
            rv += m_G1 + writeCoordinates(target_location);

            // Apply the correct feedrate for Z movement
            Velocity zSpeed = m_sb->setting<Velocity>(Constants::PrinterSettings::MachineSpeed::kZSpeed);
            rv += m_f % QString::number(zSpeed.to(m_meta.m_velocity_unit));

            rv += " EM=0" % commentSpaceLine("TRAVEL LOWER Z");
            setFeedrate(zSpeed);
        }

        return rv;
    }

    QString SiemensWriter::writeLine(const Point& start_point, const Point& target_point, const QSharedPointer<SettingsBase> params)
    {
        // Get all settings at once
        const auto speed = params->setting<Velocity>(Constants::SegmentSettings::kSpeed);
        const auto region_type = params->setting<RegionType>(Constants::SegmentSettings::kRegionType);
        const auto path_modifiers = params->setting<PathModifiers>(Constants::SegmentSettings::kPathModifiers);

        // Build the G1 command string with StringBuilder pattern
        QString rv = m_G1;

        // Only add feedrate if it changed or it's layer start
        if (getFeedrate() != speed || m_layer_start)
        {
            setFeedrate(speed);
            rv.reserve(rv.length() + 20); // Reserve approximate space needed
            rv += m_f % QString::number(speed.to(m_meta.m_velocity_unit));
            m_layer_start = false;
        }

        // Add coordinates with EM parameter
        rv += writeCoordinates(target_point);
        rv += (path_modifiers == PathModifiers::kForwardTipWipe ||
               path_modifiers == PathModifiers::kReverseTipWipe) ? " EM=0" : " EM=1";

        // Add comment efficiently
        if (path_modifiers != PathModifiers::kNone)
        {
            rv += commentSpaceLine(toString(region_type) + m_space + toString(path_modifiers));
        }
        else
        {
            rv += commentSpaceLine(toString(region_type));
        }

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
        // Remove RPM-related code
        // int rpm = params->setting<int>(Constants::SegmentSettings::kExtruderSpeed);
        int material_number = params->setting<int>(Constants::SegmentSettings::kMaterialNumber);
        auto region_type = params->setting<RegionType>(Constants::SegmentSettings::kRegionType);
        auto path_modifiers = params->setting<PathModifiers>(Constants::SegmentSettings::kPathModifiers);

        // Modify extruder on check to not depend on RPM
        if (!m_extruders_on[0])
        {
            rv += writeExtruderOn(region_type, 0); // Pass 0 as RPM or modify writeExtruderOn to not use RPM
        }

        rv += ((ccw) ? m_G3 : m_G2);

        if (getFeedrate() != speed)
        {
            setFeedrate(speed);
            rv += m_f % QString::number(speed.to(m_meta.m_velocity_unit));
        }

        // Remove RPM setting code
        // if (rpm != m_current_rpm)
        // {
        //     rv += m_s % QString::number(rpm);
        //     m_current_rpm = rpm;
        // }

        rv += m_i % QString::number(Distance(center_point.x() - start_point.x()).to(m_meta.m_distance_unit), 'f', 4) %
              m_j % QString::number(Distance(center_point.y() - start_point.y()).to(m_meta.m_distance_unit), 'f', 4) %
              m_x % QString::number(Distance(end_point.x()).to(m_meta.m_distance_unit), 'f', 4) %
              m_y % QString::number(Distance(end_point.y()).to(m_meta.m_distance_unit), 'f', 4) % " EM=1";

        // Rest of the method remains unchanged
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
        rv += m_sb->setting< QString >(Constants::PrinterSettings::GCode::kLayerCodeChange);
        return rv;
    }

    QString SiemensWriter::writeShutdown()
    {
        QString rv;

        // Voeg een comment toe voor de shutdown
        rv += commentLine("") % comment("END PROGRAM") % m_newline;

        rv += m_sb->setting< QString >(Constants::PrinterSettings::GCode::kEndCode) % m_newline;

        rv += "EXTRUDERSPEED2(0,0,1)" % commentSpaceLine("EXTRUDERSPEED 0 RPM");

        Distance current_height = m_current_z;
        Distance new_height = current_height + 50000;

        rv += "G1 Z" % QString::number(new_height.to(m_meta.m_distance_unit), 'f', 4) % " F3000" %
              commentSpaceLine("MOVE UP 50MM");

        rv += "H[10]=1" % commentSpaceLine("EXTRUDER OFF");

        rv += "M30" % commentSpaceLine("END OF G-CODE");

        return rv += ";";
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
        // Remove any code that uses the rpm parameter
        // Instead, use a fixed value or remove RPM-related commands entirely

        // Example: Set extruder on without specifying RPM
        m_extruders_on[0] = true;

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

