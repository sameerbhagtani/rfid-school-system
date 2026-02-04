import "dotenv/config";
import { connect, disconnect } from "mongoose";

// models
import User from "../models/user.model.js";
import Attendance from "../models/attendance.model.js";
import Holiday from "../models/holiday.model.js";

// data
import usersData from "../data/users.data.json" with { type: "json" };
import attendancesData from "../data/attendances.data.json" with { type: "json" };
import holidaysData from "../data/holidays.data.json" with { type: "json" };

function parseDate(dateStr) {
    const [year, month, day] = dateStr.split("-");

    return new Date(Date.UTC(year, month - 1, day, 0, 0, 0));
}

async function initDB() {
    const url = process.env.DB_URL;
    if (!url) {
        console.error(`❌ Error connecting to DB : URL is invalid`);
        process.exit(1);
    }

    try {
        await connect(url);
        console.log(`✅ Connected to DB`);

        // clean collections
        await User.deleteMany({});
        await Attendance.deleteMany({});
        await Holiday.deleteMany({});
        console.log("✅ Existing data cleared");

        // insert users
        const createdUsers = await User.insertMany(usersData);
        console.log(`✅ Inserted ${createdUsers.length} user records`);

        // insert holidays
        const formattedHolidays = holidaysData.map((h) => ({
            ...h,
            date: parseDate(h.date),
        }));
        await Holiday.insertMany(formattedHolidays);
        console.log(`✅ Inserted ${formattedHolidays.length} holiday records`);

        // create a lookup map: { userId -> Mongo ObjectId}
        const userMap = {};
        createdUsers.forEach((u) => {
            userMap[u.userId] = u._id;
        });

        // insert attendances
        const formattedAttendance = attendancesData.map((a) => ({
            user: userMap[a.user],
            date: parseDate(a.date),
            markedBy: userMap[a.markedBy],
        }));
        await Attendance.insertMany(formattedAttendance);
        console.log(
            `✅ Inserted ${formattedAttendance.length} attendance records`,
        );

        console.log("✅ DB initialized");
    } catch (err) {
        console.error(`❌ Error initializing DB: ${err.message}`);
    } finally {
        await disconnect();
        process.exit();
    }
}

initDB();
