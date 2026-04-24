import { Schema, model } from "mongoose";

const attendanceSchema = new Schema({
    user: {
        type: Schema.Types.ObjectId,
        ref: "User",
        required: true,
        index: true,
    },
    date: {
        type: Date,
        required: true,
        index: true,
    },
    markedBy: {
        type: Schema.Types.ObjectId,
        ref: "User",
        required: true,
    },
});

attendanceSchema.index({ user: 1, date: 1 }, { unique: true });

const Attendance = model("Attendance", attendanceSchema);

export default Attendance;
