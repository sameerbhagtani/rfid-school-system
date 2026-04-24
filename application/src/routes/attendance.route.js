import { Router } from "express";
const router = Router();

import verifyDevice from "../middlewares/verifyDevice.middleware.js";

import {
    markAttendance,
    resetAttendanceForToday,
} from "../controllers/attendance.controller.js";

router.post("/", verifyDevice, markAttendance);
router.delete("/today", verifyDevice, resetAttendanceForToday);

export default router;
