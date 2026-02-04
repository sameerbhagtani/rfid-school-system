import User from "../models/user.model.js";
import Attendance from "../models/attendance.model.js";
import Holiday from "../models/holiday.model.js";

import ExpressError from "../utils/ExpressError.util.js";

export async function markAttendance(req, res, next) {
    let { markedBy } = req.body;
    const { studentIds } = req.body;

    // capitalize IDs
    markedBy = markedBy.toUpperCase();
    let normalizedStudentIds = studentIds.map((id) => id.toUpperCase());

    // check for holiday
    const today = new Date();
    today.setUTCHours(0, 0, 0, 0);

    const isHoliday = await Holiday.findOne({ date: today });
    if (isHoliday) {
        throw new ExpressError(
            400,
            `Cannot mark attendance. Today is ${isHoliday.reason}.`,
        );
    }

    const teacher = await User.findOne({
        userId: markedBy,
        role: "teacher",
    });
    if (!teacher) {
        throw new ExpressError(404, "Teacher not found");
    }

    const validStudents = await User.find({
        userId: { $in: normalizedStudentIds },
        role: "student",
    });

    if (validStudents.length === 0) {
        throw new ExpressError(400, "No valid student IDs provided");
    }

    const attendanceRecords = validStudents.map((student) => ({
        user: student._id,
        date: today,
        markedBy: teacher._id,
    }));

    try {
        await Attendance.insertMany(attendanceRecords, { ordered: false });
    } catch (err) {
        const isDuplicateError =
            err.code === 11000 ||
            (err.writeErrors && err.writeErrors.some((e) => e.code === 11000));

        if (!isDuplicateError) {
            throw err;
        }
    }

    return res.json({
        success: true,
        message: `Marked ${attendanceRecords.length} students present.`,
    });
}

export async function getRole(req, res, next) {
    const { userId } = req.params;

    if (!userId || userId.length != 8) {
        throw new ExpressError(400, "Invalid User ID");
    }

    const user = await User.findOne({ userId });

    if (!user) {
        throw new ExpressError(404, "User not found");
    }

    return res.json({
        success: true,
        message: "User role fetched successfully",
        data: {
            role: user.role,
        },
    });
}

export async function getAnalytics(req, res, next) {
    return res.json({ message: "Get Analytics Route" });
}

export async function resetAttendanceForToday(req, res, next) {
    const today = new Date();
    today.setUTCHours(0, 0, 0, 0);

    const result = await Attendance.deleteMany({ date: today });

    return res.json({
        success: true,
        message: `Today's attendance has been reset. Removed ${result.deletedCount} records.`,
    });
}
