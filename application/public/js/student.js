const studentTitleEl = document.getElementById("student-title");
const errorBoxEl = document.getElementById("error-box");
const analyticsContentEl = document.getElementById("analytics-content");
const monthLabelEl = document.getElementById("month-label");
const calendarGridEl = document.getElementById("calendar-grid");
const streakValueEl = document.getElementById("streak-value");
const studentPercentValueEl = document.getElementById("student-percent-value");
const classPercentValueEl = document.getElementById("class-percent-value");
const performanceValueEl = document.getElementById("performance-value");

function getStudentIdFromPathname() {
    const parts = window.location.pathname.split("/").filter(Boolean);
    return parts.at(-1) || "";
}

function normalizeDateKey(dateInput) {
    const date = new Date(dateInput);
    const year = date.getUTCFullYear();
    const month = String(date.getUTCMonth() + 1).padStart(2, "0");
    const day = String(date.getUTCDate()).padStart(2, "0");
    return `${year}-${month}-${day}`;
}

function setError(message) {
    errorBoxEl.hidden = false;
    errorBoxEl.textContent = message;
    analyticsContentEl.hidden = true;
}

function setStats(data) {
    streakValueEl.textContent = String(data.streak);
    studentPercentValueEl.textContent = `${Number(data.studentPercent).toFixed(2)}%`;
    classPercentValueEl.textContent = `${Number(data.classAvgPercent).toFixed(2)}%`;
    performanceValueEl.textContent = data.performance;
}

function createCell(value, className = "") {
    const cell = document.createElement("div");
    cell.className = `day-cell ${className}`.trim();
    cell.textContent = value;
    return cell;
}

function renderCalendar(holidayDates, attendanceDates) {
    const now = new Date();
    const year = now.getUTCFullYear();
    const month = now.getUTCMonth();
    const today = now.getUTCDate();

    const firstDayWeekIndex = new Date(Date.UTC(year, month, 1)).getUTCDay();
    const daysInMonth = new Date(Date.UTC(year, month + 1, 0)).getUTCDate();

    const monthName = new Date(Date.UTC(year, month, 1)).toLocaleString(
        "en-US",
        {
            month: "long",
            year: "numeric",
            timeZone: "UTC",
        },
    );
    monthLabelEl.textContent = monthName;
    calendarGridEl.innerHTML = "";

    for (let i = 0; i < firstDayWeekIndex; i += 1) {
        calendarGridEl.appendChild(createCell("", "empty"));
    }

    const holidaySet = new Set(holidayDates.map(normalizeDateKey));
    const attendanceSet = new Set(attendanceDates.map(normalizeDateKey));

    for (let day = 1; day <= daysInMonth; day += 1) {
        const dateKey = `${year}-${String(month + 1).padStart(2, "0")}-${String(day).padStart(2, "0")}`;

        let cls = "pending";

        if (day <= today) {
            if (holidaySet.has(dateKey)) {
                cls = "holiday";
            } else if (attendanceSet.has(dateKey)) {
                cls = "present";
            } else {
                cls = "absent";
            }
        }

        calendarGridEl.appendChild(createCell(String(day), cls));
    }
}

async function loadStudentAnalytics() {
    const studentId = getStudentIdFromPathname();

    if (!studentId) {
        setError("Student ID is missing in the URL.");
        return;
    }

    try {
        const response = await fetch(
            `/api/users/students/${encodeURIComponent(studentId)}/analytics`,
        );
        const payload = await response.json();

        if (!response.ok || !payload.success) {
            setError(
                payload.message || "Unable to load analytics for this student.",
            );
            return;
        }

        const data = payload.data;

        studentTitleEl.textContent = `Analytics: ${data.name}`;
        setStats(data);
        renderCalendar(data.holidayDates || [], data.attendanceDates || []);

        errorBoxEl.hidden = true;
        analyticsContentEl.hidden = false;
    } catch (_error) {
        setError("Network error while loading analytics.");
    }
}

void loadStudentAnalytics();
