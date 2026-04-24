import User from "../models/user.model.js";
import Attendance from "../models/attendance.model.js";
import Holiday from "../models/holiday.model.js";

import ExpressError from "../utils/ExpressError.util.js";

export async function getAllStudents(req, res, next) {
    const students = await User.find({ role: "student" }).sort({ name: 1 });

    return res.status(200).json({
        success: true,
        data: students.map((student) => ({
            userId: student.userId,
            name: student.name,
        })),
    });
}

export async function getAllTeacherIds(req, res, next) {
    const teachers = await User.find({ role: "teacher" });

    return res.status(200).json({
        success: true,
        data: teachers.map((teacher) => teacher.userId),
    });
}

export async function getResetId(req, res, next) {
    const reset = await User.findOne({ role: "reset" });

    return res.status(200).json({
        success: true,
        data: {
            id: reset.userId,
        },
    });
}

export async function getRole(req, res, next) {
    const { userId } = req.params;

    if (typeof userId !== "string" || userId.trim().length !== 8) {
        throw new ExpressError(400, "Invalid User ID");
    }

    const user = await User.findOne({ userId: userId.toUpperCase() });
    if (!user) {
        throw new ExpressError(404, "User not found");
    }

    return res.json({
        success: true,
        data: {
            role: user.role,
            name: user.name,
        },
    });
}

export async function getAnalytics(req, res, next) {
    const { studentId } = req.params;

    if (typeof studentId !== "string" || studentId.trim().length !== 8) {
        throw new ExpressError(400, "Invalid Student ID");
    }

    const student = await User.findOne({
        userId: studentId.toUpperCase(),
        role: "student",
    });
    if (!student) {
        throw new ExpressError(404, "Student not found");
    }

    const now = new Date();
    const startOfMonth = new Date(
        Date.UTC(now.getUTCFullYear(), now.getUTCMonth(), 1),
    );
    const todayMidnight = new Date(
        Date.UTC(now.getUTCFullYear(), now.getUTCMonth(), now.getUTCDate()),
    );

    const [monthlyRecords, holidays, totalUsers, allMonthlyAttendance] =
        await Promise.all([
            Attendance.find({
                user: student._id,
                date: { $gte: startOfMonth, $lte: todayMidnight },
            }).sort({ date: -1 }),

            Holiday.find({
                date: { $gte: startOfMonth, $lte: todayMidnight },
            }),

            User.countDocuments({ role: "student" }),

            Attendance.countDocuments({
                date: { $gte: startOfMonth, $lte: todayMidnight },
            }),
        ]);

    // calculate actual school days (excluding holidays)
    const daysInMonthSoFar = now.getUTCDate();
    const schoolDaysSoFar = Math.max(1, daysInMonthSoFar - holidays.length);

    // calculate student and class percentages
    const studentPercent = (
        (monthlyRecords.length / schoolDaysSoFar) *
        100
    ).toFixed(1);

    const classAvgPercent =
        totalUsers > 0
            ? (
                  (allMonthlyAttendance / (totalUsers * schoolDaysSoFar)) *
                  100
              ).toFixed(1)
            : 0;

    // calculate streak
    let streak = 0;
    let checkDate = new Date(todayMidnight);

    for (const record of monthlyRecords) {
        while (holidays.some((h) => h.date.getTime() === checkDate.getTime())) {
            checkDate.setUTCDate(checkDate.getUTCDate() - 1);
        }

        if (record.date.getTime() === checkDate.getTime()) {
            streak++;
            checkDate.setUTCDate(checkDate.getUTCDate() - 1);
        } else if (record.date.getTime() < checkDate.getTime()) {
            break;
        }
    }

    const performance =
        parseFloat(studentPercent) >= parseFloat(classAvgPercent)
            ? "above"
            : "below";

    const attendanceDates = monthlyRecords.map(
        (r) => r.date.toISOString().split("T")[0],
    );
    const holidayDates = holidays.map(
        (h) => h.date.toISOString().split("T")[0],
    );

    return res.status(200).json({
        success: true,
        data: {
            name: student.name,
            streak,
            studentPercent,
            classAvgPercent,
            performance,
            attendanceDates, // array of present dates
            holidayDates, // array of holiday dates
        },
    });
}
