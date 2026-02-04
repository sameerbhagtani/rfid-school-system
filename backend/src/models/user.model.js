import { Schema, model } from "mongoose";

const userSchema = new Schema({
    userId: {
        type: String,
        required: true,
        unique: true,
        index: true,
        trim: true,
    },
    name: {
        type: String,
        required: true,
        trim: true,
    },
    role: {
        type: String,
        enum: ["student", "teacher", "reset"],
        default: "student",
    },
});

const User = model("User", userSchema);

export default User;
