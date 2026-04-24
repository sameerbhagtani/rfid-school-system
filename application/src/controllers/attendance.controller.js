import User from "../models/user.model.js";
import Attendance from "../models/attendance.model.js";
import Holiday from "../models/holiday.model.js";

import ExpressError from "../utils/ExpressError.util.js";

export async function markAttendance(req, res, next) {
    let { markedBy } = req.body;
    const { studentIds } = req.body;

    // validations
    if (typeof markedBy !== "string" || !markedBy.trim()) {
        throw new ExpressError(400, "markedBy is required");
    }

    if (!Array.isArray(studentIds) || studentIds.length === 0) {
        throw new ExpressError(400, "studentIds must be a non-empty array");
    }

    if (!studentIds.every((id) => typeof id === "string" && id.trim())) {
        throw new ExpressError(400, "All studentIds must be valid strings");
    }

    // capitalize IDs
    markedBy = markedBy.toUpperCase();
    const normalizedStudentIds = studentIds.map((id) => id.toUpperCase());

    const today = new Date();
    today.setUTCHours(0, 0, 0, 0);

    // check for holiday
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
        message: `Attendance Marked. Students : ${attendanceRecords.length}`,
    });
}

export async function resetAttendanceForToday(req, res, next) {
    const today = new Date();
    today.setUTCHours(0, 0, 0, 0);

    const result = await Attendance.deleteMany({ date: today });

    return res.json({
        success: true,
        message: `Removed ${result.deletedCount} records.`,
    });
}
