import { connect } from "mongoose";

export async function connectDB() {
    const url = process.env.DB_URL;

    if (!url) {
        console.error(`❌ Error connecting to DB: URL is invalid`);
        process.exit(1);
    }

    try {
        await connect(url);
        console.log(`✅ Connected to DB`);
    } catch (err) {
        console.error(`❌ Error connecting to DB: ${err}`);
        process.exit(1);
    }
}
