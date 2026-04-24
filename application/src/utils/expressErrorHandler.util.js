export default function expressErrorHandler(err, req, res, next) {
    const status = err.status || 500;
    const message = err.message || "Something went wrong";

    console.error(err);

    return res.status(status).json({
        success: false,
        message,
    });
}
