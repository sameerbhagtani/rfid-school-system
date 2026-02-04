import { Router } from "express";
const router = Router();

import {
    markAttendance,
    getRole,
    getAnalytics,
} from "../controllers/index.controller.js";
import wrapAsync from "../utils/wrapAsync.util.js";

router.post("/mark-attendance", wrapAsync(markAttendance));
router.get("/get-role", wrapAsync(getRole));
router.get("/get-analytics", wrapAsync(getAnalytics));

export default router;
