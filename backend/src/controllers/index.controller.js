import User from "../models/user.model.js";
import Attendance from "../models/attendance.model.js";
import Holiday from "../models/holiday.model.js";

// import axios from "axios";

import ExpressError from "../utils/ExpressError.util.js";

export async function markAttendance(req, res, next) {
    let { markedBy } = req.body;
    const { studentIds } = req.body;

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

export async function getRole(req, res, next) {
    const { userId } = req.params;

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

    // const prompt = `
    // # Role
    // You are a friendly AI analytics station for a classroom.

    // # Task
    // Write a single, short sentence to be read aloud via a speaker to a student.

    // # Input Data
    // - Name: ${student.name}
    // - Streak: ${streak} days
    // - Attendance: ${studentPercent}%
    // - Class Average: ${classAvgPercent}%
    // - Status: ${performance} average

    // # Constraints
    // - Strictly one sentence. Keep it brief and easy to hear.
    // - No emojis and no "fancy" or "heavy" English words.
    // - Tone: Encouraging, slightly creative, and a bit witty.
    // - Content: You must mention the name, the streak, the attendance percentage, and how they compare to the class.

    // # Examples
    // - Nice work ABC, you've hit a 5day streak with 80% attendance, putting you right above the class average!
    // - Keep it up ABC, you've got a 3 day streak at 70% attendance, which is a bit below the class average this month, but you're catching up!
    // `;

    // try {
    //     const response = await axios.post(
    //         "http://localhost:11434/api/generate",
    //         {
    //             model: "gemma3:4b",
    //             prompt: prompt,
    //             stream: false,
    //             options: {
    //                 num_ctx: 1024,
    //                 temperature: 0.6,
    //             },
    //         },
    //     );

    //     const aiText = response.data.response.trim();

    //     return res.json({
    //         success: true,
    //         message: aiText,
    //     });
    // } catch (error) {
    //     console.error("Ollama error : ", error.message);
    //     return res.json({
    //         success: true,
    //         text: `Welcome back, ${student.name}! Your attendance is ${studentPercent} percent.`,
    //     });
    // }
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
