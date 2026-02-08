// load env variables
import "dotenv/config";

// core requires
import express from "express";
import cors from "cors";

// app initialization
const app = express();

// DB connection
import { connectDB } from "./config/db.config.js";
connectDB();

// global middlewares
app.use(express.json());
app.use(cors());

// routes
import router from "./routes/index.route.js";
app.use("/api", router);

app.get("/api/ping", (req, res, next) => {
    return res.status(200).json({
        success: true,
        message: "All good!",
    });
});

// 404 handler
import ExpressError from "./utils/ExpressError.util.js";
app.all("*splat", (req, res, next) => {
    next(new ExpressError(404, "API endpoint not found!"));
});

// error handler
import expressErrorHandler from "./utils/expressErrorHandler.js";
app.use(expressErrorHandler);

// start server
const PORT = Number(process.env.PORT) || 3000;
app.listen(PORT, "0.0.0.0", () => {
    console.log(`✅ Server started at PORT: ${PORT}`);
});
