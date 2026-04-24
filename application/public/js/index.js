const studentsListEl = document.getElementById("students-list");
const stateMessageEl = document.getElementById("state-message");

function escapeHtml(value) {
    return String(value)
        .replaceAll("&", "&amp;")
        .replaceAll("<", "&lt;")
        .replaceAll(">", "&gt;")
        .replaceAll('"', "&quot;")
        .replaceAll("'", "&#39;");
}

function showError(message) {
    stateMessageEl.className = "state-message error";
    stateMessageEl.textContent = message;
}

async function loadStudents() {
    try {
        const response = await fetch("/api/users/students");
        const payload = await response.json();

        if (!response.ok || !payload.success) {
            showError(payload.message || "Unable to load students.");
            return;
        }

        const students = Array.isArray(payload.data) ? payload.data : [];
        stateMessageEl.textContent = students.length
            ? "Select a student to open analytics"
            : "No students found.";

        studentsListEl.innerHTML = students
            .map((student) => {
                const studentName = escapeHtml(student.name);
                const studentId = escapeHtml(student.userId);

                return `
                    <li class="student-item">
                        <a href="/students/${studentId}">
                            <span class="student-name">${studentName}</span>
                            <span class="student-id">${studentId}</span>
                        </a>
                    </li>
                `;
            })
            .join("");
    } catch (_error) {
        showError("Network error while loading students.");
    }
}

void loadStudents();
