import { Schema, model } from "mongoose";

const holidaySchema = new Schema({
    date: {
        type: Date,
        required: true,
        unique: true,
    },
    reason: {
        type: String,
        required: true,
        trim: true,
    },
});

const Holiday = model("Holiday", holidaySchema);

export default Holiday;
