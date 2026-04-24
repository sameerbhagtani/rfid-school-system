import { Router } from "express";
const router = Router();

import {
    getAllStudents,
    getAllTeacherIds,
    getResetId,
    getRole,
    getAnalytics,
} from "../controllers/user.controller.js";

router.get("/students", getAllStudents);
router.get("/teachers/ids", getAllTeacherIds);
router.get("/reset/id", getResetId);
router.get("/:userId/role", getRole);
router.get("/students/:studentId/analytics", getAnalytics);

export default router;
