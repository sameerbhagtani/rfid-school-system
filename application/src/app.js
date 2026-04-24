// load env variables
import "dotenv/config";

// core requires
import express from "express";
import path from "path";

// app initialization
const app = express();

// DB connection
import { connectDB } from "./db/index.js";
connectDB();

// global middlewares
app.use(express.json());
app.use(express.static("public"));

// routes
app.get("/", (req, res) => {
    return res.sendFile(path.resolve("public/index.html"));
});

app.get("/students/:studentId", (req, res) => {
    return res.sendFile(path.resolve("public/student.html"));
});

import userRoutes from "./routes/user.route.js";
app.use("/api/users", userRoutes);

import attendanceRoutes from "./routes/attendance.route.js";
app.use("/api/attendances", attendanceRoutes);

app.get("/api/ping", (req, res, next) => {
    return res.status(200).json({
        success: true,
        message: "All good!",
    });
});

// 404 handler
import ExpressError from "./utils/ExpressError.util.js";
app.all("*splat", (req, res, next) => {
    return next(new ExpressError(404, "Path not found"));
});

// error handler
import expressErrorHandler from "./utils/expressErrorHandler.util.js";
app.use(expressErrorHandler);

// start server
const PORT = Number(process.env.PORT) || 3000;
app.listen(PORT, "0.0.0.0", () => {
    console.log(`✅ Server started at PORT: ${PORT}`);
});
