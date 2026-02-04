import User from "../models/user.model.js";
import Attendance from "../models/attendance.model.js";
import Holiday from "../models/holiday.model.js";

export async function markAttendance(req, res, next) {
    return res.json({ message: "Mark Attendance Route" });
}

export async function getRole(req, res, next) {
    return res.json({ message: "Get Role Route" });
}

export async function getAnalytics(req, res, next) {
    return res.json({ message: "Get Analytics Route" });
}
