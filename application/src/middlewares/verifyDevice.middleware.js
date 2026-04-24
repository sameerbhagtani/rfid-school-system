export default function verifyDevice(req, res, next) {
    const key = req.header("x-api-key");

    if (!key || key !== process.env.DEVICE_API_KEY) {
        return res.status(401).json({
            success: false,
            message: "Unauthorized",
        });
    }

    return next();
}
