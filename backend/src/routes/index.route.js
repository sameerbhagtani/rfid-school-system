import { Router } from "express";
const router = Router();

import {
    markAttendance,
    getRole,
    getAnalytics,
    resetAttendanceForToday,
} from "../controllers/index.controller.js";
import wrapAsync from "../utils/wrapAsync.util.js";

router.post("/mark-attendance", wrapAsync(markAttendance));
router.get("/get-role/:userId", wrapAsync(getRole));
router.get("/get-analytics/:studentId", wrapAsync(getAnalytics));
router.post("/reset-attendance", wrapAsync(resetAttendanceForToday));

export default router;
